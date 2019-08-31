
#ifndef BT_IMPL_HPP
#define BT_IMP_HPP

#include "bt_iface.hpp"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s.h"

#include "sys/lock.h"



#include <memory>
#include <string>

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



namespace DispatchEvents {
	enum type : uint8_t {
		a2dEvent,
		avrcCtEvent,
		avrcTgEvent,
		btiStackup,
		failed,
	};
}



class BluetoothInterfaceBackservice
{
	/* Multi-module control with BaseMessage is done via the msg_queue it goes on top of */
	struct _BaseMessage
	{
		DispatchEvents::type	event;
		int						module_event;
		void*					module_argument_ptr;
	};

	/* Message itself has no members, only operates on top of BaseMessage members */
	template<
		DispatchEvents::type EventType,
		typename _ModuleEvent,
		typename _ModuleArgument>
	class _ClassMessage : public _BaseMessage
	{
	public:
		using ModuleEventType = _ModuleEvent;
		using ModuleArgumentType = _ModuleArgument;

		_ClassMessage(_BaseMessage b_msg)
		: _BaseMessage(b_msg)
		{
			Execute(&bti);
			if (b_msg.module_argument_ptr)
				free(b_msg.module_argument_ptr);
		}

		_ClassMessage()
		{
			this->event					= EventType;
			this->module_event			= 0;
			this->module_argument_ptr	= nullptr;
		}

		_ClassMessage(_ModuleArgument* arg)
		{
			// and this malloc is just wrong
			// sizeof pointer is fucking.... 1, or 4
			auto ptr = malloc(sizeof(arg));
			if (nullptr == ptr) {
				this->event = DispatchEvents::failed;
				return;
			}
			// should we copy our argument to its new location?
			// or maybe just swap pointers?
			// after all, it all comes and goes to same place/store, eh?
			// or even, swap against nullptr? hummm, interesting.. not so much. need to copy!!!
			this->event					= EventType;
			this->module_event			= 0;
			this->module_argument_ptr	= ptr;
		}

		_ClassMessage(_ModuleEvent evt, _ModuleArgument* arg)
		: _ClassMessage(arg)
		{
		  this->module_event = static_cast<int>(evt);
		}

		constexpr _ModuleEvent GetEvent() noexcept
		{
			return static_cast<_ModuleEvent>(this->module_event);
		}

		constexpr _ModuleArgument* GetArgument() noexcept
		{
			return static_cast<_ModuleArgument*>(this->module_argument_ptr);
		}
		void Execute(BluetoothInterface* btibi);
	};

	bool DispatchMessage(_BaseMessage& msg);

public:
	xQueueHandle msg_queue;
	xTaskHandle task_handle;
	std::unique_ptr<BluetoothInterface> target_instance_;

	static BluetoothInterfaceBackservice& GetInstanceRef() noexcept;
	void StartTask(BluetoothInterface* target_instance);
	void StopTask();

	static void EventRouter(void*);

	template<class MsgType, typename... _Args>
	bool Dispatch(_Args&&... _args)
	{
		auto msg = MsgType(std::forward<_Args>(_args)...);
		if (msg.event == DispatchEvents::failed)
			return false;
		return this->DispatchMessage(msg);
	}

	using btiStackupMsg		= _ClassMessage<DispatchEvents::btiStackup, int, void>;
	using A2DEventMsg		= _ClassMessage<DispatchEvents::a2dEvent, esp_a2d_cb_event_t, esp_a2d_cb_param_t>;
	using AVRCCtEventMsg	= _ClassMessage<DispatchEvents::avrcCtEvent, esp_avrc_ct_cb_event_t, esp_avrc_ct_cb_param_t>;
	using AVRCTgEventMsg	= _ClassMessage<DispatchEvents::avrcTgEvent, esp_avrc_tg_cb_event_t, esp_avrc_tg_cb_param_t>;
};


extern BluetoothInterfaceBackservice btibs;



#endif
