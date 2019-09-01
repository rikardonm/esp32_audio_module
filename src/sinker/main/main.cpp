



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

#include <string>
#include <cctype>




#include "bt_iface.hpp"
#include "i2s_iface.hpp"







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
			bti(BluetoothInterface::GetInstance()),
			device_name(name),
			pin_code(pin),
			output_volume(volume) {
	}
	BluetoothInterfacePtr bti;
	I2SInterface i2s;

	std::string device_name;
	std::string pin_code;
	bool enabled = false;
	float output_volume;

	void Boot();
};



Sinker sys("mbtt-0", "7864", 0);






void i2sSampleRate(int rate)
{
	sys.i2s.SetSampleRate(rate);
}

unsigned int i2sData(const uint8_t* data, uint32_t len)
{
	return sys.i2s.Write(data, len);
}



void Sinker::Boot()
{
	bti->Boot();
	i2s.Boot(I2S_NUM_0);
	i2s.ConfigurePins(CONFIG_MYBTT_ESP32_AUDIO_MODULE_I2S_BLK_PIN,
					  CONFIG_MYBTT_ESP32_AUDIO_MODULE_I2S_LCK_PIN,
					  -1,
					  CONFIG_MYBTT_ESP32_AUDIO_MODULE_I2S_DOUT_PIN);
	bti->a2d.SetCallbacks(i2sSampleRate, i2sData);
}


void app_main()
{
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    //bluetooth setup
    sys.Boot();
}

