/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-13 18:04:36
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 22:22:30
 * @FilePath: \GD32F103C8T6\App\app.c
 * @Description: 应用层核心文件，负责FreeRTOS内核对象创建、任务创建、驱动初始化和配置管理
 */

#include "app_shared.h"              // 包含应用层共享头文件，定义全局变量和任务函数声明
#include "driver_led/driver_led.h"    // 包含LED驱动头文件，提供LED初始化和控制函数
#include "driver_w25q64/driver_w25q64.h"  // 包含W25Q64 Flash驱动头文件，提供Flash读写函数
#include "driver_usart/driver_usart.h"    // 包含串口驱动头文件，提供串口配置函数
#include "driver_bluetooth/driver_bluetooth.h"  // 包含蓝牙驱动头文件，提供蓝牙配置函数
#include "driver_timer/driver_timer.h"  // 包含定时器驱动头文件，提供定时器配置函数
#include "driver_oled/driver_oled.h"    // 包含OLED驱动头文件，提供OLED显示函数
#include "driver_encoder/driver_encoder.h"  // 包含编码器驱动头文件，提供编码器初始化函数
#include "driver_light/iic_light.h"    // 包含光照传感器驱动头文件，提供光照传感器函数
#include "driver_aht20/driver_aht20.h" // 包含AHT20温湿度传感器驱动头文件，提供温湿度读取函数
#include "driver_bmp280/driver_bmp280.h"  // 包含BMP280气压传感器驱动头文件，提供气压读取函数
#include "driver_key/driver_key.h"      // 包含按键驱动头文件，提供按键初始化和扫描函数
#include "driver_buzzer/driver_buzzer.h"  // 包含蜂鸣器驱动头文件，提供蜂鸣器PWM控制函数
#include "driver_adc/driver_adc.h"      // 包含ADC驱动头文件，提供ADC多通道DMA采集函数
#include "delay.h"                      // 包含延时函数头文件，提供延时初始化和延时函数
#include <string.h>                    // 包含字符串处理头文件，提供memcpy等字符串函数

/* ==================== FreeRTOS内核对象定义 ==================== */

/**
 * @brief 传感器数据队列
 * @note 队列长度为1，用于Sensor_Task向OLED_Task传递传感器数据
 *       使用xQueueOverwrite()实现最新数据覆盖，确保OLED显示最新数据
 */
QueueHandle_t SensorDataQueue = NULL;  // 定义传感器数据队列句柄，初始化为NULL

/**
 * @brief 页面事件队列
 * @note 队列长度为5，用于Key_Task向OLED_Task传递页面切换事件
 *       当用户按下KEY1时，发送新的页面索引
 */
QueueHandle_t PageEventQueue = NULL;   // 定义页面事件队列句柄，初始化为NULL

/**
 * @brief OLED互斥锁
 * @note 保护OLED显示资源，防止多个任务同时操作OLED导致显示混乱
 *       使用方法：xSemaphoreTake(OLED_Mutex, timeout) / xSemaphoreGive(OLED_Mutex)
 */
SemaphoreHandle_t OLED_Mutex = NULL;   // 定义OLED互斥锁句柄，初始化为NULL

/* ==================== 全局变量定义 ==================== */

int16_t Num = 0;                      // 定义编码器数值变量，初始值为0
uint16_t Light = 0;                   // 定义光照强度变量，初始值为0
uint8_t oled_dirty = 1;               // 定义OLED显示脏标志，1表示需要刷新，0表示无需刷新
uint8_t pumpState = 0;                // 定义水泵状态变量，0表示关闭，1表示开启

/**
 * @brief 农场安全阈值范围配置
 * @note 默认阈值：
 *       - 温度：20.0°C ~ 30.0°C
 *       - 湿度：40.0% ~ 60.0%
 *       - 光照：0 ~ 1000
 *       - 土壤湿度：30% ~ 70%
 *       - 降雨量：0 ~ 50%
 */
