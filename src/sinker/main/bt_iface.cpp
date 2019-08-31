/*
 * bt_audio_iface.cpp
 *
 *  Created on: Jul 4, 2019
 *      Author: ricardo
 */

#include "bt_iface.hpp"
#include "bt_impl.hpp"


BluetoothInterface bti;

bool BluetoothInterface::booted_ = 0;

const char logTag[]		= "btiIface";

void BluetoothInterface::Boot()
{
	if (booted_)
		return;
	booted_ = true;
	ESP_LOGI(logTag, "Booting ...");
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

	if (auto err = esp_bt_controller_init(&bt_cfg) != ESP_OK) {
		ESP_LOGE(logTag, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(err));
		return;
	}
	if (auto err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK) {
		ESP_LOGE(logTag, "%s enable controller failed: %s\n", __func__, esp_err_to_name(err));
		return;
	}
	if (auto err = esp_bluedroid_init() != ESP_OK) {
		ESP_LOGE(logTag, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(err));
		return;
	}
	if (auto err = esp_bluedroid_enable() != ESP_OK) {
		ESP_LOGE(logTag, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(err));
		return;
	}
	/* create application task */
	BluetoothInterfaceBackservice::GetInstanceRef().StartTask(this);
	gap.ConfigureSecurity();
	/* Bluetooth device name, connection mode and profile set up */
	btibs.Dispatch<BluetoothInterfaceBackservice::btiStackupMsg>();
}


void BluetoothInterface::SetName(const std::string& name)
{
	esp_bt_dev_set_device_name(name.c_str());
}


// called to request stack bring up
// called from main to worker thread
void BluetoothInterface::WorkerStackup()
{
	ESP_LOGI(logTag, "Executing stack-up event");
	bti.SetName("FUCK");
	bti.gap.Boot();
	bti.avrcp.Boot();
	bti.a2d.Boot();
	/* set discoverable and connectable mode, wait to be connected */
	esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
}






