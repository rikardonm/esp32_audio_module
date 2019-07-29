



extern "C" {
	// export symbols expected by C code
	extern void app_main();
};




#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/i2s.h"

#include <string>
#include <cctype>




#include "bt_iface.hpp"








/*
 * TODOS 4 system
 * - uart for configuring data
 *   - device name
 *   - device pin
 *   - enabled
 *   - output volume
 *   - other fx
 * - later on audio processing insitu
 * - the OLED interafce will be managed by the summer block
 *
 *
 *
 * TX/RX comm
 * 
 * actions: set & get values...?
 * nope, transmit events & queries
 * received queries and events, as these participate in the whole system infrastructure
 * so:
 * 	upward comms: from esp32 to summer
 * 	downward comms: from summer to esp32
 *
 *
 * upward events:
 * 	boot event
 * 	current configuration data
 * 	
 *
 * replies:
 * 	event ID executed successfully
 * 	event ID failed
 * 	event ID has no suitable receptor
 *
 *
 * downward events:
 * 	boot
 * 	set configuration data
 */



/* what var type is esp_bt_pin_type_t, is it char? */
struct Sinker {
public:
	Sinker(std::string name, std::string pin, float volume) :
			device_name(name), pin_code(pin), output_volume(volume) {
	}
	BluetoothInterface bti;
	std::string device_name;
	std::string pin_code;
	bool enabled = false;
	float output_volume;

	void Boot()
	{
		bti.Boot();
	}
};






Sinker sys("mbtt-0", "7864", 0);






void app_main()
{
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    i2s_config_t i2s_config = {
        .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),         // Only TX
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = 0,                                                  //Default interrupt priority
        .dma_buf_count = 6,
        .dma_buf_len = 60,
		.use_apll = false,
        .tx_desc_auto_clear = true,                                              //Auto clear tx descriptor on underflow
		.fixed_mclk = 0
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, nullptr);

    i2s_pin_config_t pin_config = {
        .bck_io_num = CONFIG_MYBTT_ESP32_AUDIO_MODULE_I2S_BLK_PIN,
        .ws_io_num = CONFIG_MYBTT_ESP32_AUDIO_MODULE_I2S_LCK_PIN,
        .data_out_num = CONFIG_MYBTT_ESP32_AUDIO_MODULE_I2S_DIN_PIN,
        .data_in_num = -1                                                       //Not used
    };

    i2s_set_pin(I2S_NUM_0, &pin_config);

    //bluetooth setup
    sys.Boot();
}

