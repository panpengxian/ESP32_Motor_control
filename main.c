#include "stdio.h"
#include "string.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "font.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/timer.h"
#include "esp_http_client.h"

#define D0 GPIO_NUM_5 //D0 -- sclk
#define D1 GPIO_NUM_18 //D1 -- MOSI
#define RES GPIO_NUM_19
#define DC GPIO_NUM_21
#define CS GPIO_NUM_22
#define MISO GPIO_NUM_23
#define CMD 0
#define DATA 1
#define DEFAULT_SSID "ChinaNet-tmdk"//"iPhone"//"HUAWEI-2801"
#define DEFAULT_PWD "jisjugwk"//"As123456"
#define EXAMPLE_ESP_MAXIMUM_RETRY  10
#define DEFAULT_SCAN_METHOD WIFI_FAST_SCAN
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#define DEFAULT_RSSI -127 
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char TAG[] = "main";
static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;
esp_err_t ret;
QueueHandle_t xQueue;
spi_device_handle_t spi3_dev;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


void uart_com(void){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,

    };
    // gpio_config_t gpio_cfg = {
    //     .pin_bit_mask = BIT64(GPIO_NUM_3)|BIT64(GPIO_NUM_1),
    //     .mode = GPIO_MODE_INPUT_OUTPUT,
    // }
    // gpio_config(&gpio_cfg);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    int uart_buffer_size = (1024*2);
    QueueHandle_t uart_queue;
    uart_driver_install(UART_NUM_0, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0);
    while(1)
    {
    int Message[] = {0,1,2,3,4,5};
    uart_write_bytes(UART_NUM_0, (int*)Message, sizeof(Message));
    vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void feedWTD(void *pvParameters){
    while(1)
    {
        gpio_set_level(RES,1);
        vTaskDelay(1/portTICK_PERIOD_MS);
        gpio_set_level(RES,0);
        vTaskDelay(1/portTICK_PERIOD_MS);   
    }
}

void blink(void *pvParameters){
    while(1)
    {
        gpio_set_level(GPIO_NUM_2,1);
        vTaskDelay(100/portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_2,0);
        vTaskDelay(100/portTICK_PERIOD_MS);     
    }
}

void TPS65381A_Init(void)
{
    
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = BIT64(D0)|BIT64(D1)|BIT64(RES)|BIT64(DC)|BIT64(CS)|BIT64(GPIO_NUM_2)|BIT64(MISO),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    spi_bus_config_t spi_cfg = {
        .mosi_io_num = D1,
        .miso_io_num = MISO,
        .sclk_io_num = D0,
        .max_transfer_sz = 1024,
        .flags = SPICOMMON_BUSFLAG_GPIO_PINS|SPICOMMON_BUSFLAG_MASTER, 
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
    int re_code = spi_bus_initialize(SPI2_HOST, &spi_cfg, SPI_DMA_DISABLED);
    if(re_code == ESP_OK ){
        ESP_LOGI("SPI:","Bus initialized!");
    }
    else{
        ESP_LOGI("SPI:","Bus failed!\nerror code is %d\n",re_code);
    }
    re_code = spi_bus_add_device(SPI2_HOST, &spi_dev_if_cfg, &spi3_dev);
    if(re_code == ESP_OK){
        ESP_LOGI("SPI:","device added to bus!");
    }
    else{
        ESP_LOGI("SPI:","devive not added to device!\nerror code is %d\n",re_code);
    }
}

void TPS65381A_Sensor(void)
{
    spi_transaction_t spi_trans={
        .flags = SPI_TRANS_USE_RXDATA|SPI_TRANS_USE_TXDATA,
        .cmd = 0x06,//RD_REV_ID
        .length = 8,
        .rxlength = 8,
        .tx_data = {0x03,0x02,0x03,0x04},
        };
    gpio_set_level(CS, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi3_dev,&spi_trans));
    gpio_set_level(CS,1);
    for(int i=0;i<4;i++)
    {
        printf("%d\n",spi_trans.tx_data[i]);
        printf("%d\n",spi_trans.rx_data[i]);
    }   

}

void DEV_ID(void)
{
    spi_transaction_t spi_trans={
        .flags = SPI_TRANS_USE_RXDATA|SPI_TRANS_USE_TXDATA,
        .cmd = 0x0C,//RD_REV_ID
        .length = 8,
        .rxlength = 8,
        .tx_data = {0x02,0x02,0x03,0x04},
        };
    gpio_set_level(CS, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi3_dev,&spi_trans));
    gpio_set_level(CS,1);
    for(int i=0;i<4;i++)
    {
        printf("%d\n",spi_trans.tx_data[i]);
        printf("%d\n",spi_trans.rx_data[i]);
    }   

}

