
#include "bt_iface.hpp"
#include "bt_impl.hpp"

#include <cstring>


const char logTag[]		= "btiAVRCP";

esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
_lock_t s_volume_lock;
uint8_t s_volume = 0;
bool s_volume_notify;



void BluetoothAVRCP::SetVolume(uint8_t volume)
{
	ESP_LOGI(logTag, "Volume is set locally to: %d%%", (uint32_t )volume * 100 / 0x7f);
	_lock_acquire (&s_volume_lock);
	s_volume = volume;
	_lock_release(&s_volume_lock);

	if (s_volume_notify) {
		esp_avrc_rn_param_t rn_param;
		rn_param.volume = s_volume;
		esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_CHANGED, &rn_param);
		s_volume_notify = false;
	}
}

void BluetoothAVRCP::RequestMetadata()
{
	// request metadata
	uint8_t attr_mask = ESP_AVRC_MD_ATTR_TITLE
						| ESP_AVRC_MD_ATTR_ARTIST
						| ESP_AVRC_MD_ATTR_ALBUM
						| ESP_AVRC_MD_ATTR_GENRE;
	esp_avrc_ct_send_metadata_cmd(BluetoothTag::AVRCP::TransacionLabel::getMetadata, attr_mask);
}


void BluetoothAVRCP::EventNewTrack(decltype(esp_avrc_rn_param_t::elm_id) trackInfo)
{
	// register notification if peer support the event_id
	if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap, ESP_AVRC_RN_TRACK_CHANGE)) {
		esp_avrc_ct_send_register_notification_cmd(BluetoothTag::AVRCP::TransacionLabel::RNTrackChange, ESP_AVRC_RN_TRACK_CHANGE, 0);
	}
}

void BluetoothAVRCP::EventPlayback(decltype(esp_avrc_rn_param_t::playback) playback)
{
	ESP_LOGI(logTag, "Playback status changed: 0x%x", playback);
	// register notification if supported
	if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap, ESP_AVRC_RN_PLAY_STATUS_CHANGE)) {
		esp_avrc_ct_send_register_notification_cmd(
				BluetoothTag::AVRCP::TransacionLabel::RNPlaybackChange, ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
	}
}

void BluetoothAVRCP::EventPosition(decltype(esp_avrc_rn_param_t::play_pos) play_pos)
{
	ESP_LOGI(logTag, "Play position changed: %d-ms", play_pos);
	// register notification if supported
	if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap, ESP_AVRC_RN_PLAY_POS_CHANGED)) {
		esp_avrc_ct_send_register_notification_cmd(
				BluetoothTag::AVRCP::TransacionLabel::RNPlayPositionChange, ESP_AVRC_RN_PLAY_POS_CHANGED, 10);
	}
}

void BluetoothAVRCP::EventVolume(decltype(esp_avrc_rn_param_t::volume) volume)
{
	ESP_LOGI(logTag, "Volume is set by remote controller %d%%\n",
			static_cast<uint32_t>(volume * 100 / 0x7f));
	_lock_acquire (&s_volume_lock);
	s_volume = volume;
	_lock_release(&s_volume_lock);
}

void BluetoothAVRCP::EventBattery(decltype(esp_avrc_rn_param_t::batt) batt)
{
}

void BluetoothAVRCP::EventController(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
	auto bt_app_alloc_meta_buffer = [](esp_avrc_ct_cb_param_t *rc)
	{
		uint8_t *attr_text = (uint8_t *) malloc (rc->meta_rsp.attr_length + 1);	// allocate space for string
			memcpy(attr_text, rc->meta_rsp.attr_text, rc->meta_rsp.attr_length);// copy string
			attr_text[rc->meta_rsp.attr_length] = 0;// null terminate string

			rc->meta_rsp.attr_text = attr_text;// copy pointer of response for copied string
		};

	ESP_LOGI(logTag, "Event on controller with %d ", event);
	switch (event)
	{
	case ESP_AVRC_CT_METADATA_RSP_EVT:
		bt_app_alloc_meta_buffer(param);
		/* fall through */
	case ESP_AVRC_CT_CONNECTION_STATE_EVT:
	case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
	case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
	case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
	case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
	{
		btibs.Dispatch<BluetoothInterfaceBackservice::AVRCCtEventMsg>(event, param);
		break;
	}
	default:
		ESP_LOGE(logTag, "Invalid AVRC event: %d", event);
		break;
	}
}