FarmSafeRange_t farmSafeRange = {   // 定义农场安全阈值范围结构体变量，初始化为默认值
    .minTemperature = 20.0f,         // 温度最小值20.0°C
    .maxTemperature = 30.0f,         // 温度最大值30.0°C
    .minHumidity = 40.0f,            // 湿度最小值40.0%
    .maxHumidity = 60.0f,            // 湿度最大值60.0%
    .minLightIntensity = 0,          // 光照最小值0
    .maxLightIntensity = 1000,       // 光照最大值1000
    .minSoilMoisture = 30,           // 土壤湿度最小值30%
    .maxSoilMoisture = 70,           // 土壤湿度最大值70%
    .maxRainGauge = 50               // 降雨量最大值50%
};

RangeEditIndex_t rangeEditIndex = RANGE_EDIT_TEMPERATURE_MIN;  // 定义当前编辑的阈值项索引，初始为温度最小值
RangeEditState_t rangeEditState = RANGE_EDIT_STATE_BROWSING;   // 定义阈值编辑状态，初始为浏览状态
uint8_t isEditingPage = 0;                                      // 定义是否在编辑页面标志，0表示不在编辑页面，1表示在编辑页面
DisplayPage_t currentPage = PAGE_HOME;                          // 定义当前显示页面，初始为主页
uint8_t buzzerAlarmEnabled = 1;                                 // 定义蜂鸣器警报开关，1表示开启，0表示关闭

/* ==================== FreeRTOS钩子函数 ==================== */

/**
 * @brief 任务栈溢出钩子函数
 * @param xTask 发生栈溢出的任务句柄
 * @param pcTaskName 发生栈溢出的任务名称
 * @note 当任务栈溢出时，FreeRTOS会自动调用此函数
 *       此函数会关闭所有中断，点亮LED0和LED1，然后进入死循环
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    taskDISABLE_INTERRUPTS();        // 关闭所有中断，防止其他任务干扰
    
    gpio_bit_write(LED0_GPIO_PORT, LED0_GPIO_PIN, SET);  // 点亮LED0，指示栈溢出错误
    gpio_bit_write(LED1_GPIO_PORT, LED1_GPIO_PIN, SET);  // 点亮LED1，指示栈溢出错误
    
    for(;;)                         // 进入死循环，防止程序继续执行
    {
    }
}

/* ==================== 应用层初始化函数 ==================== */

/**
 * @brief 初始化所有硬件驱动
 * @note 初始化顺序：
 *       1. NVIC优先级分组（抢占优先级4位，子优先级0位）
 *       2. LED驱动
 *       3. 延时功能
 *       4. 串口通信
 *       5. 蓝牙模块
 *       6. 定时器2（用于系统滴答）
 *       7. OLED显示屏
 *       8. 编码器
 *       9. 光照传感器
 *       10. AHT20温湿度传感器
 *       11. BMP280气压传感器
 *       12. 按键
 *       13. W25Q64 Flash存储
 *       14. 加载阈值配置
 *       15. 蜂鸣器PWM
 *       16. ADC多通道DMA
 */
void App_InitDrivers(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);  // 设置NVIC优先级分组为抢占优先级4位，子优先级0位
    
    led_init();                    // 初始化LED驱动，配置LED引脚为输出模式
    DelayInit();                   // 初始化延时功能，配置系统滴答定时器
    usart_config();                // 配置串口通信，设置波特率和数据格式
    blt_config();                  // 配置蓝牙模块，初始化蓝牙通信
    timer2_config();               // 配置定时器2，用于系统滴答或其他定时功能
    OLED_Init();                   // 初始化OLED显示屏，配置I2C通信和显示参数
    Encoder_Init();                // 初始化编码器，配置编码器引脚和中断
    IIC_Light_Init();              // 初始化光照传感器，配置I2C通信
    AHT20_Init();                  // 初始化AHT20温湿度传感器，配置I2C通信
    BMP280_Init();                 // 初始化BMP280气压传感器，配置I2C通信
    Key_Init();                    // 初始化按键，配置按键引脚和中断
    W25Q64_Init();                 // 初始化W25Q64 Flash存储，配置SPI通信
    App_LoadRangeConfig();         // 从Flash加载阈值配置，恢复用户设置的阈值
    Buzzer_PWM_Init();             // 初始化蜂鸣器PWM，配置PWM输出
    ADC_DMA_MultiChannel_Init();   // 初始化ADC多通道DMA采集，配置ADC和DMA
}