void DEV_STAT(void)
{
    spi_transaction_t spi_trans={
        .flags = SPI_TRANS_USE_RXDATA|SPI_TRANS_USE_TXDATA,
        .cmd = 0x11,//RD_DEV_STAT
        .length = 8,
        .rxlength = 8,
        .tx_data = {0x02,0x02,0x03,0x04},
        };
    gpio_set_level(CS, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi3_dev,&spi_trans));
    gpio_set_level(CS,1);
    ESP_LOGI("DEV_STAT:","%d",spi_trans.rx_data[0]);
    for(int i=0;i<4;i++)
    {
        //printf("%d\n",spi_trans.tx_data[i]);
        printf("%d\n",spi_trans.rx_data[i]);
    }   

}

void SAFETY_STAT_2(void)
{
    spi_transaction_t spi_trans={
        .flags = SPI_TRANS_USE_RXDATA|SPI_TRANS_USE_TXDATA,
        .cmd = 0xc5,//SAFETY_STAT_2
        .length = 8,
        .rxlength = 8,
        .tx_data = {0},
        };
    gpio_set_level(CS, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi3_dev,&spi_trans));
    gpio_set_level(CS,1);
    ESP_LOGI("SAFETY_STAT_2:","%x",spi_trans.rx_data[0]);  
}

void RD_WD_STATUS(void)
{
    spi_transaction_t spi_trans={
        .flags = SPI_TRANS_USE_RXDATA|SPI_TRANS_USE_TXDATA,
        .cmd = 0x4e,//SAFETY_STAT_2
        .length = 8,
        .rxlength = 8,
        .tx_data = {0},
        };
    gpio_set_level(CS, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi3_dev,&spi_trans));
    gpio_set_level(CS,1);
    ESP_LOGI("WD_STATUS:","%x",spi_trans.rx_data[0]);  
}

void WR_WD_WIN1_CFG(void)
{
    spi_transaction_t spi_trans={
        .flags = SPI_TRANS_USE_TXDATA,
        .cmd = 0xED,//SAFETY_STAT_2
        .length = 8,
        .tx_data = {0x0a},
        };
    gpio_set_level(CS, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi3_dev,&spi_trans));
    gpio_set_level(CS,1);
    ESP_LOGI("SAFETY_STAT_2:","%x",spi_trans.rx_data[0]);  
}

void WR_WD_WIN2_CFG(void)
{
    spi_transaction_t spi_trans={
        .flags = SPI_TRANS_USE_TXDATA,
        .cmd = 0x09,
        .length = 8,
        .tx_data = {0x0a},
        };
    gpio_set_level(CS, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi3_dev,&spi_trans));
    gpio_set_level(CS,1);

}


void get_wtd_stat(void *pvParameters)
{
    while(1)
    {
        for(int n=0;n<4;n++)
        {
        vTaskDelay(6/portTICK_PERIOD_MS);    
        gpio_set_level(RES,1);
        vTaskDelay(2/portTICK_PERIOD_MS); 
        gpio_set_level(RES,0);
        }
        SAFETY_STAT_2();
        RD_WD_STATUS();
    }
}

esp_err_t OLED_WR_Byte(int dat, int cmd, spi_device_handle_t spi_dev)
{   
    spi_transaction_t spi_trans={.cmd = dat,};
    esp_err_t ret;
    if (cmd)
    {
        gpio_set_level(DC, 1);
    }
        
    else
    {
        gpio_set_level(DC, 0);
    }

    gpio_set_level(CS, 0);
    ret = spi_device_polling_transmit(spi_dev,&spi_trans);
    gpio_set_level(CS,1);
    gpio_set_level(DC,0);
    return ret;
}

