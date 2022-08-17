/*
 * irs_play.c
 *
 *  Created on: Aug 17, 2022
 *      Author: shiv
 */

#include "driver/rmt.h"

#include "logging.h"
#include "errors.h"

#define IR_EMITTER_RMT_CHANNEL RMT_CHANNEL_1

void initialize_rmt_driver(gpio_num_t gpio_output_pin, unsigned int duty_cycle, unsigned int frequency) {
	LOGI("Installing RMT Driver");
    rmt_config_t rmt_cfg;
    rmt_cfg.rmt_mode = RMT_MODE_TX;
    rmt_cfg.channel = IR_EMITTER_RMT_CHANNEL;
    rmt_cfg.gpio_num = gpio_output_pin;
    rmt_cfg.mem_block_num = 1;
    rmt_cfg.tx_config.loop_en = 0;
    rmt_cfg.tx_config.carrier_en = 1 ;
    rmt_cfg.tx_config.idle_output_en = 1;
    rmt_cfg.tx_config.idle_level = 0;
    rmt_cfg.tx_config.carrier_duty_percent = duty_cycle;
    rmt_cfg.tx_config.carrier_freq_hz = frequency;
    rmt_cfg.tx_config.carrier_level = 1;
    rmt_cfg.clk_div = 80;
    rmt_config(&rmt_cfg);
    rmt_driver_install(IR_EMITTER_RMT_CHANNEL, 0, 0);
    LOGI("Done!");
}

void cleanup_rmt_driver(gpio_num_t gpio_output_pin) {
	LOGI("Cleaning up RMT Driver");
	rmt_driver_uninstall(IR_EMITTER_RMT_CHANNEL);
	LOGI("Done!");
}

void play_command(unsigned int length, rmt_item32_t * rmt_items) {
	LOGI("Sending Signal");
	rmt_write_items(IR_EMITTER_RMT_CHANNEL, rmt_items, length, true);
	LOGI("Done!");
}
