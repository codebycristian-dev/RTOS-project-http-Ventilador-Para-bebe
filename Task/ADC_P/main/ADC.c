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

#include "nvs_flash.h"
#include "nvs.h"

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
    // Steinhart–Hart (no usamos en CALRFIX, pero se conserva en struct)
    float a, b, c; // 1/T = a + b lnR + c (lnR)^3
    // Hardware
    float Rfix; // Resistencia del divisor (ohm)
} ntc_params_t;

// ---------------- Persistencia en NVS ----------------
#define NVS_NS "cal"
#define NVS_KEY "ntc"
#define NTC_PERSIST_VER 1

typedef struct
{
    uint32_t ver;
    ntc_params_t ntc;
} ntc_persist_t;

static esp_err_t ntc_nvs_save(const ntc_params_t *p)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK)
        return err;
    ntc_persist_t s = {.ver = NTC_PERSIST_VER, .ntc = *p};
    err = nvs_set_blob(h, NVS_KEY, &s, sizeof(s));
    if (err == ESP_OK)
        err = nvs_commit(h);
    nvs_close(h);
    return err;
}

static esp_err_t ntc_nvs_load(ntc_params_t *p)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK)
        return err;
    ntc_persist_t s;
    size_t sz = sizeof(s);
    err = nvs_get_blob(h, NVS_KEY, &s, &sz);
    nvs_close(h);
    if (err == ESP_OK && sz == sizeof(s) && s.ver == NTC_PERSIST_VER)
    {
        *p = s.ntc;
        return ESP_OK;
    }
    return err;
}

static esp_err_t ntc_nvs_erase(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK)
        return err;
    err = nvs_erase_key(h, NVS_KEY);
    if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND)
        err = nvs_commit(h);
    nvs_close(h);
    return err;
}

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

// --------- Helpers NTC: ADC→Vout, Vout→Rntc, modelos ----------
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

static inline float tempK_from_beta(float R, float BETA, float R0, float T0K)
{
    return 1.0f / ((1.0f / T0K) + (1.0f / BETA) * logf(R / R0));
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

    static uint32_t last_isr_tick = 0;
    uint32_t now = xTaskGetTickCountFromISR();
    if ((now - last_isr_tick) < pdMS_TO_TICKS(200))
        return;
    last_isr_tick = now;

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

        float Tk = 298.15f;
        if (ctx->ntc.model == NTC_MODEL_BETA)
            Tk = tempK_from_beta(Rntc, ctx->ntc.BETA, ctx->ntc.R0, ctx->ntc.T0);
        else
            Tk = tempK_from_sh(Rntc, ctx->ntc.a, ctx->ntc.b, ctx->ntc.c);
        float Tc = Tk - 273.15f;

        uint16_t duty_local = 0;
        bool r = false, g = false, b = false;
        uint32_t log_ms_local = 400;

        if (xSemaphoreTake(ctx->mtx, portMAX_DELAY))
        {
            duty_local = ctx->pot_duty;
            color_mix_from_temp(ctx, Tc, &r, &g, &b);
            log_ms_local = ctx->ntc_log_ms;
            xSemaphoreGive(ctx->mtx);
        }

        uint8_t new_r = (uint8_t)(r ? duty_local : 0);
        uint8_t new_g = (uint8_t)(g ? duty_local : 0);
        uint8_t new_b = (uint8_t)(b ? duty_local : 0);

        bool enabled = ctx->led_enabled;

        if (ctx->led_enabled_changed)
        {
            if (!enabled)
            {
                ctx->saved_r = new_r;
                ctx->saved_g = new_g;
                ctx->saved_b = new_b;
                ctx->saved_duty = (uint8_t)duty_local;
                led_rgb_write(&ctx->led, 0, 0, 0);
                ESP_LOGI("BTN", "LED OFF (saved R=%u G=%u B=%u duty=%u)",
                         ctx->saved_r, ctx->saved_g, ctx->saved_b, ctx->saved_duty);
            }
            else
            {
                led_rgb_write(&ctx->led, ctx->saved_r, ctx->saved_g, ctx->saved_b);
                ESP_LOGI("BTN", "LED RESTORE (R=%u G=%u B=%u duty=%u)",
                         ctx->saved_r, ctx->saved_g, ctx->saved_b, ctx->saved_duty);
            }
            ctx->led_enabled_changed = false;
        }
        else
        {
            if (enabled)
                led_rgb_write(&ctx->led, new_r, new_g, new_b);
        }

        ESP_LOGI("NTC", "T=%.2f°C -> R=%d G=%d B=%d Duty=%u Enabled=%d",
                 Tc, r, g, b, duty_local, enabled);

        vTaskDelay(pdMS_TO_TICKS(log_ms_local));
    }
}

