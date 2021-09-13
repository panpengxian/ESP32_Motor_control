#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "tps65381a.h"
#include "L9906.h"
#include "driver/timer.h"
#include "sdkconfig.h"
#include "esp_task_wdt.h"
#include "driver/adc.h"

#define WDI GPIO_NUM_2

int wdi_level = 0;
//static QueueHandle_t xQueue;
static xQueueHandle s_timer_queue;

//vTaskDelay(6/portTICK_PERIOD_MS);

void read_endrv(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);
    int val = adc1_get_raw(ADC1_CHANNEL_0);
    printf("En_Drv Voltage is:%d;\r\n",val);
}

static bool IRAM_ATTR pmic_wtd1(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    wdi_level = wdi_level^1;
    gpio_set_level(WDI, wdi_level);
    if (wdi_level == 1)
    {
        timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 1000);
    }
    else{timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 9000);}
    return high_task_awoken == pdTRUE;
}

void timer_task1(void)
{
  timer_config_t timer_cfg={
      .alarm_en = TIMER_ALARM_EN,
      .counter_en = TIMER_PAUSE,
      .counter_dir = TIMER_COUNT_UP,
      .auto_reload = TIMER_AUTORELOAD_EN,
      .divider = 80,
      //.clk_src = TIMER_SRC_CLK_APB,
      };
  timer_init(TIMER_GROUP_0, TIMER_0, &timer_cfg);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 9000);
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);
  timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, pmic_wtd1, NULL, 0);
  timer_start(TIMER_GROUP_0, TIMER_0);
}