/**
 * @brief 创建FreeRTOS队列
 * @note 队列创建必须在任务创建之前完成
 *       - SensorDataQueue: 长度1，用于传递传感器数据
 *       - PageEventQueue: 长度5，用于传递页面切换事件
 */
void App_CreateQueues(void)
{
    SensorDataQueue = xQueueCreate(1, sizeof(SensorData_t));  // 创建传感器数据队列，长度为1，元素大小为SensorData_t结构体大小
    PageEventQueue = xQueueCreate(5, sizeof(DisplayPage_t));  // 创建页面事件队列，长度为5，元素大小为DisplayPage_t枚举大小

    configASSERT(SensorDataQueue != NULL);  // 断言传感器数据队列创建成功，如果失败则进入错误处理
    configASSERT(PageEventQueue != NULL);   // 断言页面事件队列创建成功，如果失败则进入错误处理
}

/**
 * @brief 创建FreeRTOS信号量
 * @note 信号量创建必须在任务创建之前完成
 *       - OLED_Mutex: 互斥锁，保护OLED显示资源
 */
void App_CreateSemaphores(void)
{
    OLED_Mutex = xSemaphoreCreateMutex();  // 创建OLED互斥锁，用于保护OLED显示资源

    configASSERT(OLED_Mutex != NULL);      // 断言OLED互斥锁创建成功，如果失败则进入错误处理
}

/**
 * @brief 创建FreeRTOS任务
 * @note 任务创建必须在调度器启动之前完成
 *       任务优先级（数值越大优先级越高）：
 *       - Key_Task: 优先级3（最高），确保按键响应及时
 *       - LED_Task: 优先级2，系统运行指示
 *       - OLED_Task: 优先级2，显示更新
 *       - Encoder_Task: 优先级2，编码器处理
 *       - Sensor_Task: 优先级1（最低），传感器数据采集
 */
void App_CreateTasks(void)
{
    xTaskCreate(LED_Task, "LED", TASK_STACK_SIZE_LED, NULL, TASK_PRIORITY_LED, NULL);  // 创建LED任务，任务名为"LED"，栈大小为TASK_STACK_SIZE_LED，优先级为TASK_PRIORITY_LED
    xTaskCreate(OLED_Task, "OLED", TASK_STACK_SIZE_OLED, NULL, TASK_PRIORITY_OLED, NULL);  // 创建OLED任务，任务名为"OLED"，栈大小为TASK_STACK_SIZE_OLED，优先级为TASK_PRIORITY_OLED
    xTaskCreate(Key_Task, "Key", TASK_STACK_SIZE_KEY, NULL, TASK_PRIORITY_KEY, NULL);  // 创建按键任务，任务名为"Key"，栈大小为TASK_STACK_SIZE_KEY，优先级为TASK_PRIORITY_KEY
    xTaskCreate(Sensor_Task, "Sensor", TASK_STACK_SIZE_SENSOR, NULL, TASK_PRIORITY_SENSOR, NULL);  // 创建传感器任务，任务名为"Sensor"，栈大小为TASK_STACK_SIZE_SENSOR，优先级为TASK_PRIORITY_SENSOR
    xTaskCreate(Encoder_Task, "Encoder", TASK_STACK_SIZE_ENCODER, NULL, TASK_PRIORITY_ENCODER, NULL);  // 创建编码器任务，任务名为"Encoder"，栈大小为TASK_STACK_SIZE_ENCODER，优先级为TASK_PRIORITY_ENCODER
}

/* ==================== 配置管理函数 ==================== */

