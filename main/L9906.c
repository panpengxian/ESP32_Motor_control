/*
 * L9906.c
 *
 *  Created on: Sep 3, 2021
 *      Author: RD02995
 */

#include "L9906.h"
#include "esp_log.h"

void predrv_spi_Init(predrv_context_t** out_ctx)
{
    predrv_context_t* ctx = (predrv_context_t*)malloc(sizeof(predrv_context_t));
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = BIT64(DRV_CS)|BIT64(DRV_MOSI)|BIT64(DRV_MISO)|BIT64(DRV_SCLK),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    spi_bus_config_t spi_cfg = {
        .mosi_io_num = DRV_MOSI,
        .miso_io_num = DRV_MISO,
        .sclk_io_num = DRV_SCLK,
        .max_transfer_sz = 1024,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };

    spi_device_interface_config_t spi_dev_if_cfg = {
        .command_bits = 6,
        .mode = 1,//CPOL = 0; CPHA = 1;
        .duty_cycle_pos = 128,//dutycycle = 128/256
        .clock_speed_hz = SPI_MASTER_FREQ_8M/2,//4M
        .spics_io_num = -1,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
    int re_code = spi_bus_initialize(SPI3_HOST, &spi_cfg, 0);
    if(re_code == ESP_OK ){
        ESP_LOGI("SPI:","Bus initialized!");
    }
    else{
        ESP_LOGI("SPI:","Bus failed!\nerror code is %d\n",re_code);
    }
    re_code = spi_bus_add_device(SPI3_HOST, &spi_dev_if_cfg, &ctx->spi);
    if(re_code == ESP_OK){
        ESP_LOGI("SPI:","device added to bus!");
        *out_ctx = ctx;
    }
    else{
        ESP_LOGI("SPI:","devive not added to device!\nerror code is %d\n",re_code);
    }
}


void wr_predrv(predrv_context_t* ctx, int cmd, int data)
{
  char low_bit = data;
  char high_bit = data>>8;
  spi_transaction_t spi_trans={
      .flags = SPI_TRANS_USE_RXDATA|SPI_TRANS_USE_TXDATA,
      .cmd = cmd,
      .length = 10,
      .tx_data = {low_bit,high_bit},
  };
  gpio_set_level(DRV_CS, 0);
  ESP_ERROR_CHECK(spi_device_polling_transmit(ctx->spi, &spi_trans));
  gpio_set_level(DRV_CS,1);
}

int rd_predrv(predrv_context_t* ctx, int cmd)
{
  int rxdata;
  spi_transaction_t spi_trans={
      .flags = SPI_TRANS_USE_RXDATA|SPI_TRANS_USE_TXDATA,
      .cmd = cmd,
      .length = 10,
      .rxlength = 10,
      .tx_data = {0},
  };
  gpio_set_level(DRV_CS, 0);
  ESP_ERROR_CHECK(spi_device_polling_transmit(ctx->spi, &spi_trans));
  gpio_set_level(DRV_CS,1);
  rxdata = (spi_trans.rx_data[0]<<8) + (spi_trans.rx_data[1]);
  return rxdata;
}
