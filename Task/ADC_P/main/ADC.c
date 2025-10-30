
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/adc.h"
#include "driver/gpio.h" // Botón GPIO/ISR

#include "ledc.h"
#include "uart_user.h"
#include "isr.h"

// ------------------- Config ADC/Pines -------------------
#define NUM_SAMPLES 64
#define VCC 3.3f
#define ADC_MAX 4095

#define POT_CH ADC_CHANNEL_6 // Potenciómetro en ADC1_CH6
#define NTC_CH ADC_CHANNEL_7 // NTC en ADC1_CH7

// Botón: en muchas ESP32 DevKitC el BOOT es GPIO0 (activo a LOW)
#define BUTTON_PIN 0

// Límites para el periodo de logging de temperatura (ms)
#define NTC_PERIOD_MIN_MS 100
#define NTC_PERIOD_MAX_MS 10000

static const char *TAG = "ADC_APP";

// ------------------- Modelo NTC -------------------
typedef enum
{
    NTC_MODEL_BETA = 0,
    NTC_MODEL_SH = 1
} ntc_model_t;

typedef struct
{
    ntc_model_t model; // 0: Beta, 1: Steinhart–Hart
    // Parámetros Beta
    float BETA; // p.ej. 4100
    float R0;   // 10k @25°C
    float T0;   // 298.15 K (25°C)
    // Steinhart–Hart
    float a, b, c; // 1/T = a + b lnR + c (lnR)^3
    // Hardware
    float Rfix; // Resistencia del divisor (ohm)
} ntc_params_t;

// ---------------- Estructura compartida (sin globales) ----------------
typedef struct
{
    LED led;

    // Umbrales de temperatura (°C) por color
    float thr_R[2];
    float thr_G[2];
    float thr_B[2];

    // Brillo (duty 0..255) desde potenciómetro
    uint16_t pot_duty;

    // Sincronización general
    SemaphoreHandle_t mtx;

    // ADC one-shot + mutex dedicado para lecturas concurrentes
    adc_oneshot_unit_handle_t adc;
    SemaphoreHandle_t adc_mtx;

    // --- Estado del botón / control de LED ---
    volatile bool led_enabled;         // 1=encendido normal, 0=apagado por botón
    volatile bool led_enabled_changed; // flag de flanco

    // Valores guardados al apagar por botón (para restaurar iguales)
    uint8_t saved_r, saved_g, saved_b;
    uint8_t saved_duty;

    // --- Periodo de impresión de temperatura (ms) ---
    uint32_t ntc_log_ms; // configurable por TPERIOD

    // --- Parámetros NTC (modelo seleccionado) ---
    ntc_params_t ntc;
} shared_data_t;

// ---------------- Prototipos internos ----------------
static int adc_average(adc_oneshot_unit_handle_t handle, adc_channel_t ch, int samples);
static void task_pot(void *pvParameters);
static void task_ntc(void *pvParameters);
static void task_uart(void *pvParameters);
static adc_oneshot_unit_handle_t adc_init(void);
static void IRAM_ATTR button_isr(void *arg);

// --------- Helpers NTC: ADC→Vout, Vout→Rntc, modelos Beta/SH ----------
static inline float adc_raw_to_vout(int raw)
{
    return (raw * VCC) / ADC_MAX; // si calibras ADC, reemplaza por adc_cali_raw_to_voltage()
}

static inline float vout_to_rntc(float vout, float Rfix)
{
    float Vout = vout;
    if (VCC - Vout < 1e-3f)
        Vout = VCC - 1e-3f;
    return (Rfix * Vout) / (VCC - Vout);
}

static inline float beta_from_two_points(float T1K, float R1, float T2K, float R2)
{
    return logf(R1 / R2) / ((1.0f / T1K) - (1.0f / T2K));
}

static inline float R0_from_point(float T0K, float R, float BETA, float TmK)
{
    return R * expf(-BETA * ((1.0f / TmK) - (1.0f / T0K)));
}