/**
 * @brief 保存阈值配置到Flash
 * @note 将farmSafeRange结构体保存到W25Q64 Flash的指定地址
 *       保存流程：
 *       1. 将结构体数据复制到缓冲区
 *       2. 擦除Flash扇区
 *       3. 写入新数据
 */
void App_SaveRangeConfig(void)
{
    uint8_t buffer[sizeof(FarmSafeRange_t)];  // 定义缓冲区，大小为FarmSafeRange_t结构体大小
    
    memcpy(buffer, &farmSafeRange, sizeof(FarmSafeRange_t));  // 将farmSafeRange结构体数据复制到缓冲区
    
    W25Q64_EraseSector(FLASH_RANGE_CONFIG_ADDR);  // 擦除Flash扇区，地址为FLASH_RANGE_CONFIG_ADDR
    
    W25Q64_PageProgram(FLASH_RANGE_CONFIG_ADDR, buffer, sizeof(FarmSafeRange_t));  // 将缓冲区数据写入Flash，地址为FLASH_RANGE_CONFIG_ADDR，长度为FarmSafeRange_t结构体大小
}

/**
 * @brief 从Flash加载阈值配置
 * @note 从W25Q64 Flash读取阈值配置，并进行有效性验证
 *       验证规则：
 *       - 温度范围：0~100°C，最小值<最大值
 *       - 湿度范围：0~100%，最小值<最大值
 *       - 光照范围：最小值<最大值
 *       - 土壤湿度范围：0~100%，最小值<最大值
 *       - 降雨量范围：0~100%
 *       如果验证失败，保持默认配置不变
 */
void App_LoadRangeConfig(void)
{
    uint8_t buffer[sizeof(FarmSafeRange_t)];  // 定义缓冲区，大小为FarmSafeRange_t结构体大小
    FarmSafeRange_t tempRange;                // 定义临时结构体变量，用于存储从Flash读取的数据
    
    W25Q64_ReadData(FLASH_RANGE_CONFIG_ADDR, buffer, sizeof(FarmSafeRange_t));  // 从Flash读取数据到缓冲区，地址为FLASH_RANGE_CONFIG_ADDR，长度为FarmSafeRange_t结构体大小
    
    memcpy(&tempRange, buffer, sizeof(FarmSafeRange_t));  // 将缓冲区数据复制到临时结构体变量
    
    if (tempRange.minTemperature > 0 && tempRange.minTemperature < 100 &&  // 验证温度最小值在0~100°C之间
        tempRange.maxTemperature > 0 && tempRange.maxTemperature < 100 &&  // 验证温度最大值在0~100°C之间
        tempRange.minTemperature < tempRange.maxTemperature &&  // 验证温度最小值小于最大值
        tempRange.minHumidity > 0 && tempRange.minHumidity < 100 &&  // 验证湿度最小值在0~100%之间
        tempRange.maxHumidity > 0 && tempRange.maxHumidity < 100 &&  // 验证湿度最大值在0~100%之间
        tempRange.minHumidity < tempRange.maxHumidity &&  // 验证湿度最小值小于最大值
        tempRange.minLightIntensity < tempRange.maxLightIntensity &&  // 验证光照最小值小于最大值
        tempRange.minSoilMoisture > 0 && tempRange.minSoilMoisture < 100 &&  // 验证土壤湿度最小值在0~100%之间
        tempRange.maxSoilMoisture > 0 && tempRange.maxSoilMoisture < 100 &&  // 验证土壤湿度最大值在0~100%之间
        tempRange.minSoilMoisture < tempRange.maxSoilMoisture &&  // 验证土壤湿度最小值小于最大值
        tempRange.maxRainGauge > 0 && tempRange.maxRainGauge < 100)  // 验证降雨量在0~100%之间
    {
        memcpy(&farmSafeRange, &tempRange, sizeof(FarmSafeRange_t));  // 所有验证通过，将临时结构体数据复制到全局结构体变量
    }
}
