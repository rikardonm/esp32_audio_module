/*
 * bt_interface.hpp
 *
 *  Created on: Jul 3, 2019
 *      Author: ricardo
 */

#ifndef MAIN_BT_AUDIO_IFACE_HPP_
#define MAIN_BT_INTERFACE_HPP_


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <string>




/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/
/**************************************************************/




#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"

#include "sys/lock.h"
















class BluetoothA2D {
public:

	void SetVolume(uint8_t volume);
	void VolumeUp(unsigned int change = 1);
	void VolumeDown(unsigned int change = 1);
	void Mute();
	void RequestNextTrack();
	void RequestPreviousTrack();
	void RequestPlay();
	void RequestPause();
	void RequestPlayToogle();
	void RequestStop();

	void NewTrackEvent();
	void PlaybackEvent();
	void PositionEvent();
	void VolumeEvent(uint8_t volume);

	void callbackSink(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
	{
	    switch (event) {
	    case ESP_A2D_CONNECTION_STATE_EVT:
	    case ESP_A2D_AUDIO_STATE_EVT:
	    case ESP_A2D_AUDIO_CFG_EVT: {
	        bt_app_work_dispatch(BTI____bt_av_hdl_a2d_evt, event, param, static_cast<unsigned int>(sizeof(esp_a2d_cb_param_t)), nullptr);
	        break;
	    }
	    default:
	        ESP_LOGE(BluetoothInterfaceBackservice::Log, "Invalid A2DP event: %d", event);
	        break;
	    }
	}


	void callbackData(const uint8_t *data, uint32_t len)
	{
	    size_t bytes_written;
	    i2s_write(0, data, len, &bytes_written, portMAX_DELAY);
	    if (++s_pkt_cnt % 100 == 0) {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "Audio packet count %u", s_pkt_cnt);
	    }
	}




	void worker(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* a2d)
	{

		ESP_LOGD(BluetoothInterfaceBackservice::Log, "%s evt %d", __func__, event);

		switch (event) {
		case ESP_A2D_CONNECTION_STATE_EVT: {
			uint8_t *bda = a2d->conn_stat.remote_bda;
			ESP_LOGI(BluetoothInterfaceBackservice::Log, "A2DP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
				 BluetoothInterfaceBackservice::A2D::StateStrings::connection[a2d->conn_stat.state], bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
			if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
				esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
			} else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED){
				esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
			}
			break;
		}
		case ESP_A2D_AUDIO_STATE_EVT: {
			ESP_LOGI(BluetoothInterfaceBackservice::Log, "A2DP audio state: %s",
					BluetoothInterfaceBackservice::A2D::StateStrings::audio[a2d->audio_stat.state]);
			s_audio_state = a2d->audio_stat.state;
			if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state) {
				s_pkt_cnt = 0;
			}
			break;
		}
		case ESP_A2D_AUDIO_CFG_EVT: {
			ESP_LOGI(BluetoothInterfaceBackservice::Log, "A2DP audio stream configuration, codec type %d", a2d->audio_cfg.mcc.type);
			// for now only SBC stream is supported
			if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
				int sample_rate = 16000;
				char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];
				if (oct0 & (0x01 << 6)) {
					sample_rate = 32000;
				} else if (oct0 & (0x01 << 5)) {
					sample_rate = 44100;
				} else if (oct0 & (0x01 << 4)) {
					sample_rate = 48000;
				}

				i2s_set_clk(0, sample_rate, 16, 2);

				ESP_LOGI(BluetoothInterfaceBackservice::Log, "Configure audio player %x-%x-%x-%x",
						 a2d->audio_cfg.mcc.cie.sbc[0],
						 a2d->audio_cfg.mcc.cie.sbc[1],
						 a2d->audio_cfg.mcc.cie.sbc[2],
						 a2d->audio_cfg.mcc.cie.sbc[3]);
				ESP_LOGI(BluetoothInterfaceBackservice::Log, "Audio player configured, sample rate=%d", sample_rate);
			}
			break;
		}
		default:
			ESP_LOGE(BluetoothInterfaceBackservice::Log, "%s unhandled evt %d", __func__, event);
			break;
		}
	}





private:
	void RequestMetadata();
};






class BluetoothAVRCP {
public:
	// something something

	void callbackController(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
	{
		switch (event) {
		case ESP_AVRC_CT_METADATA_RSP_EVT:
			bt_app_alloc_meta_buffer(param);
			/* fall through */
		case ESP_AVRC_CT_CONNECTION_STATE_EVT:
		case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
		case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
		case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
		case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
			bt_app_work_dispatch(BTI___bt_av_hdl_avrc_ct_evt, event, param, static_cast<unsigned int>(sizeof(esp_avrc_ct_cb_param_t)), nullptr);
			break;
		}
		default:
			ESP_LOGE(BluetoothInterfaceBackservice::Log, "Invalid AVRC event: %d", event);
			break;
		}
	}


