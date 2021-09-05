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

//static QueueHandle_t xQueue;
static xQueueHandle s_timer_queue;

//vTaskDelay(6/portTICK_PERIOD_MS);

static bool IRAM_ATTR pmic_wtd(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    int evt=1;
    xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);
    return high_task_awoken == pdTRUE;
}

void timer_task(void)
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
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 10);
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);
  timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, pmic_wtd, NULL, 0);
  timer_start(TIMER_GROUP_0, TIMER_0);
}

void app_main(void)
{
    pmic_handle_t pmic_dev;
    s_timer_queue = xQueueCreate(10, sizeof(int));
    //blink();
    
    pmic_spi_init((pmic_context_t**)&pmic_dev);
    wr_pmic((pmic_context_t*)pmic_dev, WR_WD_WIN1_CFG, 0x64);
    wr_pmic((pmic_context_t*)pmic_dev, WR_WD_WIN2_CFG, 0x1e);

    
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

    timer_task();

    int level = 0;
    while(1)
    {
        int evt;
        xQueueReceive(s_timer_queue, &evt, portMAX_DELAY);
        level = evt^level;
        gpio_set_level(WDI, level);
        printf("Received value is %d\n",level);
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
   // xTaskCreate(get_wtd_stat, "get_wtd_stat", 8192, NULL, 1, NULL);

}
