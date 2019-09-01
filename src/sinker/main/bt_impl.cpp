

#include "bt_impl.hpp"

BluetoothInterfaceBackservice btibs;

const char logTag[]		= "BTi";


BluetoothInterfaceBackservice& BluetoothInterfaceBackservice::GetInstanceRef() noexcept
{
	return btibs;
}

bool BluetoothInterfaceBackservice::DispatchMessage(_BaseMessage& msg)
{
	if (xQueueSend(msg_queue, &msg, 10 / portTICK_RATE_MS) != pdTRUE) {
		ESP_LOGE(logTag, "%s xQueue send failed", __func__);
		return false;
	}
	return true;
}

void BluetoothInterfaceBackservice::EventRouter(void* /*freertos_argument*/)
{
	_BaseMessage msg;
	while (true) {
		if (pdTRUE == xQueueReceive(btibs.msg_queue, &msg, (portTickType)portMAX_DELAY)) {
			switch(msg.event) {
			case DispatchEvents::a2dEvent: {
				volatile auto x = A2DEventMsg(msg);
				break;
			}
			case DispatchEvents::btiStackup: {
				volatile auto x = btiStackupMsg(msg);
				break;
			}
			case DispatchEvents::avrcCtEvent: {
				volatile auto x = AVRCCtEventMsg(msg);
				break;
			}
			case DispatchEvents::avrcTgEvent: {
				volatile auto x = AVRCTgEventMsg(msg);
				break;
			}
			default:
				break;
			}
		}
	}
}

void BluetoothInterfaceBackservice::StartTask(BluetoothInterface* target_instance)
{
	msg_queue = xQueueCreate(10, static_cast<unsigned int>(sizeof(_BaseMessage)));
	xTaskCreate(&BluetoothInterfaceBackservice::EventRouter,
				"BtTask",
				3072,
				nullptr,
				configMAX_PRIORITIES - 3,
				&task_handle);
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






template<>
void BluetoothInterfaceBackservice::btiStackupMsg::Execute(BluetoothInterface* btibi)
{
	btibi->WorkerStackup();
}

template<>
void BluetoothInterfaceBackservice::A2DEventMsg::Execute(BluetoothInterface* btibi)
{
	btibi->a2d.WorkerState(GetEvent(), GetArgument());
}

template<>
void BluetoothInterfaceBackservice::AVRCCtEventMsg::Execute(BluetoothInterface* btibi)
{
	btibi->avrcp.WorkerController(GetEvent(), GetArgument());
}

template<>
void BluetoothInterfaceBackservice::AVRCTgEventMsg::Execute(BluetoothInterface* btibi)
{
	btibi->avrcp.WorkerTarget(GetEvent(), GetArgument());
}





