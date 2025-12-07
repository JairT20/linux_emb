# ESP32-S3 · Sensor Ultrasónico + MQTT + Control de LED

Este proyecto implementa un nodo IoT basado en **ESP32-S3**, capaz de:

- Leer un sensor ultrasónico (HC-SR04) cada 500 ms.
- Calcular un **promedio cada 10 segundos**.
- Publicar ese promedio mediante **MQTT**.
- Recibir comandos MQTT para **encender/apagar un LED**.
- Integrarse con **Node-RED** u otros clientes MQTT.

Todo el sistema se basa en **ESP-IDF + FreeRTOS**, con tareas independientes y colas para comunicar datos internamente.

---

#  ¿Qué hace el ESP32?

### 1. Conexión WiFi
El ESP32 se conecta en modo estación (STA) a una red WiFi.  
Cuando obtiene IP, continúa con la inicialización de MQTT.

### 2. Conexión al Broker MQTT
Una vez conectado, el ESP32:
- Publica su estado en `esp32/ultra/status = online`
- Se suscribe al tópico `esp32/ultra/led`

### 3. Lectura del sensor ultrasónico
Cada **500 ms**, una tarea dedicada:
- Dispara TRIG
- Espera el pulso de ECHO
- Calcula la distancia
- Envía la lectura a la cola `distance_queue`

### 4. Publicación del promedio
Cada **10 segundos**, otra tarea:
- Toma todos los valores acumulados en la cola
- Calcula el promedio
- Publica por MQTT en `esp32/ultra/distancia`

### 5. Control remoto del LED
Node-RED o cualquier otro cliente puede enviar:
- `"1"` para encender
- `"0"` para apagar
- cualquier valor → toggle

El comando viaja por MQTT → llega al handler → se encola → lo ejecuta la tarea LED.

---

#  Tópicos MQTT usados

| Tópico | Dirección | Descripción |
|-------|-----------|-------------|
| `esp32/ultra/status` | ESP32 → Broker | Publica `"online"` al conectarse |
| `esp32/ultra/distancia` | ESP32 → Broker | Publicación del promedio cada 10 s |
| `esp32/ultra/led` | Cliente → ESP32 | Control remoto del LED |

Ejemplo de publicación para encender el LED:

```bash
mosquitto_pub -t "esp32/ultra/led" -m "1"
```

Escuchar distancias:

```bash
mosquitto_sub -t "esp32/ultra/distancia" -v
```

---

# Diagrama de Software (ordenado y legible)

```mermaid
flowchart LR
    %% ============================================
    %% DIAGRAMA DE SOFTWARE - ESP32-S3 + WiFi + MQTT
    %% DISEÑO ORDENADO EN 3 COLUMNAS
    %% ============================================

    linkStyle default stroke-width:2.5px;

    %% -------- COLUMNAS --------
    %% Columna 1: Drivers
    subgraph DRV["Drivers ESP32-S3"]
        direction TB
        WIFI_DRV["wifi_driver.c<br/>wifi_init_sta()"]
        MQTT_DRV["mqtt_driver.c<br/>mqtt_app_start() + event_handler"]
        ULTRA_DRV["ultrasonic_driver.c<br/>lectura del sensor"]
        LED_DRV["led_driver.c<br/>control del LED"]
    end

    %% Columna 2: Tareas + colas
    subgraph TASKS["Tareas FreeRTOS + Colas"]
        direction TB
        T_ULTRA["ultrasonic_reader_task<br/>lee cada 500 ms"]
        Q_DIST["distance_queue<br/>cola de distancias"]
        T_MQTT["mqtt_avg_publisher_task<br/>promedia 10 s y publica"]
        Q_LED["led_cmd_queue<br/>cola de comandos LED"]
        T_LED["led_task<br/>maneja LED según comando"]
    end

    %% Columna 3: Red
    subgraph RED["Red WiFi + Broker + Cliente"]
        direction TB
        WIFI_AP["Punto de acceso WiFi"]
        MQTT_BROKER["Broker MQTT (Mosquitto)"]
        NODE_RED["Node-RED / Dashboard<br/>cliente MQTT"]
    end

    %% --------- CONEXIONES ---------

    %% Inicialización
    APP_MAIN["app_main.c<br/>Inicializa contexto, WiFi, MQTT, colas y tareas"]

    APP_MAIN --> WIFI_DRV
    APP_MAIN --> MQTT_DRV
    APP_MAIN --> ULTRA_DRV
    APP_MAIN --> LED_DRV

    APP_MAIN --> T_ULTRA
    APP_MAIN --> T_MQTT
    APP_MAIN --> T_LED

    APP_MAIN --> Q_DIST
    APP_MAIN --> Q_LED

    %% Sensor → Tarea → Cola
    ULTRA_DRV --> T_ULTRA
    T_ULTRA -->|xQueueSend(distancia)| Q_DIST
    Q_DIST --> T_MQTT

    %% Tarea MQTT → broker
    T_MQTT -->|publish TOPIC_DISTANCIA| MQTT_DRV

    %% WiFi → Broker
    WIFI_DRV --> WIFI_AP
    WIFI_AP --> MQTT_BROKER
    MQTT_DRV --> MQTT_BROKER

    %% LED desde MQTT
    NODE_RED -->|TOPIC_LED_CONTROL| MQTT_BROKER
    MQTT_BROKER -->|MQTT_EVENT_DATA| MQTT_DRV
    MQTT_DRV -->|cmd LED| Q_LED
    Q_LED --> T_LED
    T_LED --> LED_DRV

    %% Estado
    MQTT_DRV -->|publish TOPIC_STATUS = 'online'| MQTT_BROKER

    %% Distancia hacia Node-RED
    MQTT_BROKER -->|TOPIC_DISTANCIA| NODE_RED
```

---

#  ¿Cómo interactuar con el ESP32?

### 🔹 Encender y apagar LED desde consola
```bash
mosquitto_pub -t "esp32/ultra/led" -m "1"   # Encender LED
mosquitto_pub -t "esp32/ultra/led" -m "0"   # Apagar LED
```

### 🔹 Ver promedio de distancia en tiempo real
```bash
mosquitto_sub -t "esp32/ultra/distancia" -v
```

### 🔹 Integración con Node-RED
Puedes crear:

- Un **dashboard** con:
  - Gauge (distancia)
  - Gráfica
  - Tarjeta del valor actual
  - Botones ON/OFF para el LED

- Un **flow** que recibe:
  - `esp32/ultra/distancia` → muestra en dashboard
  - `esp32/ultra/led` → controla el LED

---

# Componentes internos (resumen técnico)

### Drivers
- `wifi_driver.c` → maneja conexión STA + eventos IP  
- `mqtt_driver.c` → cliente MQTT + parser de comandos  
- `ultrasonic_driver.c` → medición TRIG/ECHO  
- `led_driver.c` → manejo GPIO LED  

### Tareas FreeRTOS
- **ultrasonic_reader_task** → lectura periódica  
- **mqtt_avg_publisher_task** → promediado + publicación  
- **led_task** → control del LED vía cola  

### Colas
- `distance_queue` → flotantes con distancias  
- `led_cmd_queue` → comandos enumerados para LED  

---

#  Estado actual
Todo el sistema está modularizado, escalable y listo para agregarse:

- Más sensores  
- Más actuadores  
- Más tópicos MQTT  
- Lógica adicional en Node-RED  

---

Si quieres que también incluya:

 Diagrama de hardware  
 Imagen del HC-SR04  
 Capturas de Node-RED  
 Sección “Cómo compilar” (idf.py step-by-step)

solo dímelo y lo agrego al README.  