	void workerController(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t* rc)
	{
	    ESP_LOGD(BluetoothInterfaceBackservice::Log, "%s evt %d", __func__, event);
	    switch (event) {
	    case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
	        uint8_t *bda = rc->conn_stat.remote_bda;
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
	                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

	        if (rc->conn_stat.connected) {
	            // get remote supported event_ids of peer AVRCP Target
	            esp_avrc_ct_send_get_rn_capabilities_cmd(BluetoothInterfaceBackservice::AVRCPTransacionLabel::getCaps);
	        } else {
	            // clear peer notification capability record
	            s_avrc_peer_rn_cap.bits = 0;
	        }
	        break;
	    }
	    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT: {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "AVRC passthrough rsp: key_code 0x%x, key_state %d", rc->psth_rsp.key_code, rc->psth_rsp.key_state);
	        break;
	    }
	    case ESP_AVRC_CT_METADATA_RSP_EVT: {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "AVRC metadata rsp: attribute id 0x%x, %s", rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
	        free(rc->meta_rsp.attr_text);
	        break;
	    }
	    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT: {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "AVRC event notification: %d", rc->change_ntf.event_id);
	        bt_av_notify_evt_handler(rc->change_ntf.event_id, &rc->change_ntf.event_parameter);
	        break;
	    }
	    case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "AVRC remote features %x, TG features %x", rc->rmt_feats.feat_mask, rc->rmt_feats.tg_feat_flag);
	        break;
	    }
	    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "remote rn_cap: count %d, bitmask 0x%x", rc->get_rn_caps_rsp.cap_count,
	                 rc->get_rn_caps_rsp.evt_set.bits);
	        s_avrc_peer_rn_cap.bits = rc->get_rn_caps_rsp.evt_set.bits;
	        bt_av_new_track();
	        bt_av_playback_changed();
	        bt_av_play_pos_changed();
	        break;
	    }
	    default:
	        ESP_LOGE(BluetoothInterfaceBackservice::Log, "%s unhandled evt %d", __func__, event);
	        break;
	    }
	}


	void callbackNotification(esp_avrc_rn_event_ids_t event_id, esp_avrc_rn_param_t *event_parameter)
	{
	    switch (event_id) {
	    case ESP_AVRC_RN_TRACK_CHANGE:
	        bt_av_new_track();
	        break;
	    case ESP_AVRC_RN_PLAY_STATUS_CHANGE:
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "Playback status changed: 0x%x", event_parameter->playback);
	        bt_av_playback_changed();
	        break;
	    case ESP_AVRC_RN_PLAY_POS_CHANGED:
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "Play position changed: %d-ms", event_parameter->play_pos);
	        bt_av_play_pos_changed();
	        break;
	    }
	}

	void callbackTarget(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
	{
		switch (event) {
		case ESP_AVRC_TG_CONNECTION_STATE_EVT:
		case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
		case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
		case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
		case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT:
			bt_app_work_dispatch(BTI____bt_av_hdl_avrc_tg_evt, event, param, static_cast<unsigned int>(sizeof(esp_avrc_tg_cb_param_t)), nullptr);
			break;
		default:
			ESP_LOGE(BluetoothInterfaceBackservice::Log, "Invalid AVRC event: %d", event);
			break;
		}
	}


	void workerTarget(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t* rc)
	{
	    ESP_LOGD(BluetoothInterfaceBackservice::Log, "%s evt %d", __func__, event);

	    switch (event) {
	    case ESP_AVRC_TG_CONNECTION_STATE_EVT: {
	        uint8_t *bda = rc->conn_stat.remote_bda;
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
	                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
	        if (rc->conn_stat.connected) {
	        	//fixme how do we notify that we are now connected? shoot signal up?
	        } else {
	            ESP_LOGI(BluetoothInterfaceBackservice::Log, "Connection Dropped");
	        }
	        break;
	    }
	    case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT: {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "AVRC passthrough cmd: key_code 0x%x, key_state %d", rc->psth_cmd.key_code, rc->psth_cmd.key_state);
	        break;
	    }
	    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "AVRC set absolute volume: %d%%", (int)rc->set_abs_vol.volume * 100/ 0x7f);
	        volume_set_by_controller(rc->set_abs_vol.volume);
	        break;
	    }
	    case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT: {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "AVRC register event notification: %d, param: 0x%x", rc->reg_ntf.event_id, rc->reg_ntf.event_parameter);
	        if (rc->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE) {
	            s_volume_notify = true;
	            esp_avrc_rn_param_t rn_param;
	            rn_param.volume = s_volume;
	            esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &rn_param);
	        }
	        break;
	    }
	    case ESP_AVRC_TG_REMOTE_FEATURES_EVT: {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "AVRC remote features %x, CT features %x", rc->rmt_feats.feat_mask, rc->rmt_feats.ct_feat_flag);
	        break;
	    }
	    default:
	        ESP_LOGE(BluetoothInterfaceBackservice::Log, "%s unhandled evt %d", __func__, event);
	        break;
	    }
	}


};




