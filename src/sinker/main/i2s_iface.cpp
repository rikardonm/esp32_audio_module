

#include "i2s_iface.hpp"


bool I2SInterface::booted_[I2S_NUM_MAX] = {false, false};

bool I2SInterface::Boot(i2s_port_t module)
{
	return Boot(module, {
						.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),         // Only TX
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
						});
}


bool I2SInterface::Boot(i2s_port_t module, i2s_config_t config)
{
	if (module >= I2S_NUM_MAX)
		return false;
	if (booted_[module])
		return false;

	module_ = module;
	i2s_config_ = config;
	//fixme who clears this on tear-down?
	booted_[module] = true;

	i2s_driver_install(I2S_NUM_0, &i2s_config_, 0, nullptr);

	return true;
}


void I2SInterface::ConfigurePins(int blk,
								 int lck,
								 int din,
								 int dout)
{
	pin_config_ = i2s_pin_config_t {
		.bck_io_num = blk,
		.ws_io_num = lck,
		.data_out_num = dout,
		.data_in_num = din
	};

	i2s_set_pin(I2S_NUM_0, &pin_config_);
}