void OLED_Init(void){
    gpio_config_t gpio_cfg = {
    .pin_bit_mask = BIT64(D0)|BIT64(D1)|BIT64(RES)|BIT64(DC)|BIT64(CS)|BIT64(GPIO_NUM_2),
    .mode = GPIO_MODE_INPUT_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
    };

    spi_bus_config_t spi_cfg = {
    .mosi_io_num = D1,
    .miso_io_num = -1,
    .sclk_io_num = D0,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 0,
    .flags = SPICOMMON_BUSFLAG_GPIO_PINS, 
    };

    spi_device_interface_config_t spi_dev_if_cfg = {
    .command_bits = 8,
    .address_bits = 0,
    .mode = 0,
    .duty_cycle_pos = 128,
    .clock_speed_hz = SPI_MASTER_FREQ_8M,
    .spics_io_num = -1,
    .queue_size = 1,
    };
    gpio_config(&gpio_cfg);
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &spi_cfg, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &spi_dev_if_cfg, &spi3_dev));

    gpio_set_level(RES, 1);
    vTaskDelay(100/portTICK_PERIOD_MS); 
    gpio_set_level(RES, 0); 
    vTaskDelay(100/portTICK_PERIOD_MS); 
    gpio_set_level(RES, 1); 

    OLED_WR_Byte(0xAE, CMD, spi3_dev);//Set Display OFF
    OLED_WR_Byte(0x00, CMD, spi3_dev);
    OLED_WR_Byte(0x10, CMD, spi3_dev);//Set Column Start Address
    OLED_WR_Byte(0x40, CMD, spi3_dev);//Set Display Start Line
    OLED_WR_Byte(0x81, CMD, spi3_dev);//Set Contrast Control
    OLED_WR_Byte(0xcf, CMD, spi3_dev);
    OLED_WR_Byte(0xa1, CMD, spi3_dev);//column address 127 mapped to SEG0
    OLED_WR_Byte(0xc8, CMD, spi3_dev);//set com output scan direction
    OLED_WR_Byte(0xa6, CMD, spi3_dev);//set normal display 
    OLED_WR_Byte(0xa8, CMD, spi3_dev);//set multiplex ratio
    OLED_WR_Byte(0x3f, CMD, spi3_dev);
    OLED_WR_Byte(0xd3, CMD, spi3_dev);//set display offset
    OLED_WR_Byte(0x00, CMD, spi3_dev);
    OLED_WR_Byte(0xd5, CMD, spi3_dev);//set display clock
    OLED_WR_Byte(0x80, CMD, spi3_dev);
    OLED_WR_Byte(0xd9, CMD, spi3_dev);//set pre charge
    OLED_WR_Byte(0xf1, CMD, spi3_dev);
    OLED_WR_Byte(0xda, CMD, spi3_dev);//set com pins
    OLED_WR_Byte(0x12, CMD, spi3_dev);
    OLED_WR_Byte(0xdb, CMD, spi3_dev);//set comh deselect level
    OLED_WR_Byte(0x40, CMD, spi3_dev);
    OLED_WR_Byte(0x20, CMD, spi3_dev);//Setting Memory addressing mode   
    OLED_WR_Byte(0x02, CMD, spi3_dev);//Page addressing mode set
    OLED_WR_Byte(0x8d, CMD, spi3_dev);//Disable charge pump
    OLED_WR_Byte(0x14, CMD, spi3_dev);
    OLED_WR_Byte(0xa4, CMD, spi3_dev);//entire display on
    OLED_WR_Byte(0xa6, CMD, spi3_dev);
    OLED_WR_Byte(0xaf, CMD, spi3_dev);//Display On
}

void oled_clear(void){
    int row,col;
    for (row = 0; row<8;row++)
    {
        OLED_WR_Byte(0xb0+row, CMD, spi3_dev);
        OLED_WR_Byte(0x10, CMD, spi3_dev);
        OLED_WR_Byte(0x00, CMD, spi3_dev);
        for (col = 0; col < 128; col++)
        {
            OLED_WR_Byte(0x00, DATA, spi3_dev);
        }
    }
}

void OLED_Set_Position(int x, int y){
    ret = OLED_WR_Byte(y&0x0F,CMD,spi3_dev);//Set Lower Column Start address
    ret = OLED_WR_Byte(0x10+((y&0xF0)>>4),CMD,spi3_dev);//Set Higher Column Start address
    ret = OLED_WR_Byte(0xb0+x,CMD,spi3_dev);//Set Page Start address
}

void oled_display_num(char number){
    for (int n=0;n<6;n++)
    {
        OLED_WR_Byte(F6x8[number-32][n], DATA, spi3_dev);
    }
}

void oled_display_time(char* time, int position){
    for (int n = 0; n< strlen(time);n++){
        OLED_Set_Position(0,n*6+position);
        oled_display_num(time[n]);
    }
}

int get_cur_time(char* time, int position, char* sign, bool is_second)
{
    strtok(time,"=");
    time = strtok(NULL,"=");
    oled_display_time(time,position);   
    position += strlen(time)*6;
    if (is_second) {return position;}
    if(sign){
        oled_display_time(sign,position);
        position += strlen(sign)*6;
    }
    
    return position;

}

