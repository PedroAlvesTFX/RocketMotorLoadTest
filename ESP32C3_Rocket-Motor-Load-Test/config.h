#ifndef CONFIG_H
#define CONFIG_H

// ==============================
// Hardware
// ==============================

#define HX711_DOUT_PIN      13
#define HX711_SCK_PIN       12

// ==============================
// WiFi Access Point
// ==============================

#define AP_SSID             "RocketTestStand"
#define AP_PASSWORD         "rocket123"

// ==============================
// Acquisition
// ==============================

#define SAMPLE_RATE         80

#define MAX_TEST_TIME_MS    30000

// Durante a gravação, encerra quando a força permanecer neste limite
// ou abaixo dele. Leituras negativas também satisfazem esta condição.

#define DEFAULT_TRIGGER_GRAMS  15.0f
#define MIN_TRIGGER_GRAMS       0.5f
#define MAX_TRIGGER_GRAMS   10000.0f

#define STOP_GRAMS          10.0f

#define STOP_TIME_MS        250

// Continua gravando após a confirmação do fim da queima.
#define POST_TRIGGER_TIME_MS 2000

#define PRE_TRIGGER_SECONDS 2

// Capacidade física do buffer circular. O descarte é feito por tempo,
// portanto esta quantidade não define a duração do pré-trigger.
#define PRE_TRIGGER_BUFFER_SAMPLES 256

// HX711

#define HX711_SCALE         104.0f

// ==============================
// Storage
// ==============================

// Reserva mantida após a gravação de cada ensaio.
#define STORAGE_MIN_FREE_BYTES  32768UL

// ==============================

#endif