/*
 * tps65381a.c
 *
 *  Created on: Sep 2, 2021
 *      Author: RD02995
 */
#include "stdio.h"
#include "tps65381a.h"
#include "driver/timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"

void blink(void)
{
    printf("hello");
}

void pmic_spi_init(pmic_context_t** out_ctx)
{
    pmic_context_t* ctx = (pmic_context_t*)malloc(sizeof(pmic_context_t));
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = BIT64(NCS)|BIT64(MOSI)|BIT64(MISO)|BIT64(SCLK)|BIT64(GPIO_NUM_2),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    spi_bus_config_t spi_cfg = {
        .mosi_io_num = MOSI,
        .miso_io_num = MISO,
        .sclk_io_num = SCLK,
        .max_transfer_sz = 1024,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };

    spi_device_interface_config_t spi_dev_if_cfg = {
        .command_bits = 8,//命令的长度为8
        .address_bits = 0,//没有地址
        .mode = 1,//CPOL=0，CPHA=1
        .duty_cycle_pos = 128,//dutycycle = 128/256
        .clock_speed_hz = SPI_MASTER_FREQ_8M/2,//4M
        .spics_io_num = -1,//没有用到csio
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
    int re_code = spi_bus_initialize(SPI2_HOST, &spi_cfg, 0);
    if(re_code == ESP_OK ){
        ESP_LOGI("SPI:","Bus initialized!");
    }
    else{
        ESP_LOGI("SPI:","Bus failed!\nerror code is %d\n",re_code);
    }
    re_code = spi_bus_add_device(SPI2_HOST, &spi_dev_if_cfg, &ctx->spi);
    if(re_code == ESP_OK){
        ESP_LOGI("SPI:","device added to bus!");
        *out_ctx = ctx;
    }
    else{
        ESP_LOGI("SPI:","devive not added to device!\nerror code is %d\n",re_code);
    }
}

void wr_pmic(pmic_context_t* ctx, int cmd, int data)
{
  spi_transaction_t spi_trans={
      .flags = SPI_TRANS_USE_RXDATA|SPI_TRANS_USE_TXDATA,
      .cmd =cmd,
      .length = 8,
      .tx_data = {data},
  };
  gpio_set_level(NCS, 0);
  ESP_ERROR_CHECK(spi_device_polling_transmit(ctx->spi,&spi_trans));
  gpio_set_level(NCS,1);
}

int rd_pmic(pmic_context_t* ctx, int cmd)
{
  spi_transaction_t spi_trans={
      .flags = SPI_TRANS_USE_RXDATA|SPI_TRANS_USE_TXDATA,
      .cmd = cmd,
      .length = 8,
      .rxlength = 8,
      .tx_data = {0x00},
      };
  gpio_set_level(NCS, 0);
  ESP_ERROR_CHECK(spi_device_polling_transmit(ctx->spi, &spi_trans));
  gpio_set_level(NCS,1);
  //ESP_LOGI("SPI","Response is %d",spi_trans.rx_data[0]);
  return spi_trans.rx_data[0];
}

