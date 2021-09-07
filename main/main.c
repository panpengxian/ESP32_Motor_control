#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "tps65381a.h"
#include "driver/timer.h"
#include "sdkconfig.h"
#include "esp_task_wdt.h"

#define WDI GPIO_NUM_2

int wdi_level = 0;
//static QueueHandle_t xQueue;
static xQueueHandle s_timer_queue;

//vTaskDelay(6/portTICK_PERIOD_MS);

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

static bool IRAM_ATTR pmic_wtd2(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    gpio_set_level(WDI, 1);
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

void timer_task2(void)
{
  timer_config_t timer_cfg={
      .alarm_en = TIMER_ALARM_EN,
      .counter_en = TIMER_PAUSE,
      .counter_dir = TIMER_COUNT_UP,
      .auto_reload = TIMER_AUTORELOAD_EN,
      .divider = 80,
      //.clk_src = TIMER_SRC_CLK_APB,
      };
  timer_init(TIMER_GROUP_0, TIMER_1, &timer_cfg);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_1, 9000);
  timer_enable_intr(TIMER_GROUP_0, TIMER_1);
  timer_isr_callback_add(TIMER_GROUP_0, TIMER_1, pmic_wtd2, NULL, 0);
  timer_start(TIMER_GROUP_0, TIMER_1);
}

void print1(void)
{
    for(;;)
    {
    printf("%d\n",1);
    vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void print2(void)
{
    for(;;)
    {

    printf("%d\n",2);
    vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    pmic_handle_t pmic_dev;
    s_timer_queue = xQueueCreate(10, sizeof(int));
    //blink();
    
    pmic_spi_init((pmic_context_t**)&pmic_dev);

    wr_pmic((pmic_context_t*)pmic_dev, WR_SAFETY_CHECK_CTRL, 0x02);
    wr_pmic((pmic_context_t*)pmic_dev, SW_UNLOCK, 0x55);
    //Init Watchdog Window
    wr_pmic((pmic_context_t*)pmic_dev, WR_WD_WIN1_CFG, 0x0e);//14*0.55*1.05 = 8.05ms -- 13*0.55*0.95 = 6.79 ms
    wr_pmic((pmic_context_t*)pmic_dev, WR_WD_WIN2_CFG, 0x06);//(6+1)*0.55*1 = 3.85ms
    
    int dev_id = rd_pmic((pmic_context_t*)pmic_dev, RD_DEV_ID);  
    int dev_rev = rd_pmic((pmic_context_t*)pmic_dev, RD_DEV_REV);   
    int dev_state = rd_pmic((pmic_context_t*)pmic_dev, RD_DEV_STAT);
    int safety_state_1 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_1);  
    int safety_state_2 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_2);
    int safety_state_3 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_3);
    int safety_state_4 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_4);
    int safety_state_5 = rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_5);

    printf("Device ID is %d;\r\n",dev_id);
    printf("Device Revision is %d;\r\n", dev_rev);
    printf("Device State is %d;\r\n",dev_state);
    printf("Device Safety State 1: %d;\r\n",safety_state_1);
    printf("Device Safety State 2: %d;\r\n",safety_state_2);
    printf("Device Safety State 3: %d;\r\n",safety_state_3);
    printf("Device Safety State 4: %d;\r\n",safety_state_4);
    printf("Device Safety State 5: %d;\r\n",safety_state_5);

    //xTaskCreate(print1, "print1", 8192, NULL, 1, NULL);
    //xTaskCreate(print2, "print2", 8192, NULL, 1, NULL);

    timer_task1();
    while(1)
    {
        printf("Device Safety State 2: %d;\r\n", rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_STAT_2));
        printf("WIN1_CFG: %d;\r\n", rd_pmic((pmic_context_t*)pmic_dev, RD_WD_WIN1_CFG));
        printf("WIN2_CFG: %d;\r\n", rd_pmic((pmic_context_t*)pmic_dev, RD_WD_WIN2_CFG));
        printf("Safety Control: %d;\r\n", rd_pmic((pmic_context_t*)pmic_dev, RD_SAFETY_CHECK_CTRL));
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
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
