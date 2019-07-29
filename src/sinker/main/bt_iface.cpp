/*
 * bt_audio_iface.cpp
 *
 *  Created on: Jul 4, 2019
 *      Author: ricardo
 */

#include "bt_iface.hpp"


#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"





/**
 * Declare and use callbacks on the C compatibility space
 */
extern "C" {

	/* Handler for bt task message handling */
	void bti_task_handler(void* /*arg*/);

	/* callback function for A2DP sink */
	void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

	/* callback function for A2DP sink audio data stream */
	void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len);

	/* callback function for AVRCP controller */
	void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);

	/* callback function for AVRCP target */
	void bt_app_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);

	/* callback function for GAP */
	void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param);

}







/* Multi-module control with BaseMessage is done via the msg_queue it goes on top of */
template<typename DispatchEventType>
struct _BaseMessage {
	DispatchEventType		event;		/*!< signal to bt_app_task (which member function) */
	int						evt_of_evt;	/*!< event of event */
	void*					argument;	/*!< parameter area needs to be last */
	_BaseMessage(DispatchEventType event, void* arg = nullptr)
	: event(event),
	  evt_of_evt(0),
	  argument(arg)
	{}

	// create from types
	template<typename T>
	_BaseMessage(DispatchEventType event, T* arg = nullptr)
	: event(event),
	  evt_of_evt(0),
	  argument(static_cast<void*>(arg))
	{}

	template<typename T>
	_BaseMessage(DispatchEventType event, T eoe, void* arg = nullptr)
	: event(event),
	  evt_of_evt(static_cast<int>(eoe)),
	  argument(arg)
	{}

};













/* handler for the dispatched work */
typedef void (* bt_app_cb_t) (uint16_t event, void *param);


/* parameter deep-copy function to be customized */
typedef void (* bt_app_copy_cb_t) (bt_app_msg_t *msg, void *p_dest, void *p_src);









uint32_t s_pkt_cnt = 0;
esp_a2d_audio_state_t s_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;


esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
_lock_t s_volume_lock;
uint8_t s_volume = 0;
bool s_volume_notify;















class BluetoothInterfaceBackservice {
public:

	static const char* Log = "BTi";

	namespace AVRCPTransacionLabel {
		enum type : uint8_t
		{
			getCaps 			= 0x00,
			getMetadata 		= 0x01,
			RNTrackChange		= 0x02,
			RNPlaybackChange	= 0x03,
			RNPlayPositionChange= 0x04
		};
	}

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





	void StartTask();
	xQueueHandle msg_queue;
	xTaskHandle task_handle;
	void StopTask();
	void A2dHdlAvEvent(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *a2d);
	void StackHdlAvEvent();





	namespace DispatchEvents {
		enum type : uint8_t {
			a2dEvent			= 0x01,
			avHdlAvrcCtEvent,
			avHdlAvrcTgEvent,
			avHdlStackEvent,
			failed,
		};
	}




	using BaseMessage = _BaseMessage<DispatchEvents::type>;


	/* Message itself has no members, only operates on top of BaseMessage members */
	template<
		DispatchEvents::type Type,
		typename Argument = void,
		typename EventOfEvent = int>
	class _ClassMessage : BaseMessage
	{
	public:
		// convert argument and execute corresponding member function
		_ClassMessage(BaseMessage b_msg, BluetoothInterfaceBackservice* instance)
		: BaseMessage(b_msg)
		{
			Execute(instance);
			free(argument);
		}

		// create from type, for all types except void: allocate
		_ClassMessage(Argument arg)
		{
			Argument* ptr = malloc(sizeof(arg));
			if (nullptr == ptr) {
				event = DispatchEvents::failed;
				return;
			}
			*this = BaseMessage(Type, ptr);
		}

		_ClassMessage(Argument arg, EventOfEvent eoe)
		{
			_ClassMessage(argument);
			this->evt_of_evt = eoe;
		}

		Argument* GetArgument() const
		{
			return static_cast<Argument*>(argument);
		}

		EventOfEvent GetEventOfEvent() const
		{
			return static_cast<EventOfEvent>(evt_of_evt);
		}

		// execute callback
		void Execute(BluetoothInterfaceBackservice* btibi);
	};






	using A2DEventMsg = _ClassMessage<DispatchEvents::a2dEvent, esp_a2d_cb_param_t, esp_a2d_cb_event_t>;
	template<>
	void A2DEventMsg::Execute(BluetoothInterfaceBackservice* btibi)
	{
		btibi->A2dHdlAvEvent(GetEventOfEvent(), GetArgument());
	}

	using StackHdlAvMsg = _ClassMessage<DispatchEvents::avHdlStackEvent>;
	template<>
	void StackHdlAvMsg::Execute(BluetoothInterfaceBackservice* btibi)
	{
		btibi->StackHdlAvEvent();
	}

