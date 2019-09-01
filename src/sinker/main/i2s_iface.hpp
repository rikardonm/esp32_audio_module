#ifndef I2S_IFACE_HPP
#define I2S_IFACE_HPP

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"
#include "esp_log.h"


class I2SInterface
{
public:
	I2SInterface() = default;
	~I2SInterface() = default;

	bool Boot(i2s_port_t module);
	bool Boot(i2s_port_t module, i2s_config_t config);

	unsigned int Write(const uint8_t* data, uint32_t len)
	{
		unsigned int written;
		i2s_write(module_, data, len, &written, portMAX_DELAY);
		return written;
	}

	void SetSampleRate(int rate,
					   i2s_bits_per_sample_t bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
					   i2s_channel_t channels = I2S_CHANNEL_STEREO)
	{
		i2s_set_clk(module_, rate, bits_per_sample, channels);
	}

	void ConfigurePins(int blk,
					   int lck,
					   int din,
					   int dout);

private:
    i2s_config_t i2s_config_;
    i2s_pin_config_t pin_config_;
    i2s_port_t module_;

    static bool booted_[I2S_NUM_MAX];

};


#endif // I2S_IFACE_HPP