class BluetoothGap {
public:
	// something else here


	void callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
	{
	    switch (event) {
	    case ESP_BT_GAP_AUTH_CMPL_EVT: {
	        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
	            ESP_LOGI(BluetoothInterfaceBackservice::Log, "authentication success: %s", param->auth_cmpl.device_name);
	            esp_log_buffer_hex(BluetoothInterfaceBackservice::Log, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
	        } else {
	            ESP_LOGE(BluetoothInterfaceBackservice::Log, "authentication failed, status:%d", param->auth_cmpl.stat);
	        }
	        break;
	    }
	    // SSP
	    case ESP_BT_GAP_CFM_REQ_EVT:
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
	        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
	        break;
	    case ESP_BT_GAP_KEY_NOTIF_EVT:
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
	        break;
	    case ESP_BT_GAP_KEY_REQ_EVT:
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
	        break;
	    default: {
	        ESP_LOGI(BluetoothInterfaceBackservice::Log, "event: %d", event);
	        break;
	    }
	    }
	    return;
	}
};



class BluetoothInterface
{
public:
	bool ValidadePinCode(std::string npin);
	bool SetNewPinCode(std::string npin);
	bool Connect();
	bool Disconnect();
	bool ConnectLastPaired();
	bool Scan();
	bool Enable();
	bool Disable();
	void Boot();
	void SetMode();

	BluetoothA2D a2d;
	BluetoothAVRCP avrcp;

};
































void BluetoothA2D::VolumeEvent(uint8_t volume)
{
    ESP_LOGI(BluetoothInterfaceBackservice::Log, "Volume is set by remote controller %d%%\n", (uint32_t)volume * 100 / 0x7f);
    _lock_acquire(&s_volume_lock);
    s_volume = volume;
    _lock_release(&s_volume_lock);
}

void BluetoothA2D::SetVolume(uint8_t volume)
{
    ESP_LOGI(BluetoothInterfaceBackservice::Log, "Volume is set locally to: %d%%", (uint32_t)volume * 100 / 0x7f);
    _lock_acquire(&s_volume_lock);
    s_volume = volume;
    _lock_release(&s_volume_lock);

    if (s_volume_notify) {
        esp_avrc_rn_param_t rn_param;
        rn_param.volume = s_volume;
        esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_CHANGED, &rn_param);
        s_volume_notify = false;
    }
}





void BluetoothA2D::RequestMetadata()
{
    // request metadata
    uint8_t attr_mask = ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_GENRE;
    esp_avrc_ct_send_metadata_cmd(BluetoothInterfaceBackservice::AVRCPTransacionLabel::getMetadata, attr_mask);
}

void BluetoothA2D::NewTrackEvent()
{
    // register notification if peer support the event_id
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_TRACK_CHANGE)) {
        esp_avrc_ct_send_register_notification_cmd(BluetoothInterfaceBackservice::AVRCPTransacionLabel::RNTrackChange, ESP_AVRC_RN_TRACK_CHANGE, 0);
    }
}

void BluetoothA2D::PlaybackEvent()
{
	// register notification if supported
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_PLAY_STATUS_CHANGE)) {
        esp_avrc_ct_send_register_notification_cmd(BluetoothInterfaceBackservice::AVRCPTransacionLabel::RNPlaybackChange, ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
    }
}

void BluetoothA2D::PositionEvent()
{
	// register notification if supported
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_PLAY_POS_CHANGED)) {
        esp_avrc_ct_send_register_notification_cmd(BluetoothInterfaceBackservice::AVRCPTransacionLabel::RNPlayPositionChange, ESP_AVRC_RN_PLAY_POS_CHANGED, 10);
    }
}

























// todo
// make some RAII for delayed execution?
// can I do that? accumulate new settings and only then apply them?


extern BluetoothInterface bti;

#endif /* MAIN_BT_AUDIO_IFACE_HPP_ */
