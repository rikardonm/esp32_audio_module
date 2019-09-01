





#include "bt_iface.hpp"
#include "bt_impl.hpp"

const char logTag[]		= "btiA2D";


uint32_t s_pkt_cnt = 0;
esp_a2d_audio_state_t s_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;



void BluetoothA2D::SetCallbacks(BluetoothTag::A2D::CallbackType::SampleRate cb_sr,
								BluetoothTag::A2D::CallbackType::Data cb_dta)
{
	sample_rate_cb_ = cb_sr;
	data_cb_ = cb_dta;
}

void BluetoothA2D::Boot()
{
	/* initialize A2DP sink */
	esp_a2d_register_callback(&BluetoothA2D::EventState);
	esp_a2d_sink_register_data_callback(&BluetoothA2D::EventDataSink);
	esp_a2d_sink_init();
}

void BluetoothA2D::EventState(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
	ESP_LOGI(logTag, "Got into A2D Event with %d", event);
	switch (event)
	{
	case ESP_A2D_CONNECTION_STATE_EVT:
	case ESP_A2D_AUDIO_STATE_EVT:
	case ESP_A2D_AUDIO_CFG_EVT:
	{
		btibs.Dispatch<BluetoothInterfaceBackservice::A2DEventMsg>(event, param);
		break;
	}
	default:
		ESP_LOGE(logTag, "Invalid A2DP event: %d", event);
		break;
	}
}

void BluetoothA2D::EventDataSink(const uint8_t *data, uint32_t len)
{
	static size_t bytes_written = 0;
	//fixme what a hack to just leave it with a static/global
	if (BluetoothInterface::GetInstance()->a2d.data_cb_)
		bytes_written += BluetoothInterface::GetInstance()->a2d.data_cb_(data, len);
	if (++s_pkt_cnt % 100 == 0) {
		ESP_LOGI(logTag, "Audio packet count %u. Total of %d bytes", s_pkt_cnt, bytes_written);
	}
}

void BluetoothA2D::WorkerState(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* a2d)
{
	ESP_LOGD(logTag, "%s evt %d", __func__, event);
	switch (event)
	{
	case ESP_A2D_CONNECTION_STATE_EVT:
	{
		uint8_t *bda = a2d->conn_stat.remote_bda;
		ESP_LOGI(logTag, "A2DP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
				BluetoothTag::A2D::StateStrings::connection[a2d->conn_stat.state], bda[0], bda[1],
				bda[2], bda[3], bda[4], bda[5]);
		if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
			esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
		}
		else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
			esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
		}
		break;
	}
	case ESP_A2D_AUDIO_STATE_EVT:
	{
		ESP_LOGI(logTag, "A2DP audio state: %s",
				BluetoothTag::A2D::StateStrings::audio[a2d->audio_stat.state]);
		s_audio_state = a2d->audio_stat.state;
		if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state) {
			s_pkt_cnt = 0;
		}
		break;
	}
	case ESP_A2D_AUDIO_CFG_EVT:
	{
		ESP_LOGI(logTag, "A2DP audio stream configuration, codec type %d",
				a2d->audio_cfg.mcc.type);
		// for now only SBC stream is supported
		if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
			int sample_rate = 16000;
			char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];
			if (oct0 & (0x01 << 6)) {
				sample_rate = 32000;
			}
			else if (oct0 & (0x01 << 5)) {
				sample_rate = 44100;
			}
			else if (oct0 & (0x01 << 4)) {
				sample_rate = 48000;
			}
			if (sample_rate_cb_)
				sample_rate_cb_(sample_rate);

			ESP_LOGI(logTag, "Configure audio player %x-%x-%x-%x",
					a2d->audio_cfg.mcc.cie.sbc[0], a2d->audio_cfg.mcc.cie.sbc[1], a2d->audio_cfg.mcc.cie.sbc[2],
					a2d->audio_cfg.mcc.cie.sbc[3]);
			ESP_LOGI(logTag, "Audio player configured, sample rate=%d", sample_rate);
		}
		break;
	}
	default:
		ESP_LOGE(logTag, "%s unhandled evt %d", __func__, event);
		break;
	}
}

