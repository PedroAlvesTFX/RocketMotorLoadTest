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

#define TRIGGER_GRAMS       15.0f

#define STOP_GRAMS          5.0f

#define STOP_TIME_MS        500

#define PRE_TRIGGER_SECONDS 2

// HX711

#define HX711_SCALE         104.0f

// ==============================
// Storage
// ==============================

// Reserva mantida após a gravação de cada ensaio.
#define STORAGE_MIN_FREE_BYTES  32768UL

// ==============================

#endif