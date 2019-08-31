

#include "bt_iface.hpp"
#include "bt_impl.hpp"

const char	logTag[]	= "btiGap";

void BluetoothGAP::ConfigureSecurity()
{
	// SSP
	/* Set default parameters for Secure Simple Pairing */
	esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
	esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
	esp_bt_gap_set_security_param(param_type,
								  static_cast<void*>(&iocap),
								  static_cast<uint8_t>(sizeof(uint8_t)));
	SetNewPinCode("FUCK;");
}


bool BluetoothGAP::SetNewPinCode(std::string npin) {
	if (ValidadePinCode(npin)) {
		esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
			esp_bt_pin_code_t pin_code;
			for (int i =0; i != npin.length(); ++i)
				pin_code[i] = npin.at(i);
			return esp_bt_gap_set_pin(pin_type,
									static_cast<uint8_t>(npin.length()),
									pin_code)
					== ESP_OK;
	}
	return false;
}


bool BluetoothGAP::ValidadePinCode(std::string npin) {
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


void BluetoothGAP::EventState(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
	ESP_LOGI(logTag, "GAP EventState for %d", event);
	switch (event)
	{
	case ESP_BT_GAP_AUTH_CMPL_EVT:
	{
		if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
			ESP_LOGI(logTag, "authentication success: %s",
					param->auth_cmpl.device_name);
			esp_log_buffer_hex(logTag, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
		}
		else {
			ESP_LOGE(logTag, "authentication failed, status:%d", param->auth_cmpl.stat);
		}
		break;
	}
	// SSP
	case ESP_BT_GAP_CFM_REQ_EVT:
		ESP_LOGI(logTag, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d",
				param->cfm_req.num_val);
		esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
		break;
	case ESP_BT_GAP_KEY_NOTIF_EVT:
		ESP_LOGI(logTag, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d",
				param->key_notif.passkey);
		break;
	case ESP_BT_GAP_KEY_REQ_EVT:
		ESP_LOGI(logTag, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
		break;
	default:
	{
		ESP_LOGI(logTag, "event: %d", event);
		break;
	}
	}
	return;
}


void BluetoothGAP::Boot()
{
	esp_bt_gap_register_callback(&BluetoothGAP::EventState);
}

