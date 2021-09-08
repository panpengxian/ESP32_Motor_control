/*
 * L9906.h
 *
 *  Created on: Sep 3, 2021
 *      Author: RD02995
 */

#ifndef L9906_H_
#define L9906_H_

#include "driver/spi_master.h"
#include "driver/gpio.h"


#define DRV_CS GPIO_NUM_18
#define DRV_SCLK GPIO_NUM_5
#define DRV_MOSI GPIO_NUM_17//U2TXD
#define DRV_MISO GPIO_NUM_16//U2RXD

#define WR_CMD0 0x03//b0000*1*1
#define WR_CMD1 0x0f//b001111
#define WR_CMD2 0x17
#define WR_CMD3 0x1d
#define WR_CMD4 0x27

typedef struct {
    spi_device_handle_t spi;
}predrv_context_t;

typedef predrv_context_t* predrv_handle;

void predrv_spi_Init(predrv_context_t** out_ctx);
void wr_predrv(predrv_context_t* ctx, int cmd, int data);
int rd_predrv(predrv_context_t* ctx, int cmd);

#endif /* L9906_H_ */
