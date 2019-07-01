
extern "C" {

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

	#include "esp_bt.h"
	#include "bt_app_core.h"
	#include "bt_app_av.h"
	#include "esp_bt_main.h"
	#include "esp_bt_device.h"
	#include "esp_gap_bt_api.h"
	#include "esp_a2dp_api.h"
	#include "esp_avrc_api.h"
	#include "driver/i2s.h"

	// export symbols expected by C code
	extern void app_main();
};

#include <string>
#include <cctype>



/* event for handler "bt_av_hdl_stack_up */
enum {
	BT_APP_EVT_STACK_UP = 0,
};



/* handler for bluetooth stack enabled events */
static void bt_av_hdl_stack_evt(uint16_t event, void *p_param);





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
 */


/* TX/RX comm
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
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */


class BluetoothInterface {
public:
	bool ValidadePinCode(std::string npin) {
		// check pin pre-requisites
		// 1 - length: 4-16 chars, non-null terminated
		// 2 - char types: 0-9
		if (npin.length() > 16 || npin.length() < 4)
			return false;
		for (auto& ch : npin) {
			if (!std::isdigit(ch)) {
				return false;
			}
		}
		return true;
	}

	bool SetNewPinCode(std::string npin) {
		if (ValidadePinCode(npin)) {
			esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
//			esp_bt_gap_set_pin(pin_type, npin.length(), static_cast<esp_bt_pin_code_t>(npin.c_str()));
			return true;
		}
		return false;
	}

	void Boot()
	{
		esp_err_t err;
	    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
	    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	    if ((err = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
	        ESP_LOGE(BT_AV_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(err));
	        return;
	    }
	    if ((err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
	        ESP_LOGE(BT_AV_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(err));
	        return;
	    }
	    if ((err = esp_bluedroid_init()) != ESP_OK) {
	        ESP_LOGE(BT_AV_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(err));
	        return;
	    }
	    if ((err = esp_bluedroid_enable()) != ESP_OK) {
	        ESP_LOGE(BT_AV_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(err));
	        return;
	    }
	    /* create application task */
	    bt_app_task_start_up();
	    /* Bluetooth device name, connection mode and profile set up */
	    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);
	    // SSP
	    /* Set default parameters for Secure Simple Pairing */
	    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
	    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
	    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
	    SetNewPinCode("FUCK;");
	}
};


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
        .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),                                  // Only TX
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

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(BT_AV_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(BT_AV_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(BT_AV_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }


    // SSP
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
    default: {
        ESP_LOGI(BT_AV_TAG, "event: %d", event);
        break;
    }
    }
    return;
}

static void bt_av_hdl_stack_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    switch (event) {
    case BT_APP_EVT_STACK_UP: {
        /* set up device name */

    	// todo name here
    	const char* dev_name = "FUCk";

    	esp_bt_dev_set_device_name(dev_name);

        esp_bt_gap_register_callback(bt_app_gap_cb);

        /* initialize AVRCP controller */
        esp_avrc_ct_init();
        esp_avrc_ct_register_callback(bt_app_rc_ct_cb);
        /* initialize AVRCP target */
        assert (esp_avrc_tg_init() == ESP_OK);
        esp_avrc_tg_register_callback(bt_app_rc_tg_cb);

        esp_avrc_rn_evt_cap_mask_t evt_set = {0};
        esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
        assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);

        /* initialize A2DP sink */
        esp_a2d_register_callback(&bt_app_a2d_cb);
        esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
        esp_a2d_sink_init();

        /* set discoverable and connectable mode, wait to be connected */
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        break;
    }
    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}
