

#pragma once
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "sdkconfig.h"

#define NCS GPIO_NUM_23
#define MOSI GPIO_NUM_22
#define MISO GPIO_NUM_21
#define SCLK GPIO_NUM_19

#define SW_LOCK 0xbd //0xaa
#define SW_UNLOCK 0xbb //0x55

#define RD_DEV_ID 0x06
#define RD_DEV_REV 0x0c
#define RD_DEV_CFG1 0xaf
#define RD_DEV_CFG2 0x48
#define RD_SAFETY_STAT_1 0x24
#define RD_SAFETY_STAT_2 0xc5
#define RD_SAFETY_STAT_3 0xa3
#define RD_SAFETY_STAT_4 0xa5
#define RD_SAFETY_STAT_5 0xc0
#define RD_SAFETY_ERR_CFG 0x30
#define RD_SAFETY_ERR_STAT 0xaa
#define RD_SAFETY_PWD_THR_CFG 0x39
#define RD_SAFETY_CHECK_CTRL 0x44
#define RD_SAFETY_BIST_CTRL 0x3c
#define RD_WD_WIN1_CFG 0x2e
#define RD_WD_WIN2_CFG 0x05
#define RD_WD_TOKEN_VALUE 0x36
#define RD_WD_STATUS 0x4e
#define RD_DEV_STAT 0x11
#define RD_VMON_STAT_1 0x12
#define RD_VMON_STAT_2 0x12
#define RD_SENS_CTRL 0x56
#define RD_SAFETY_FUNC_CFG 0x3a
#define RD_SAFETY_CFG_CRC 0x5a
#define RD_DIAG_CFG_CTRL 0xdd
#define RD_DIAG_MUX_SEL 0xac
#define RD_SAFETY_ERR_PWM_H 0xd7
#define RD_SAFETY_ERR_PWM_L 0x59
#define RD_WD_TOKEN_FDBK 0x78

#define WR_DEV_CFG1 0xb7
#define WR_DEV_CFG2 0x95
#define WR_CAN_STBY 0x7d
#define WR_SAFETY_ERR_CFG 0xdb
#define WR_SAFETY_ERR_STAT 0xa9
#define WR_SAFETY_PWD_THR_CFG 0x99
#define WR_WD_WIN1_CFG 0xed
#define WR_WD_WIN2_CFG 0x09
#define WR_SAFETY_FUNC_CFG 0x35
#define WR_SAFETY_CFG_CRC 0x63
#define WR_SAFETY_ERR_PWM_H 0xd8
#define WR_SAFETY_ERR_PWM_L 0x7e
#define WR_WD_TOKEN_FDBK 0x77

#define WR_SAFETY_CHECK_CTRL 0x93
#define WR_SAFETY_BIST_CTRL 0x9f
#define WR_WD_ANSWER 0xe1
#define WR_SENS_CTRL 0x7b
#define WR_DIAG_CFG_CTRL 0xcc
#define WR_DIAG_MUX_SEL 0xc9

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
  spi_device_handle_t spi;
}pmic_context_t;

typedef struct pmic_context_t* pmic_handle_t;

void blink(void);
void pmic_spi_init(pmic_context_t** out_ctx);
void wr_pmic(pmic_context_t* ctx, int cmd, int data);
int rd_pmic(pmic_context_t* ctx, int cmd);