// ---------------- Tarea UART ----------------
static void task_uart(void *pvParameters)
{
    shared_data_t *ctx = (shared_data_t *)pvParameters;
    char buf[128];

    uart_send("\r\nUART listo. Comandos:\r\n");
    uart_send("  TPERIOD [ms]      -> fija o consulta periodo de impresión de temperatura\r\n");
    uart_send("  POTV              -> lee voltaje del potenciómetro\r\n");
    uart_send("  NTC?              -> consulta parámetros de modelo NTC\r\n");
    uart_send("  NTCRAW            -> lee raw instantáneo del NTC\r\n");
    uart_send("  SETNTC BETA <val> | SETNTC R0 <ohm> | SETNTC RFIX <ohm>\r\n");
    uart_send("  CALRFIX <T_C> [raw]  (calibra RFIX y guarda en NVS)\r\n");
    uart_send("  SAVENTC | LOADNTC | NTCRESET\r\n");

    while (1)
    {
        if (uart_available(buf, sizeof(buf)))
        {
            for (char *p = buf; *p; ++p)
                if (*p == '\r' || *p == '\n' || *p == '\t')
                    *p = ' ';

            float f1 = 0.0f;

            if (strncasecmp(buf, "TPERIOD", 7) == 0)
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
            else if (strncasecmp(buf, "NTC?", 4) == 0)
            {
                char msg[240];
                snprintf(msg, sizeof(msg),
                         "NTC model=%s | BETA=%.2f R0=%.1f T0=%.2fK | a=%.6e b=%.6e c=%.6e | Rfix=%.1f\r\n",
                         (ctx->ntc.model == NTC_MODEL_BETA ? "BETA" : "SH"),
                         ctx->ntc.BETA, ctx->ntc.R0, ctx->ntc.T0,
                         ctx->ntc.a, ctx->ntc.b, ctx->ntc.c, ctx->ntc.Rfix);
                uart_send(msg);
            }
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
            // ----- CALRFIX (1 punto, ajusta RFIX y guarda en NVS) -----
            else if (strncasecmp(buf, "CALRFIX", 7) == 0)
            {
                // Uso: CALRFIX <T_C> [raw]
                float Tc_user;
                int raw_opt = -1;
                if (sscanf(buf, "CALRFIX %f %d", &Tc_user, &raw_opt) >= 1)
                {
                    int raw = raw_opt;
                    if (raw < 0)
                    {
                        if (xSemaphoreTake(ctx->adc_mtx, portMAX_DELAY))
                        {
                            raw = adc_average(ctx->adc, NTC_CH, NUM_SAMPLES);
                            xSemaphoreGive(ctx->adc_mtx);
                        }
                    }
                    if (raw <= 0 || raw >= ADC_MAX)
                    {
                        uart_send("CALRFIX fallo: raw fuera de rango\r\n");
                    }
                    else if (ctx->ntc.model != NTC_MODEL_BETA)
                    {
                        uart_send("CALRFIX requiere MODEL BETA\r\n");
                    }
                    else
                    {
                        float Tk = Tc_user + 273.15f;
                        float Rntc_expected = ctx->ntc.R0 * expf(ctx->ntc.BETA * ((1.0f / Tk) - (1.0f / ctx->ntc.T0)));
                        float Rfix_new = Rntc_expected * ((float)(ADC_MAX - raw) / (float)raw);
                        ctx->ntc.Rfix = Rfix_new;

                        char msg[160];
                        snprintf(msg, sizeof(msg),
                                 "OK CALRFIX -> RFIX=%.1f ohm (raw=%d, T=%.2fC)\r\n",
                                 Rfix_new, raw, Tc_user);
                        uart_send(msg);

                        esp_err_t se = ntc_nvs_save(&ctx->ntc);
                        if (se == ESP_OK)
                            uart_send("   (guardado en NVS)\r\n");
                        else
                            uart_send("   (No se pudo guardar en NVS)\r\n");
                    }
                }
                else
                {
                    uart_send("Uso: CALRFIX <T_C> [raw]\r\n");
                }
            }
            // ----- Persistencia manual -----
            else if (strncasecmp(buf, "SAVENTC", 7) == 0)
            {
                esp_err_t se = ntc_nvs_save(&ctx->ntc);
                if (se == ESP_OK)
                    uart_send("OK SAVENTC (guardado en NVS)\r\n");
                else
                    uart_send("SAVENTC fallo\r\n");
            }
            else if (strncasecmp(buf, "LOADNTC", 7) == 0)
            {
                ntc_params_t tmp;
                if (ntc_nvs_load(&tmp) == ESP_OK)
                {
                    ctx->ntc = tmp;
                    uart_send("OK LOADNTC (cargado de NVS)\r\n");
                }
                else
                {
                    uart_send("LOADNTC: no hay datos en NVS\r\n");
                }
            }
            else if (strncasecmp(buf, "NTCRESET", 8) == 0)
            {
                if (ntc_nvs_erase() == ESP_OK)
                    uart_send("OK NTCRESET (borrado NVS)\r\n");
                else
                    uart_send("NTCRESET fallo\r\n");
            }
            else
            {
                uart_send("Comando no valido. Escribe: TPERIOD | POTV | NTC? | NTCRAW | SETNTC | CALRFIX | SAVENTC | LOADNTC | NTCRESET\r\n");
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

    // --- LED RGB ---
    ctx.led.CHANEL_R = LEDC_CHANNEL_0;
    ctx.led.CHANEL_G = LEDC_CHANNEL_1;
    ctx.led.CHANEL_B = LEDC_CHANNEL_2;

    ctx.led.PIN_R = 5;  // Rojo  (GPIO5)
    ctx.led.PIN_G = 18; // Verde (GPIO18)
    ctx.led.PIN_B = 19; // Azul  (GPIO19)

    ctx.led.Duty_R = ctx.led.Duty_G = ctx.led.Duty_B = 0;

    ctx.led.common_anode = false; // cátodo común (activo-alto)
    ctx.led.SPEED_MODE = LEDC_LOW_SPEED_MODE;
    ctx.led.TIMER_R = LEDC_TIMER_0;
    ctx.led.TIMER_G = LEDC_TIMER_1;
    ctx.led.TIMER_B = LEDC_TIMER_2;
    ctx.led.RES = LEDC_TIMER_8_BIT;
    ctx.led.FREQ_HZ = 2000;

    configurar_led(&ctx.led);

    // --- Inicializa NVS y carga calibración previa ---
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    bool ntc_loaded = (ntc_nvs_load(&ctx.ntc) == ESP_OK);
    if (ntc_loaded)
    {
        ESP_LOGI(TAG, "NTC params loaded from NVS (RFIX=%.1f, model=%d, BETA=%.1f, R0=%.1f)",
                 ctx.ntc.Rfix, (int)ctx.ntc.model, ctx.ntc.BETA, ctx.ntc.R0);
    }
    if (!ntc_loaded)
    {
        ctx.ntc.model = NTC_MODEL_BETA;
        ctx.ntc.BETA = 4100.0f;
        ctx.ntc.R0 = 10000.0f;   // 10k @ 25°C
        ctx.ntc.T0 = 298.15f;    // 25°C
        ctx.ntc.Rfix = 10000.0f; // divisor
        ctx.ntc.a = ctx.ntc.b = ctx.ntc.c = 0.0f;
    }

    // --- Botón con pull-up e interrupción por flanco de bajada ---
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_NEGEDGE};
    gpio_config(&io_conf);

    // Estado inicial del LED: habilitado
    ctx.led_enabled = true;
    ctx.led_enabled_changed = false;
    ctx.saved_r = ctx.saved_g = ctx.saved_b = ctx.saved_duty = 0;

    // ISR
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr, (void *)&ctx);

    // --- TAREAS ---
    xTaskCreatePinnedToCore(task_pot, "POT", 4096, &ctx, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_ntc, "NTC", 4096, &ctx, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_uart, "UART", 4096, &ctx, 6, NULL, 0);

    while (1)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