static inline float tempK_from_beta(float R, float BETA, float R0, float T0K)
{
    return 1.0f / ((1.0f / T0K) + (1.0f / BETA) * logf(R / R0));
}

static void steinhart_hart_from_3pts(float T1K, float R1, float T2K, float R2, float T3K, float R3,
                                     float *a, float *b, float *c)
{
    float L1 = logf(R1), L2 = logf(R2), L3 = logf(R3);
    float Y1 = 1.0f / T1K, Y2 = 1.0f / T2K, Y3 = 1.0f / T3K;

    float dY12 = Y2 - Y1;
    float dY23 = Y3 - Y2;
    float dL12 = L2 - L1;
    float dL23 = L3 - L2;
    float dL3_12 = (L2 * L2 * L2 - L1 * L1 * L1);
    float dL3_23 = (L3 * L3 * L3 - L2 * L2 * L2);

    float c_local = (dY23 / dL23 - dY12 / dL12) / (dL3_23 / dL23 - dL3_12 / dL12);
    float b_local = dY12 / dL12 - c_local * dL3_12 / dL12;
    float a_local = Y1 - b_local * L1 - c_local * L1 * L1 * L1;

    *a = a_local;
    *b = b_local;
    *c = c_local;
}

static inline float tempK_from_sh(float R, float a, float b, float c)
{
    float L = logf(R);
    return 1.0f / (a + b * L + c * L * L * L);
}

// ---------------- Lectura promedio ADC (one-shot) ----------------
static int adc_average(adc_oneshot_unit_handle_t handle, adc_channel_t ch, int samples)
{
    int acc = 0, val = 0;
    for (int i = 0; i < samples; i++)
    {
        if (adc_oneshot_read(handle, ch, &val) == ESP_OK)
            acc += val;
    }
    return acc / (samples > 0 ? samples : 1);
}