void app_main(void)
{
    pmic_handle_t pmic_dev;
    predrv_handle_t predrv_dev;
    s_timer_queue = xQueueCreate(10, sizeof(int));
    //blink();
    
    pmic_spi_init((pmic_context_t**)&pmic_dev);
    predrv_spi_init((predrv_context_t**)&predrv_dev);

    wr_pmic((pmic_context_t*)pmic_dev, WR_SAFETY_CHECK_CTRL, 0x06);
    wr_pmic((pmic_context_t*)pmic_dev, SW_UNLOCK, 0x55);
    //Init Watchdog Window
    wr_pmic((pmic_context_t*)pmic_dev, WR_WD_WIN1_CFG, 0x0e);//14*0.55*1.05 = 8.05ms -- 13*0.55*0.95 = 6.79 ms
    wr_pmic((pmic_context_t*)pmic_dev, WR_WD_WIN2_CFG, 0x06);//(6+1)*0.55*1 = 3.85ms
    timer_task1();
    // wr_pmic((pmic_context_t*)pmic_dev, WR_DEV_CFG1, 0x80);
    // wr_pmic((pmic_context_t*)pmic_dev, WR_DEV_CFG2, 0xc0);
    wr_pmic((pmic_context_t*)pmic_dev, WR_SAFETY_ERR_CFG, 0x00);
    // wr_pmic((pmic_context_t*)pmic_dev, WR_SAFETY_FUNC_CFG, 0x09);
    // wr_pmic((pmic_context_t*)pmic_dev, WR_SAFETY_ERR_PWM_H, 0x00);
    // wr_pmic((pmic_context_t*)pmic_dev, WR_SAFETY_ERR_PWM_L, 0x00);
    // wr_pmic((pmic_context_t*)pmic_dev, WR_SAFETY_PWD_THR_CFG, 0x01);
    // wr_pmic((pmic_context_t*)pmic_dev, WR_SAFETY_CFG_CRC, 0x10);
    //wr_pmic((pmic_context_t*)pmic_dev, WR_SAFETY_CHECK_CTRL, 0x25);

    //wr_pmic((pmic_context_t*)pmic_dev, WR_SAFETY_BIST_CTRL, 0xe0);
    int x=1;
    wr_predrv((predrv_context_t*)predrv_dev, WR_CMD1, 0x3ff);

    while(1)
    {
    int dev_id = rd_pmic((pmic_context_t*)pmic_dev, RD_DEV_ID);  
    int dev_rev = rd_pmic((pmic_context_t*)pmic_dev, RD_DEV_REV);   
    int dev_state = rd_pmic((pmic_context_t*)pmic_dev, RD_DEV_STAT);
    int vmon_state_1 = rd_pmic((pmic_context_t*)pmic_dev, RD_VMON_STAT_1);
    int vmon_state_2 = rd_pmic((pmic_context_t*)pmic_dev, RD_VMON_STAT_2);
    
    int safety_state_1 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_1);  
    int safety_state_2 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_2);
    int safety_state_3 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_3);
    int safety_state_4 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_4);
    int safety_state_5 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_5);

    int safety_error_state = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_ERR_STAT);
    int watchdog_state = rd_pmic((pmic_context_t*)pmic_dev, RD_WD_STATUS);

    int safety_err_cfg = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_ERR_CFG);
    int safety_pwd_thr_cfg = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_PWD_THR_CFG);
    int safety_func_cfg= rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_FUNC_CFG);
    int safety_cfg_crc = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_CFG_CRC);

    printf("Predriver's CMD register is: %d;\r\n", wr_predrv((predrv_context_t*)predrv_dev, RD_CMD1, 0x000));

    printf("Device ID is %d;\r\n",dev_id);
    printf("Device Revision is %d;\r\n", dev_rev);
    printf("Device State is %d;\r\n",dev_state);
    printf("Device Safety State 1: %d;\r\n",safety_state_1);
    printf("Device Safety State 2: %d;\r\n",safety_state_2);
    printf("Device Safety State 3: %d;\r\n",safety_state_3);
    printf("Device Safety State 4: %d;\r\n",safety_state_4);
    printf("Device Safety State 5: %d;\r\n",safety_state_5);
    printf("Device Vmon State 1: %d;\r\n",vmon_state_1);
    printf("Device Vmon State 2: %d;\r\n",vmon_state_2 );
    printf("Device Safety Error State : %d;\r\n",safety_error_state);
    printf("Device Watchdog status: %d;\r\n",watchdog_state);
    printf("Device Safety Error Configuration: %d;\r\n",safety_err_cfg);
    printf("Device Safety Power down Threshold: %d;\r\n",safety_pwd_thr_cfg);
    printf("Device Safety Function Configuration: %d;\r\n",safety_func_cfg);
    printf("Device Safety Configuration CRC: %d;\r\n",safety_cfg_crc);
    printf("WIN1_CFG: %d;\r\n", rd_pmic((pmic_context_t*)pmic_dev, RD_WD_WIN1_CFG));
    printf("WIN2_CFG: %d;\r\n", rd_pmic((pmic_context_t*)pmic_dev, RD_WD_WIN2_CFG));
    printf("Safety Control: %d;\r\n", rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_CHECK_CTRL));
    //read_endrv();
    vTaskDelay(2000/portTICK_PERIOD_MS);
    x++;
    }
    //xTaskCreate(print1, "print1", 8192, NULL, 1, NULL);
    //xTaskCreate(print2, "print2", 8192, NULL, 1, NULL);


    //timer_task2();
    // while(1)
    // {
    //     uint64_t timer_val = 0;
    //     timer_get_counter_value(TIMER_GROUP_0,TIMER_0, &timer_val);
    //     if (timer_val == 9000)
    //     {
    //       gpio_set_level(WDI, 1);
    //     }
    //     //int safety_state_2 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_2);
    //     //printf("Device Safety State 2: %d;\r\n",safety_state_2);

    // }

    // int level = 0;
    // while(1)
    // {
    //     int evt;
    //     xQueueReceive(s_timer_queue, &evt, portMAX_DELAY);
    //     level = evt^level;
    //     gpio_set_level(WDI, level);
    //     printf("Received value is %d\n",level);
    //     vTaskDelay(10/portTICK_PERIOD_MS);
    // }
   // xTaskCreate(get_wtd_stat, "get_wtd_stat", 8192, NULL, 1, NULL);

}
