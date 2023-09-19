#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <muse.h>
#define membersof(x) (sizeof(x) / sizeof(x[0]))

static const char* TAG = "muse";

static uint16_t MANUFACTURER_ID = 0xFFF0;
//6d b6 43 a0 fa 25 42 7c
#define MANUFACTURER_DATA_LENGTH 11
//#define MANUFACTURER_DATA_PREFIX 0x6D, 0xB6, 0x43, 0xCE, 0x97, 0xFE, 0x42, 0x7C //77 62 4d 53 45
#define MANUFACTURER_DATA_PREFIX 0x6D, 0xB6, 0x43, 0xA0, 0xFA, 0x25, 0x42, 0x7C //77 62 96 e5 33

static uint8_t _intensity_value = 0;
static uint8_t _last_set_intensity_value = 0;

static bool _stopping = false;

uint8_t manufacturerDataList[][MANUFACTURER_DATA_LENGTH] = {
    {MANUFACTURER_DATA_PREFIX, 0xE5, 0xAC, 0x26}, //E5AC26 0%
    {MANUFACTURER_DATA_PREFIX, 0xF4, 0xA4, 0x27}, //F4A427 17%
    {MANUFACTURER_DATA_PREFIX, 0xF7, 0x3F, 0x15}, //F73F15 25%
    {MANUFACTURER_DATA_PREFIX, 0xF6, 0xB6, 0x04}, //F6B604 33%
    {MANUFACTURER_DATA_PREFIX, 0xF1, 0x09, 0x70}, //F10970 42%
    {MANUFACTURER_DATA_PREFIX, 0xF0, 0x80, 0x61}, //F08061 50%
    {MANUFACTURER_DATA_PREFIX, 0xF3, 0x1B, 0x53}, //F31B53 58%
    {MANUFACTURER_DATA_PREFIX, 0xF2, 0x92, 0x42}, //F29242 66%
    {MANUFACTURER_DATA_PREFIX, 0xFD, 0x65, 0xBA}, //FD65BA 74%
    {MANUFACTURER_DATA_PREFIX, 0xFC, 0xEC, 0xAB}, //FCECAB 82%
    {MANUFACTURER_DATA_PREFIX, 0xC5, 0xAE, 0x07}  //C5AE07 100%
};

static uint8_t arrlen = membersof(manufacturerDataList)-1;

void set_manufacturer_data(uint8_t index) {
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

  pAdvertising->stop();

  uint8_t *manufacturerData = manufacturerDataList[index];
  pAdvertising->setManufacturerData(
    std::string(
      (char *)&MANUFACTURER_ID,
      2
    ) +
    std::string(
      (char *)manufacturerData,
      MANUFACTURER_DATA_LENGTH
    )
  );

  ESP_LOGD(TAG, "Manufacturer data has been set");

  pAdvertising->start();

  char buffer[256]; // Adjust the buffer size as needed
  int offset = 0;
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Advertising index: %d, data: ", index);

  for (int i = 0; i < 11; i++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%02X", manufacturerDataList[index][i]);
    if (i < 10) {
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, ", ");
    }
  }

  ESP_LOGD(TAG, "%s", buffer);
}

void muse_advertising_task(void *pvParameters) {
  ESP_LOGD(TAG, "Advertising task started");

  while (!_stopping) {
    // if (_last_set_intensity_value != _intensity_value) {
      set_manufacturer_data(_intensity_value);
      // _last_set_intensity_value = _intensity_value;
    // }
    delay(50);
  }
 
  // advertise stop all channels for a little while
  for (uint8_t i = 0; i < 10; i++) {
    set_manufacturer_data(0);
    delay(10);
  }
  vTaskDelete(NULL);
}

void muse_set_intensity(float intensity_percent) {
    // Convert the intensity percent to a value between 0 and 4
    _intensity_value = static_cast<uint8_t>(std::ceil(intensity_percent * arrlen));
    if (intensity_percent < 0.0) {
        // Limit the minimum result value to 0
        ESP_LOGW(TAG, "Intensity smaller than 0.0, received, cutting at 0.0");
        _intensity_value = 0;
    } else if (isnan(intensity_percent)) {
        ESP_LOGW(TAG, "Intensity NaN, received, cutting at 0.0");
        _intensity_value = 0;
    } else if (intensity_percent > 1.0) {
        ESP_LOGW(TAG, "Intensity larger than 1.0, received, cutting at 1.0.");
        // Limit the maximum result value to 3
        _intensity_value = arrlen;
    }

    ESP_LOGI(
      TAG,
      "Intensity %f received, advertising vibration: %d",
      intensity_percent,
      _intensity_value
    );
}

void muse_start() {
  ESP_LOGD(TAG, "Starting muse");

  _stopping = false;

  xTaskCreatePinnedToCore(muse_advertising_task, "muse_advertising_task", 4096, nullptr, 2, nullptr, 0);
}

void muse_stop() {
  ESP_LOGD(TAG, "Stopping muse");
  _stopping = true;
}

void muse_init() {
  ESP_LOGD(TAG, "Initializing muse");
}
