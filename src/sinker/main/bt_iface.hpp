/*
 * bt_interface.hpp
 *
 *  Created on: Jul 3, 2019
 *      Author: ricardo
 */

#ifndef BT_IFACE_HPP
#define BT_IFACE_HPP



#include <string>
#include <map>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


#include "esp_log.h"
#include "esp_err.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"



/*
 * Declare constructors as private members
 * Befriend BluetoothInterface to allow as regular members
 */

namespace BluetoothTag {
namespace A2D {
	namespace StateStrings {
	static const char *connection[] = {
			"Disconnected",
			"Connecting",
			"Connected",
			"Disconnecting"};
	static const char *audio[] = {
			"Suspended",
			"Stopped",
			"Started"};
	}
}
}

class BluetoothA2D
{
public:

	/*
	 * todo functions for registering callback when data is received
	 * or when data is requested too (?)
	 */
private:
	friend class BluetoothInterface;
	friend class BluetoothInterfaceBackservice;

	BluetoothA2D() = default;
	void Boot();

	static void EventState(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* a2d);
	static void EventDataSink(const uint8_t *data, uint32_t len);
	void WorkerState(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
};






namespace BluetoothTag {
namespace AVRCP {
	namespace TransacionLabel {
		enum type : uint8_t
		{
			getCaps 			= 0x00,
			getMetadata 		= 0x01,
			RNTrackChange		= 0x02,
			RNPlaybackChange	= 0x03,
			RNPlayPositionChange= 0x04
		};
	}
}
}

class BluetoothAVRCP
{
public:

	void SetVolume(uint8_t volume);
//	void VolumeUp(unsigned int change = 1);
//	void VolumeDown(unsigned int change = 1);
//	void Mute();
//	void RequestNextTrack();
//	void RequestPreviousTrack();
//	void RequestPlay();
//	void RequestPause();
//	void RequestPlayToogle();
//	void RequestStop();

	void Notification(esp_avrc_rn_event_ids_t event_id, esp_avrc_rn_param_t *event_parameter);

	static void EventController(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
	static void EventTarget(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
	void WorkerController(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t* rc);
	void WorkerTarget(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t* rc);
private:
	friend class BluetoothInterface;
	friend class BluetoothInterfaceBackservice;

	BluetoothAVRCP() = default;
	void Boot();

	void RequestMetadata();

	void EventNewTrack(decltype(esp_avrc_rn_param_t::elm_id) trackInfo);
	void EventPlayback(decltype(esp_avrc_rn_param_t::playback) playback);
	void EventPosition(decltype(esp_avrc_rn_param_t::play_pos) play_pos);
	void EventVolume(decltype(esp_avrc_rn_param_t::volume) volume);
	void EventBattery(decltype(esp_avrc_rn_param_t::batt) batt);

};



class BluetoothGAP
{
public:
	static void EventState(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

	void ConfigureSecurity();
	bool SetNewPinCode(std::string npin);
	bool ValidadePinCode(std::string npin);

private:
	friend class BluetoothInterface;
	friend class BluetoothInterfaceBackservice;
	BluetoothGAP() = default;
	void Boot();
};




class BluetoothInterface
{
public:
	BluetoothInterface()	= default;
	~BluetoothInterface()	= default;
	bool Connect()
	{
		return true;
	}
	bool Disconnect()
	{
		return false;
	}

//	bool ConnectLastPaired();
//	bool Scan();
//	bool Enable();
//	bool Disable();
//	void SetMode();
	void Boot();

	void SetName(const std::string& name);

	BluetoothGAP gap;
	BluetoothA2D a2d;
	BluetoothAVRCP avrcp;
private:
	friend class BluetoothInterfaceBackservice;

	void WorkerStackup();

	static bool booted_;
};



extern BluetoothInterface bti;


#endif /* BT_IFACE_HPP */