// ---------------- Tarea: Pot -> pot_duty ----------------
static void task_pot(void *pvParameters)
{
    shared_data_t *ctx = (shared_data_t *)pvParameters;
    while (1)
    {
        if (xSemaphoreTake(ctx->adc_mtx, portMAX_DELAY))
        {
            int raw = adc_average(ctx->adc, POT_CH, NUM_SAMPLES);
            xSemaphoreGive(ctx->adc_mtx);

            if (raw < 0)
                raw = 0;
            if (raw > ADC_MAX)
                raw = ADC_MAX;

            uint16_t duty = (uint16_t)((raw * 255) / ADC_MAX);

            if (xSemaphoreTake(ctx->mtx, portMAX_DELAY))
            {
                ctx->pot_duty = duty;
                xSemaphoreGive(ctx->mtx);
            }

            int mv = (raw * 3300) / ADC_MAX;
            ESP_LOGI("POT", "ADC6 raw=%d V=%dmV duty=%u/255", raw, mv, duty);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ---------------- Mezcla: determina r,g,b por temperatura ----------------
static inline void color_mix_from_temp(const shared_data_t *ctx, float Tc, bool *r, bool *g, bool *b)
{
    *r = (Tc >= ctx->thr_R[0] && Tc <= ctx->thr_R[1]);
    *g = (Tc >= ctx->thr_G[0] && Tc <= ctx->thr_G[1]);
    *b = (Tc >= ctx->thr_B[0] && Tc <= ctx->thr_B[1]);
}

// ---------------- ISR botón: toggle con antirrebote ----------------
static void IRAM_ATTR button_isr(void *arg)
{
    shared_data_t *ctx = (shared_data_t *)arg;

    // Antirrebote por tiempo (200 ms)
    static uint32_t last_isr_tick = 0;
    uint32_t now = xTaskGetTickCountFromISR();
    if ((now - last_isr_tick) < pdMS_TO_TICKS(200))
        return;
    last_isr_tick = now;

    // Toggle estado y marca flanco
    bool new_state = !ctx->led_enabled;
    ctx->led_enabled = new_state;
    ctx->led_enabled_changed = true;
}

// ---------------- Tarea: NTC -> mezcla RGB (PWM sincronizado) ----------------
static void task_ntc(void *pvParameters)
{
    shared_data_t *ctx = (shared_data_t *)pvParameters;

    while (1)
    {
        int raw = 0;
        if (xSemaphoreTake(ctx->adc_mtx, portMAX_DELAY))
        {
            raw = adc_average(ctx->adc, NTC_CH, NUM_SAMPLES);
            xSemaphoreGive(ctx->adc_mtx);
        }

        float Vout = adc_raw_to_vout(raw);
        float Rntc = vout_to_rntc(Vout, ctx->ntc.Rfix);

        float Tk = 298.15f; // Kelvin
        if (ctx->ntc.model == NTC_MODEL_BETA)
        {
            Tk = tempK_from_beta(Rntc, ctx->ntc.BETA, ctx->ntc.R0, ctx->ntc.T0);
        }
        else
        {
            Tk = tempK_from_sh(Rntc, ctx->ntc.a, ctx->ntc.b, ctx->ntc.c);
        }
        float Tc = Tk - 273.15f;

        uint16_t duty_local = 0;
        bool r = false, g = false, b = false;
        uint32_t log_ms_local = 400; // fallback

        if (xSemaphoreTake(ctx->mtx, portMAX_DELAY))
        {
            duty_local = ctx->pot_duty;
            color_mix_from_temp(ctx, Tc, &r, &g, &b);
            log_ms_local = ctx->ntc_log_ms;
            xSemaphoreGive(ctx->mtx);
        }

        // Calcula los PWM resultantes por mezcla
        uint8_t new_r = (uint8_t)(r ? duty_local : 0);
        uint8_t new_g = (uint8_t)(g ? duty_local : 0);
        uint8_t new_b = (uint8_t)(b ? duty_local : 0);

        // Lee estado (toggle por botón)
        bool enabled = ctx->led_enabled;

        // Manejo de flanco del botón
        if (ctx->led_enabled_changed)
        {
            if (!enabled)
            {
                // Habilitado -> Apagado: guardar y apagar
                ctx->saved_r = new_r;
                ctx->saved_g = new_g;
                ctx->saved_b = new_b;
                ctx->saved_duty = (uint8_t)duty_local;

                led_rgb_write(&ctx->led, 0, 0, 0); // apagar inmediato
                ESP_LOGI("BTN", "LED OFF (saved R=%u G=%u B=%u duty=%u)",
                         ctx->saved_r, ctx->saved_g, ctx->saved_b, ctx->saved_duty);
            }
            else
            {
                // Apagado -> Habilitado: restaurar exactamente lo que tenía
                led_rgb_write(&ctx->led, ctx->saved_r, ctx->saved_g, ctx->saved_b);
                ESP_LOGI("BTN", "LED RESTORE (R=%u G=%u B=%u duty=%u)",
                         ctx->saved_r, ctx->saved_g, ctx->saved_b, ctx->saved_duty);
            }
            ctx->led_enabled_changed = false; // limpiar flanco
        }
        else
        {
            // Operación normal
            if (enabled)
            {
                led_rgb_write(&ctx->led, new_r, new_g, new_b);
            }
        }

        ESP_LOGI("NTC", "T=%.2f°C -> R=%d G=%d B=%d Duty=%u Enabled=%d",
                 Tc, r, g, b, duty_local, enabled);

        // Espera usando el periodo configurable
        vTaskDelay(pdMS_TO_TICKS(log_ms_local));
    }
}

// ---------------- Tarea UART ----------------
static void task_uart(void *pvParameters)
{
    shared_data_t *ctx = (shared_data_t *)pvParameters;
    char buf[128];

    uart_send("\r\nUART listo. Comandos:\r\n");
    uart_send("  HELP\r\n  GET\r\n  SET R <lo> <hi>\r\n  SET G <lo> <hi>\r\n  SET B <lo> <hi>\r\n");
    uart_send("  COLOR <r 0-255> <g 0-255> <b 0-255>\r\n  OFF\r\n");
    uart_send("  TPERIOD [ms]      -> fija o consulta periodo de impresión de temperatura\r\n");
    uart_send("  POTV              -> lee voltaje del potenciómetro\r\n");
    uart_send("  NTC?              -> consulta parámetros de modelo NTC\r\n");
    uart_send("  NTCRAW            -> lee raw instantáneo del NTC\r\n");
    uart_send("  CAL2 T1C raw1 T2C raw2      (calib. Beta)\r\n");
    uart_send("  CAL3 T1C raw1 T2C raw2 T3C raw3  (calib. SH)\r\n");
    uart_send("  MODEL BETA | MODEL SH\r\n");
    uart_send("  SETNTC BETA <val> | SETNTC R0 <ohm> | SETNTC RFIX <ohm>\r\n");

    while (1)
    {
        if (uart_available(buf, sizeof(buf)))
        {
            // Normaliza espacios
            for (char *p = buf; *p; ++p)
                if (*p == '\r' || *p == '\n' || *p == '\t')
                    *p = ' ';

            // Variables auxiliares para sscanf
            float f1 = 0.0f;

            // ----- Ayuda -----
            if (strncasecmp(buf, "HELP", 4) == 0)
            {
                uart_send("Comandos:\r\n");
                uart_send("  SET R/G/B <lo> <hi>\r\n  GET\r\n  COLOR r g b\r\n  OFF\r\n");
                uart_send("  TPERIOD [ms] | POTV\r\n");
                uart_send("  NTC? | NTCRAW | CAL2 T1 raw1 T2 raw2 | CAL3 T1 raw1 T2 raw2 T3 raw3\r\n");
                uart_send("  MODEL BETA|SH | SETNTC BETA v | SETNTC R0 ohm | SETNTC RFIX ohm\r\n");
            }
            // ----- Mostrar umbrales y estado -----
            else if (strncasecmp(buf, "GET", 3) == 0)
            {
                if (xSemaphoreTake(ctx->mtx, portMAX_DELAY))
                {
                    char msg[300];
                    snprintf(msg, sizeof(msg),
                             "R:[%.1f,%.1f]  G:[%.1f,%.1f]  B:[%.1f,%.1f]  ANODE=%d  FREQ=%luHz  ENABLED=%d  TPERIOD=%lums\r\n",
                             ctx->thr_R[0], ctx->thr_R[1],
                             ctx->thr_G[0], ctx->thr_G[1],
                             ctx->thr_B[0], ctx->thr_B[1],
                             (int)ctx->led.common_anode,
                             (unsigned long)ctx->led.FREQ_HZ,
                             (int)ctx->led_enabled,
                             (unsigned long)ctx->ntc_log_ms);
                    uart_send(msg);
                    xSemaphoreGive(ctx->mtx);
                }
            }
            // ----- Cambiar umbrales -----
            else if (strncasecmp(buf, "SET", 3) == 0)
            {
                char which;
                float lo, hi;
                if (sscanf(buf, "SET %c %f %f", &which, &lo, &hi) == 3)
                {
                    if (xSemaphoreTake(ctx->mtx, portMAX_DELAY))
                    {
                        float *t = NULL;
                        switch (which)
                        {
                        case 'R':
                        case 'r':
                            t = ctx->thr_R;
                            break;
                        case 'G':
                        case 'g':
                            t = ctx->thr_G;
                            break;
                        case 'B':
                        case 'b':
                            t = ctx->thr_B;
                            break;
                        default:
                            t = NULL;
                            break;
                        }
                        if (t)
                        {
                            t[0] = lo;
                            t[1] = hi;
                            uart_send("OK\r\n");
                        }
                        else
                        {
                            uart_send("Uso: SET <R|G|B> <lo> <hi>\r\n");
                        }
                        xSemaphoreGive(ctx->mtx);
                    }
                }
                else
                    uart_send("Uso: SET <R|G|B> <lo> <hi>\r\n");
            }
            // ----- Forzar color manualmente -----
            else if (strncasecmp(buf, "COLOR", 5) == 0)
            {
                int r, g, b;
                if (sscanf(buf, "COLOR %d %d %d", &r, &g, &b) == 3)
                {
                    if (r < 0)
                        r = 0;
                    if (r > 255)
                        r = 255;
                    if (g < 0)
                        g = 0;
                    if (g > 255)
                        g = 255;
                    if (b < 0)
                        b = 0;
                    if (b > 255)
                        b = 255;
                    led_rgb_write(&ctx->led, (uint8_t)r, (uint8_t)g, (uint8_t)b);
                    uart_send("OK COLOR\r\n");
                }
                else
                {
                    uart_send("Uso: COLOR <r 0-255> <g 0-255> <b 0-255>\r\n");
                }
            }
            // ----- Apagar LED por comando -----
            else if (strncasecmp(buf, "OFF", 3) == 0)
            {
                led_rgb_write(&ctx->led, 0, 0, 0);
                uart_send("OK OFF\r\n");
            }
            // ----- TPERIOD [ms] -----
            else if (strncasecmp(buf, "TPERIOD", 7) == 0)
            {
                unsigned ms;
                if (sscanf(buf, "TPERIOD %u", &ms) == 1)
                {
                    if (ms < NTC_PERIOD_MIN_MS)
                        ms = NTC_PERIOD_MIN_MS;
                    if (ms > NTC_PERIOD_MAX_MS)
                        ms = NTC_PERIOD_MAX_MS;
                    if (xSemaphoreTake(ctx->mtx, portMAX_DELAY))
                    {
                        ctx->ntc_log_ms = ms;
                        xSemaphoreGive(ctx->mtx);
                    }
                    uart_send("OK TPERIOD\r\n");
                }
                else
                {
                    char msg[48];
                    uint32_t cur = 0;
                    if (xSemaphoreTake(ctx->mtx, portMAX_DELAY))
                    {
                        cur = ctx->ntc_log_ms;
                        xSemaphoreGive(ctx->mtx);
                    }
                    snprintf(msg, sizeof(msg), "TPERIOD=%lums\r\n", (unsigned long)cur);
                    uart_send(msg);
                }
            }
            // ----- POTV -----
            else if (strncasecmp(buf, "POTV", 4) == 0)
            {
                int raw = 0;
                if (xSemaphoreTake(ctx->adc_mtx, portMAX_DELAY))
                {
                    raw = adc_average(ctx->adc, POT_CH, NUM_SAMPLES / 2);
                    xSemaphoreGive(ctx->adc_mtx);
                }
                if (raw < 0)
                    raw = 0;
                if (raw > ADC_MAX)
                    raw = ADC_MAX;
                int mv = (raw * 3300) / ADC_MAX;
                int pct = (raw * 100) / ADC_MAX;
                char msg[64];
                snprintf(msg, sizeof(msg), "POT: %d mV (%d%%)\r\n", mv, pct);
                uart_send(msg);
            }
            // ----- NTC? -----
            else if (strncasecmp(buf, "NTC?", 4) == 0)
            {
                char msg[220];
                snprintf(msg, sizeof(msg),
                         "NTC model=%s | BETA=%.2f R0=%.1f T0=%.2fK | a=%.6e b=%.6e c=%.6e | Rfix=%.1f\r\n",
                         (ctx->ntc.model == NTC_MODEL_BETA ? "BETA" : "SH"),
                         ctx->ntc.BETA, ctx->ntc.R0, ctx->ntc.T0,
                         ctx->ntc.a, ctx->ntc.b, ctx->ntc.c, ctx->ntc.Rfix);
                uart_send(msg);
            }
            // ----- SETNTC ... -----
            else if (sscanf(buf, "SETNTC BETA %f", &f1) == 1)
            {
                ctx->ntc.BETA = f1;
                uart_send("OK SETNTC BETA\r\n");
            }
            else if (sscanf(buf, "SETNTC R0 %f", &f1) == 1)
            {
                ctx->ntc.R0 = f1;
                uart_send("OK SETNTC R0\r\n");
            }
            else if (sscanf(buf, "SETNTC RFIX %f", &f1) == 1)
            {
                ctx->ntc.Rfix = f1;
                uart_send("OK SETNTC RFIX\r\n");
            }
            // ----- MODEL -----
            else if (strncasecmp(buf, "MODEL SH", 8) == 0)
            {
                ctx->ntc.model = NTC_MODEL_SH;
                uart_send("OK MODEL SH\r\n");
            }
            else if (strncasecmp(buf, "MODEL BETA", 10) == 0)
            {
                ctx->ntc.model = NTC_MODEL_BETA;
                uart_send("OK MODEL BETA\r\n");
            }
            // ----- NTCRAW -----
            else if (strncasecmp(buf, "NTCRAW", 6) == 0)
            {
                int raw = 0;
                if (xSemaphoreTake(ctx->adc_mtx, portMAX_DELAY))
                {
                    raw = adc_average(ctx->adc, NTC_CH, NUM_SAMPLES / 2);
                    xSemaphoreGive(ctx->adc_mtx);
                }
                char msg[64];
                snprintf(msg, sizeof(msg), "NTCRAW=%d\r\n", raw);
                uart_send(msg);
            }
            // ----- CAL2 (Beta, 2 puntos) -----
            else if (strncasecmp(buf, "CAL2", 4) == 0)
            {
                float T1C, T2C;
                int raw1, raw2;
                if (sscanf(buf, "CAL2 %f %d %f %d", &T1C, &raw1, &T2C, &raw2) == 4)
                {
                    float T1K = T1C + 273.15f, T2K = T2C + 273.15f;
                    float R1 = vout_to_rntc(adc_raw_to_vout(raw1), ctx->ntc.Rfix);
                    float R2 = vout_to_rntc(adc_raw_to_vout(raw2), ctx->ntc.Rfix);
                    float beta = beta_from_two_points(T1K, R1, T2K, R2);
                    // Ajusta R0 usando el punto más cercano a 25°C
                    float TmK = (fabsf(T1C - 25.0f) < fabsf(T2C - 25.0f)) ? T1K : T2K;
                    float Rm = (fabsf(T1C - 25.0f) < fabsf(T2C - 25.0f)) ? R1 : R2;
                    float R0 = R0_from_point(ctx->ntc.T0, Rm, beta, TmK);

                    ctx->ntc.model = NTC_MODEL_BETA;
                    ctx->ntc.BETA = beta;
                    ctx->ntc.R0 = R0;

                    char msg[128];
                    snprintf(msg, sizeof(msg),
                             "OK CAL2 -> BETA=%.2f R0=%.1f (T0=%.2fK)\r\n",
                             beta, R0, ctx->ntc.T0);
                    uart_send(msg);
                }
                else
                {
                    uart_send("Uso: CAL2 <T1C> <raw1> <T2C> <raw2>\r\n");
                }
            }
            // ----- CAL3 (Steinhart–Hart, 3 puntos) -----
            else if (strncasecmp(buf, "CAL3", 4) == 0)
            {
                float T1C, T2C, T3C;
                int raw1, raw2, raw3;
                if (sscanf(buf, "CAL3 %f %d %f %d %f %d", &T1C, &raw1, &T2C, &raw2, &T3C, &raw3) == 6)
                {
                    float T1K = T1C + 273.15f, T2K = T2C + 273.15f, T3K = T3C + 273.15f;
                    float R1 = vout_to_rntc(adc_raw_to_vout(raw1), ctx->ntc.Rfix);
                    float R2 = vout_to_rntc(adc_raw_to_vout(raw2), ctx->ntc.Rfix);
                    float R3 = vout_to_rntc(adc_raw_to_vout(raw3), ctx->ntc.Rfix);
                    float a, b, c;
                    steinhart_hart_from_3pts(T1K, R1, T2K, R2, T3K, R3, &a, &b, &c);

                    ctx->ntc.model = NTC_MODEL_SH;
                    ctx->ntc.a = a;
                    ctx->ntc.b = b;
                    ctx->ntc.c = c;

                    char msg[160];
                    snprintf(msg, sizeof(msg),
                             "OK CAL3 -> a=%.6e b=%.6e c=%.6e\r\n", a, b, c);
                    uart_send(msg);
                }
                else
                {
                    uart_send("Uso: CAL3 <T1C> <raw1> <T2C> <raw2> <T3C> <raw3>\r\n");
                }
            }
            // ----- Comando inválido -----
            else
            {
                uart_send("Comando no valido. Escribe HELP\r\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

// ---------------- ADC init (one-shot) ----------------
static adc_oneshot_unit_handle_t adc_init(void)
{
    adc_oneshot_unit_handle_t handle;
    adc_oneshot_unit_init_cfg_t cfg = {.unit_id = ADC_UNIT_1};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&cfg, &handle));

    adc_oneshot_chan_cfg_t chcfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_11, // ~3.3V full-scale
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, POT_CH, &chcfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, NTC_CH, &chcfg));
    return handle;
}

// ---------------- app_main ----------------
void app_main(void)
{
    init_uart();
    isr_install();

    shared_data_t ctx = {
        .thr_R = {0.0f, 15.0f},
        .thr_G = {10.0f, 30.0f},
        .thr_B = {40.0f, 50.0f},
        .pot_duty = 0,
        .mtx = xSemaphoreCreateMutex(),
        .ntc_log_ms = 400, // valor por defecto (ms)
    };
    ctx.adc = adc_init();
    ctx.adc_mtx = xSemaphoreCreateMutex();

    // --- LED RGB: usa los pines y modo que ya probaste en los tests ---
    ctx.led.CHANEL_R = LEDC_CHANNEL_0;
    ctx.led.CHANEL_G = LEDC_CHANNEL_1;
    ctx.led.CHANEL_B = LEDC_CHANNEL_2;

    ctx.led.PIN_R = 5;  // Rojo  (GPIO5)
    ctx.led.PIN_G = 18; // Verde (GPIO18)
    ctx.led.PIN_B = 19; // Azul  (GPIO19)

    ctx.led.Duty_R = ctx.led.Duty_G = ctx.led.Duty_B = 0;

    // Config general (coincide con el test que funcionó)
    ctx.led.common_anode = false; // cátodo común (activo-alto)
    ctx.led.SPEED_MODE = LEDC_LOW_SPEED_MODE;
    ctx.led.TIMER_R = LEDC_TIMER_0;
    ctx.led.TIMER_G = LEDC_TIMER_1;
    ctx.led.TIMER_B = LEDC_TIMER_2;
    ctx.led.RES = LEDC_TIMER_8_BIT;
    ctx.led.FREQ_HZ = 2000;

    configurar_led(&ctx.led);

    // --- NTC defaults (puedes ajustar por UART) ---
    ctx.ntc.model = NTC_MODEL_BETA;
    ctx.ntc.BETA = 4100.0f;
    ctx.ntc.R0 = 10000.0f;   // 10k @ 25°C
    ctx.ntc.T0 = 298.15f;    // 25°C
    ctx.ntc.Rfix = 10000.0f; // divisor
    ctx.ntc.a = ctx.ntc.b = ctx.ntc.c = 0.0f;

    // --- Inicializar botón con pull-up e interrupción por flanco de bajada ---
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_NEGEDGE // presiona -> LOW en BOOT
    };
    gpio_config(&io_conf);

    // Estado inicial del LED: habilitado
    ctx.led_enabled = true;
    ctx.led_enabled_changed = false;
    ctx.saved_r = ctx.saved_g = ctx.saved_b = ctx.saved_duty = 0;

    // Instalar servicio ISR y registrar handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr, (void *)&ctx);

    // --- TAREAS ---
    xTaskCreatePinnedToCore(task_pot, "POT", 4096, &ctx, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_ntc, "NTC", 4096, &ctx, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_uart, "UART", 4096, &ctx, 6, NULL, 0);

    while (1)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