	using AvrHdlEventMsg = _ClassMessage<DispatchEvents::avHdlAvrcCtEvent>;
	template<>
	void AvrHdlEventMsg::Execute(BluetoothInterfaceBackservice* btibi)
	{
		btibi->
	}

	bool DispatchMessage(BaseMessage& msg)
	{
	    if (xQueueSend(msg_queue, &msg, 10 / portTICK_RATE_MS) != pdTRUE) {
	        ESP_LOGE(BluetoothInterfaceBackservice::Log, "%s xQueue send failed", __func__);
	        return false;
	    }
	    return true;
	}





	void callback_Message()
	{
		BaseMessage msg;
	    while (true) {
	        if (pdTRUE == xQueueReceive(msg_queue, &msg, (portTickType)portMAX_DELAY)) {
	        	switch(msg.event) {
	        	case DispatchEvents::a2dEvent:			A2DEventMsg(msg, this);	break;
	        	case DispatchEvents::avHdlStackEvent:	StackHdlAvMsg(msg, this); break;
	        	}
	        }
	    }
	}






	void bt_app_alloc_meta_buffer(esp_avrc_ct_cb_param_t *param)
	{
	    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(param);
	    uint8_t *attr_text = (uint8_t *) malloc (rc->meta_rsp.attr_length + 1);	// allocate space for string
	    memcpy(attr_text, rc->meta_rsp.attr_text, rc->meta_rsp.attr_length);	// copy string
	    attr_text[rc->meta_rsp.attr_length] = 0;	// null terminate string

	    rc->meta_rsp.attr_text = attr_text;	// copy pointer of response for copied string
	}


	/*
	 ****************************************************************************************
	 ****************************************************************************************
	 ****************                                            ****************************
	 ****************        Callbacks from IDF API              ****************************
	 ****************                                            ****************************
	 ****************************************************************************************
	 ****************************************************************************************
	 */




	// called to request stack bring up
	// called from main to worker thread
	void StackHdlAvEvent()
	{
	    /* set up device name */
		// todo name here
		const char* dev_name = "FUCk";

		esp_bt_dev_set_device_name(dev_name);

		esp_bt_gap_register_callback(bt_app_gap_cb);

		// FIXME XXX SPLIT HERE

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
	}

};












/* delcare global variable for interface */
BluetoothInterface bti;
BluetoothInterfaceBackservice bti_back;



void bti_task_handler(void* /*arg*/)
{
	bti_back.callback_Message();
}

void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{

}

void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{

}

void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{

}

void bt_app_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
	bti_back.callback_avrcTarget(event, param);
}

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param)
{
	bti_back.callback_GAP(event, param);
}














bool BluetoothInterface::ValidadePinCode(std::string npin) {
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


bool BluetoothInterface::SetNewPinCode(std::string npin) {
	if (ValidadePinCode(npin)) {
		esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
//			esp_bt_gap_set_pin(pin_type, npin.length(), static_cast<esp_bt_pin_code_t>(npin.c_str()));
		return true;
	}
	return false;
}

void BluetoothInterface::Boot()
{
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

	if (auto err = esp_bt_controller_init(&bt_cfg) != ESP_OK) {
		ESP_LOGE(BluetoothInterfaceBackservice::Log, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(err));
		return;
	}
	if (auto err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK) {
		ESP_LOGE(BluetoothInterfaceBackservice::Log, "%s enable controller failed: %s\n", __func__, esp_err_to_name(err));
		return;
	}
	if (auto err = esp_bluedroid_init() != ESP_OK) {
		ESP_LOGE(BluetoothInterfaceBackservice::Log, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(err));
		return;
	}
	if (auto err = esp_bluedroid_enable() != ESP_OK) {
		ESP_LOGE(BluetoothInterfaceBackservice::Log, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(err));
		return;
	}
	/* create application task */
	bti_back.StartTask();

	/* Bluetooth device name, connection mode and profile set up */
	bt_app_work_dispatch(BTI___bt_av_hdl_stack_evt, BluetoothInterfaceBackservice::Events::StackUp, nullptr, 0, nullptr);
	bti_back.StackHdlAvMsg
	// SSP
	/* Set default parameters for Secure Simple Pairing */
	esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
	esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
	esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
	SetNewPinCode("FUCK;");
}



















void BluetoothInterfaceBackservice::StartTask(void)
{
    msg_queue = xQueueCreate(10, static_cast<unsigned int>(sizeof(BaseMessage)));
    xTaskCreate(bti_task_handler, "BtTask", 3072, nullptr, configMAX_PRIORITIES - 3, &task_handle);
}

void BluetoothInterfaceBackservice::StopTask(void)
{
	if (task_handle) {
		vTaskDelete(task_handle);
		task_handle = nullptr;
	}
	if (msg_queue) {
		vQueueDelete(msg_queue);
		msg_queue = nullptr;
	}
}







































