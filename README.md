# 🌬️ Ventilador Inteligente con ESP32 + RTOS + Interfaz Web

Este proyecto implementa un **sistema de ventilación inteligente** basado en un ESP32, utilizando **FreeRTOS**, control PWM, sensores ambientales y una interfaz web completamente interactiva.  

Permite controlar manualmente el ventilador, usar un modo automático por temperatura y un modo programado con hasta **3 registros horarios**, todos configurables desde un panel web moderno.

En el siguiente Repositorio en la carpeta http se puede observar los archivos del proyecto principal a cabo en las demás carpetas tambien se puede encontrar archivos con bases interesantes para RTOS en esp32

---

## 🚀 Características principales

- ✔ Control por **PWM de alta frecuencia** (25 kHz)
- ✔ Sensor de temperatura **DS18B20**
- ✔ Sensor de presencia **PIR HC-SR501**
- ✔ 3 modos de trabajo:
  - **Manual** → el usuario fija el PWM
  - **Automático** → PWM depende de Tmin/Tmax
  - **Programado** → hasta 3 horarios con días, temperaturas y activación
- ✔ Prevención de **solapamiento de horarios**
- ✔ API REST interna (JSON)
- ✔ Interfaz web estilo iOS completamente responsiva
- ✔ Actualización OTA del firmware
- ✔ Lógica modular separada por tareas RTOS

---

# 📦 Hardware utilizado

| Componente | Función |
|-----------|---------|
| **ESP32 DevKit V1** | Microcontrolador principal |
| **DS18B20** | Lectura de temperatura |
| **PIR HC-SR501** | Detección de presencia |
| **Ventilador PWM / LED de simulación** | Actuador |
| **Fuente + cableado** | Interconexión |

---

# 🧠 Arquitectura del Firmware

El firmware está dividido en **módulos independientes**, cada uno responsable de una parte del sistema:

| Archivo | Descripción |
|--------|-------------|
| `sensor_app.c` | Lee temperatura, presencia y maneja fallos del DS18B20 |
| `logic_app.c` | Calcula PWM según el modo (manual, auto, programado) |
| `fan_control.c` | Configura el PWM (LEDC) y aplica el duty cycle |
| `config_app.c` | Gestiona configuración persistente en NVS |
| `http_server.c` | Maneja servidor web + API REST |
| `wifi_app.c` | Conexión WiFi + estado + tiempo NTP |
| `main.c` | Inicialización general + arranque de tareas |

---

# 🧵 Tareas FreeRTOS (RTOS)

El sistema utiliza varias tareas que se ejecutan en paralelo:

### **1️⃣ sensor_task**
- Lee temperatura del DS18B20  
- Lee presencia del PIR  
- Maneja error del sensor (usa último valor válido)  

### **2️⃣ logic_task**
- Aplica la lógica principal del ventilador  
- Evalúa modo manual, auto y programado  
- Detecta coincidencias de horario/días  
- Calcula el PWM final  

### **3️⃣ Servidor Web / WiFi**
- Maneja peticiones HTTP  
- Procesa POST/GET de la API  
- Envía estado en JSON al frontend  

---

# 🧩 Modos de funcionamiento

## 🔧 Modo Manual
El usuario ajusta el slider → se envía a `/fan/set_manual_pwm.json`

## 🤖 Modo Automático
Usa dos límites configurables:
- **Tmin ➜ 0% PWM**
- **Tmax ➜ 100% PWM**

Mapeo lineal:


## 📆 Modo Programado
Hasta 3 registros:

Cada uno contiene:
- Activado / desactivado
- Hora inicio – Hora fin  
- Días de la semana (bitmask)
- Temp0 → PWM = 0%
- Temp100 → PWM = 100%

✔ Se evita automáticamente crear **horarios solapados** (respuesta HTTP 409).  

---
## 🧵 Tareas FreeRTOS
1️⃣ sensor_task

Lee temperatura del DS18B20 cada 1s

Detecta errores de lectura

Mantiene último valor válido

Lee presencia del PIR

2️⃣ logic_task

Determina el modo actual

Evalúa:

Manual → PWM directo

Automático → mapeo lineal

Programado → coincidencia de horario y día

Aplica PWM mediante fan_set_pwm()

Corre cada 1 segundo

3️⃣ http_server_task

Atiende peticiones REST

Sirve archivos HTML + CSS + JS

Maneja OTA

Responde estado del sistema en JSON

## 🌐 API REST del sistema
GET /fan/get_state.json
🔹 Obtener estado del ventilador
GET /fan/get_state.json


Respuesta:

{
  "temperature": 21.7,
  "presence": 1,
  "pwm": 70,
  "mode": 2,
  "Tmin": 22.0,
  "Tmax": 30.0
}
---


## 🔹 Cambiar modo
POST /fan/set_mode.json
Body: "0" | "1" | "2"
---
## 🔹 Guardar PWM manual
POST /fan/set_manual_pwm.json
Body: "45"
---
## 🔹 Guardar configuración automática
POST /fan/set_auto.json
{
  "Tmin": 22.0,
  "Tmax": 30.0
}
---
## 🔹 Obtener registro programado
GET /fan/get_register.json?id=1
---
## 🔹 Guardar registro
POST /fan/set_register.json
{
  "id": 1,
  "active": 1,
  "hour_start": 1,
  "min_start": 0,
  "hour_end": 2,
  "min_end": 0,
  "temp0": 22.0,
  "temp100": 28.0,
  "days": 4
}

🧩 Estructura del proyecto
/main
    main.c
    logic_app.c
    sensor_app.c
    fan_control.c
    config_app.c
    wifi_app.c
    http_server.c

/frontend
    index.html
    app.css
    app.js

README.md

🛠️ Cómo compilar y ejecutar
Compilar
idf.py build

Flashear
idf.py flash

Monitorear
idf.py monitor

Abrir la interfaz web
http://<IP-del-ESP32>/