void http_connect(){
   ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());//Initialize the underlying TCP/IP stack
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));



    wifi_config_t wifi_config = {
        .sta = {
            .ssid = DEFAULT_SSID,
            .password = DEFAULT_PWD,
            .scan_method = DEFAULT_SCAN_METHOD,
            .sort_method = DEFAULT_SORT_METHOD,
            .threshold.rssi = DEFAULT_RSSI,
            .threshold.authmode = DEFAULT_AUTHMODE,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    ESP_ERROR_CHECK(esp_wifi_connect());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 DEFAULT_SSID, DEFAULT_PWD);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 DEFAULT_SSID, DEFAULT_PWD);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

void http_client(void *pvParameters){
    char time[1024] ={0};
    esp_http_client_config_t config = {
        .url = "http://www.beijing-time.org/t/time.asp",
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    while(1)
    {
        ESP_ERROR_CHECK(esp_http_client_open(client,0));
        int content_length = esp_http_client_fetch_headers(client);
        int data_len = esp_http_client_read_response(client, time, 1024);
        if(data_len > 0 && content_length>0){
            BaseType_t xStatus = xQueueSendToBack(xQueue, time, 0);
            if (xStatus != pdPASS)
            {
                printf("Cound not send to the queue.\n");
            }
        }
        esp_http_client_close(client); 
        vTaskDelay(pdMS_TO_TICKS(1));      
    }   
}

void send_string(void *pvParameters){
    BaseType_t xStatus;
    int a = (int)pvParameters;
    for(;;)
    {
        a += 1;
        xStatus = xQueueSendToBack(xQueue, &a, portMAX_DELAY);
        if (xStatus != pdPASS)
        {
            printf("Cound not send to the queue.\n");
        }
    }
}

void print(void *pvParameters){
    char time[1024] = {}; 
    while(1)
    {
        if( uxQueueMessagesWaiting( xQueue ) != 0)
        {
            printf("The queue should have been empty!\n");
        }
        BaseType_t xStatus = xQueueReceive( xQueue, time, pdMS_TO_TICKS(500));
        if ( xStatus == pdPASS)
        {
            //oled_clear();
            strtok(time,";\r\n");
            char* nyear = strtok(NULL,";\r\n");
            char* nmonth = strtok(NULL,";\r\n");
            char* nday = strtok(NULL,";\r\n");
            strtok(NULL,";\r\n");
            char* nhrs = strtok(NULL,";\r\n");
            char* nmin = strtok(NULL,";\r\n");
            char* nsecs = strtok(NULL,";\r\n");
            int position = 0;
            position = get_cur_time(nyear, position, "/", false);
            position = get_cur_time(nmonth, position, "/",false);           
            position = get_cur_time(nday, position, " ",false);           
            position = get_cur_time(nhrs, position, ":",false);
            position = get_cur_time(nmin, position, ":",false);
            position = get_cur_time(nsecs, position, "", true);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        else
        {
            printf("Could not reveive from the queue.\n");
        }
        
    }
}

static xQueueHandle s_timer_queue;

static bool IRAM_ATTR light(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    int evt=1;
    xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);    
    return high_task_awoken == pdTRUE;
}

void timer(){
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
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, light, NULL, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);
}

static xQueueHandle s_timer_queue;

void app_main(void)
{  
    s_timer_queue = xQueueCreate(10, sizeof(int));
    TPS65381A_Init();
    timer();
    while(1)
    {
        int evt;
        xQueueReceive(s_timer_queue, &evt, portMAX_DELAY);
        printf("Received value is %d\n",evt);
    }
    //for(;;)
    //{int a=1;}
    //s_timer_queue = xQueueCreate(10, sizeof(int));

    //TPS65381A_Init();
    //WR_WD_WIN1_CFG();
    //WR_WD_WIN2_CFG();
    //xTaskCreate(blink, "blink", 1000, NULL, 1, NULL);
    //xTaskCreate(feedWTD, "feedWTD", 1000, NULL, 1, NULL);
   // xTaskCreate(get_wtd_stat, "get_wtd_stat", 8192, NULL, 1, NULL);
    
    //TPS65381A_Sensor();
    //DEV_ID();
    //DEV_STAT();
    //SAFETY_STAT_2();
    // xQueue = xQueueCreate(10,1024);
    // OLED_Init();
    // oled_clear();
    // http_connect(); 
    // if (xQueue!=NULL)
    // {
    //     xTaskCreate(http_client, "http_client", 10000, NULL, 5, NULL);
    //     xTaskCreate(print, "print", 10000, NULL, 5, NULL);
    // }
    //uart_com();



}
