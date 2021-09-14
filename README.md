# Esp32_Motor_Control

#### 介绍
基于ESP32的电机控制系统，MCU为ESP32芯片，电源管理芯片为TI的TPS65381A-Q1，预驱芯片为ST的L9907，MOSFET为ST的sth270N8F7-6

#### L9907简介
L9907的SPI校验位奇校验
CHPA = 1；CPOL = 0；
TPS65381a的SPI校验为偶校验

什么是Odd Parity and Even Parity？
Odd Parity 是奇校验，Even Parity 是偶校验
奇校验：当传输数据中有奇数个‘1’时，校验位为0
偶校验：当传输数据中有偶数个‘1’时，校验位为0

供电部分：
L9906的VB为MOSFET驱动的电源电压为12V，Vcc为Logic模块的供电电压，可以为5V/3.3V，它由内部LDO转换为Vdd，给LOgic模块供电。
外围Boost电路以VDH为基准产生比电源电压12V高10V的驱动电压驱动高边驱动器，这个驱动电压经过VCAP电压转换器产生电压驱动低边驱动器。

通信接口：
L9907提供了四线制SPI通信接口，其CPHA=1，CPOL=0，数据长度为16bit，数据传输顺序为MSB first，其SPI校验位奇校验，即当传输数据中有奇数个‘1’时，校验位为0；
L9907的系统时钟最大值为8MHz，这也意味着L9907的通信速率不大于8MHz。

驱动器模块：
L9907的驱动器是限制电流的驱动器


L9907提供了一些指令来决定采用何种故障管理策略：切换CMD4和CMD1寄存器中的enable fault flags来决定故障管理策略

当确认某一错误时，相应的diagnostic flag会置高且FS_FLAG置低。设备会关断且根据enable fault flag它不会采取任何动作。

在一些情况中，由错误故障引起的shut-off会被关闭，这时芯片会为disabled fault完全负责shut-off management

在默认情况下，所有的enable fault flags置高


##### CMD0(Driver Setting)
B9,B8(Dead time parameter): 
(0,0):100-200ns
(0,1):300-500ns
(1,0):700-1000ns
(1,1):1000-1500ns
B7,B6()


#### 开发过程中的问题
ESP-IDF VSCODE编译时发生 undefined reference to 'func()'的问题，原因是CMakeLists.txt中没有包含c文件