void BluetoothAVRCP::WorkerController(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t* rc)
{
	ESP_LOGD(logTag, "%s evt %d", __func__, event);
	switch (event)
	{
	case ESP_AVRC_CT_CONNECTION_STATE_EVT:
	{
		uint8_t *bda = rc->conn_stat.remote_bda;
		ESP_LOGI(logTag, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
				rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

		if (rc->conn_stat.connected) {
			// get remote supported event_ids of peer AVRCP Target
			esp_avrc_ct_send_get_rn_capabilities_cmd(BluetoothTag::AVRCP::TransacionLabel::getCaps);
		}
		else {
			// clear peer notification capability record
			s_avrc_peer_rn_cap.bits = 0;
		}
		break;
	}
	case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
	{
		ESP_LOGI(logTag, "AVRC passthrough rsp: key_code 0x%x, key_state %d",
				rc->psth_rsp.key_code, rc->psth_rsp.key_state);
		break;
	}
	case ESP_AVRC_CT_METADATA_RSP_EVT:
	{
		ESP_LOGI(logTag, "AVRC metadata rsp: attribute id 0x%x, %s", rc->meta_rsp.attr_id,
				rc->meta_rsp.attr_text);
		// xxx what the fuck here?
		free(rc->meta_rsp.attr_text);
		break;
	}
	case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
	{
		Notification(static_cast<esp_avrc_rn_event_ids_t>(rc->change_ntf.event_id),
					 &rc->change_ntf.event_parameter);
		break;
	}
	case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
	{
		ESP_LOGI(logTag, "AVRC remote features %x, TG features %x", rc->rmt_feats.feat_mask,
				rc->rmt_feats.tg_feat_flag);
		break;
	}
	case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
	{
		ESP_LOGI(logTag, "remote rn_cap: count %d, bitmask 0x%x",
				rc->get_rn_caps_rsp.cap_count, rc->get_rn_caps_rsp.evt_set.bits);
		s_avrc_peer_rn_cap.bits = rc->get_rn_caps_rsp.evt_set.bits;
		break;
	}
	default:
		ESP_LOGE(logTag, "%s unhandled evt %d", __func__, event);
		break;
	}
}

void BluetoothAVRCP::Notification(esp_avrc_rn_event_ids_t event_id, esp_avrc_rn_param_t *event_parameter)
{
	ESP_LOGI(logTag, "AVRC event notification: %d", event_id);
	switch (event_id)
	{
	case ESP_AVRC_RN_PLAY_STATUS_CHANGE:	EventPlayback(event_parameter->playback);	break;
	case ESP_AVRC_RN_TRACK_CHANGE:			EventNewTrack(event_parameter->elm_id);		break;
	case ESP_AVRC_RN_TRACK_REACHED_END:		break;
	case ESP_AVRC_RN_TRACK_REACHED_START:	break;
	case ESP_AVRC_RN_PLAY_POS_CHANGED:		EventPosition(event_parameter->play_pos);	break;
	case ESP_AVRC_RN_BATTERY_STATUS_CHANGE:	EventBattery(event_parameter->batt);		break;
	case ESP_AVRC_RN_SYSTEM_STATUS_CHANGE:	break;
	case ESP_AVRC_RN_APP_SETTING_CHANGE:	break;
	case ESP_AVRC_RN_NOW_PLAYING_CHANGE:	break;
	case ESP_AVRC_RN_AVAILABLE_PLAYERS_CHANGE:	break;
	case ESP_AVRC_RN_ADDRESSED_PLAYER_CHANGE:	break;
	case ESP_AVRC_RN_UIDS_CHANGE:				break;
	case ESP_AVRC_RN_VOLUME_CHANGE:			EventVolume(event_parameter->volume);		break;
	default:								break;
	}
}

void BluetoothAVRCP::EventTarget(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
	switch (event)
	{
	case ESP_AVRC_TG_CONNECTION_STATE_EVT:
	case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
	case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
	case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
	case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT:
		btibs.Dispatch<BluetoothInterfaceBackservice::AVRCTgEventMsg>(event, param);
		break;
	default:
		ESP_LOGE(logTag, "Invalid AVRC event: %d", event);
		break;
	}
}

void BluetoothAVRCP::WorkerTarget(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t* rc)
{
	ESP_LOGD(logTag, "%s evt %d", __func__, event);

	switch (event)
	{
	case ESP_AVRC_TG_CONNECTION_STATE_EVT:
	{
		uint8_t *bda = rc->conn_stat.remote_bda;
		ESP_LOGI(logTag, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
				rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
		if (rc->conn_stat.connected) {
			//fixme how do we notify that we are now connected? shoot signal up?
		}
		else {
			ESP_LOGI(logTag, "Connection Dropped");
		}
		break;
	}
	case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
	{
		ESP_LOGI(logTag, "AVRC passthrough cmd: key_code 0x%x, key_state %d",
				rc->psth_cmd.key_code, rc->psth_cmd.key_state);
		break;
	}
	case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
	{
		EventVolume(rc->set_abs_vol.volume);
		break;
	}
	case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT:
	{
		ESP_LOGI(logTag, "AVRC register event notification: %d, param: 0x%x",
				rc->reg_ntf.event_id, rc->reg_ntf.event_parameter);
		if (rc->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE) {
			s_volume_notify = true;
			esp_avrc_rn_param_t rn_param;
			rn_param.volume = s_volume;
			esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &rn_param);
		}
		break;
	}
	case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
	{
		ESP_LOGI(logTag, "AVRC remote features %x, CT features %x", rc->rmt_feats.feat_mask,
				rc->rmt_feats.ct_feat_flag);
		break;
	}
	default:
		ESP_LOGE(logTag, "%s unhandled evt %d", __func__, event);
		break;
	}
}

void BluetoothAVRCP::Boot()
{
	/* initialize AVRCP controller */
	esp_avrc_ct_init();
	esp_avrc_ct_register_callback(&BluetoothAVRCP::EventController);

	/* initialize AVRCP target */
	assert(esp_avrc_tg_init() == ESP_OK);
	esp_avrc_tg_register_callback(&BluetoothAVRCP::EventTarget);

	esp_avrc_rn_evt_cap_mask_t evt_set = { 0 };
	esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
	assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);
}

