/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-13 18:04:36
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 22:20:06
 * @FilePath: \GD32F103C8T6\App\app.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _APP_H  // 防止头文件重复包含，如果没有定义_APP_H宏
#define _APP_H  // 定义_APP_H宏，表示已经包含过此头文件

#include "FreeRTOS.h"  // 包含FreeRTOS内核头文件，定义FreeRTOS内核函数和宏
#include "task.h"       // 包含FreeRTOS任务管理头文件，定义任务创建、删除、调度等函数
#include "queue.h"      // 包含FreeRTOS队列头文件，定义队列创建、发送、接收等函数
#include "semphr.h"     // 包含FreeRTOS信号量头文件，定义信号量和互斥锁创建、获取、释放等函数
#include "gd32f10x.h"   // 包含GD32F10x系列芯片头文件，定义芯片寄存器和外设

#define TASK_PRIORITY_LED         (tskIDLE_PRIORITY + 2)  // 定义LED任务优先级，为空闲优先级+2
#define TASK_PRIORITY_OLED        (tskIDLE_PRIORITY + 2)  // 定义OLED任务优先级，为空闲优先级+2
#define TASK_PRIORITY_KEY         (tskIDLE_PRIORITY + 3)  // 定义按键任务优先级，为空闲优先级+3（最高优先级）
#define TASK_PRIORITY_SENSOR      (tskIDLE_PRIORITY + 1)  // 定义传感器任务优先级，为空闲优先级+1（最低优先级）
#define TASK_PRIORITY_ENCODER     (tskIDLE_PRIORITY + 2)  // 定义编码器任务优先级，为空闲优先级+2

#define TASK_STACK_SIZE_LED       64   // 定义LED任务栈大小，为64字（256字节）
#define TASK_STACK_SIZE_OLED      384  // 定义OLED任务栈大小，为384字（1536字节）
#define TASK_STACK_SIZE_KEY       64   // 定义按键任务栈大小，为64字（256字节）
#define TASK_STACK_SIZE_SENSOR    192  // 定义传感器任务栈大小，为192字（768字节）
#define TASK_STACK_SIZE_ENCODER   192  // 定义编码器任务栈大小，为192字（768字节）

typedef struct {  // 定义传感器数据结构体类型
    float temp0;      // 温度0（AHT20传感器读取的温度值）
    float temp1;      // 温度1（BMP280传感器读取的温度值）
    float humi;       // 湿度（AHT20传感器读取的湿度值）
    float press;       // 气压（BMP280传感器读取的气压值）
    uint16_t light;   // 光照强度（光照传感器读取的光照值）
    uint16_t soil;    // 土壤湿度（土壤湿度传感器读取的土壤湿度值）
    uint16_t rain;    // 降雨量（降雨量传感器读取的降雨量值）
} SensorData_t;  // 结构体类型名称为SensorData_t

typedef enum {  // 定义显示页面枚举类型
    PAGE_HOME = 0,  // 主页，显示传感器实时数据
    PAGE_RANGE = 1   // 阈值页，显示阈值设置界面
} DisplayPage_t;  // 枚举类型名称为DisplayPage_t

typedef struct {  // 定义农场安全阈值范围结构体类型
    float minTemperature;         // 温度最小值
    float maxTemperature;         // 温度最大值
    float minHumidity;           // 湿度最小值
    float maxHumidity;           // 湿度最大值
    uint16_t minLightIntensity;   // 光照最小值
    uint16_t maxLightIntensity;   // 光照最大值
    uint16_t minSoilMoisture;   // 土壤湿度最小值
    uint16_t maxSoilMoisture;   // 土壤湿度最大值
    uint16_t maxRainGauge;       // 降雨量最大值
} FarmSafeRange_t;  // 结构体类型名称为FarmSafeRange_t

typedef enum {  // 定义阈值编辑项索引枚举类型
    RANGE_EDIT_TEMPERATURE_MIN = 0,    // 温度最小值编辑项
    RANGE_EDIT_TEMPERATURE_MAX,          // 温度最大值编辑项
    RANGE_EDIT_HUMIDITY_MIN,           // 湿度最小值编辑项
    RANGE_EDIT_HUMIDITY_MAX,           // 湿度最大值编辑项
    RANGE_EDIT_LIGHT_INTENSITY_MIN,     // 光照最小值编辑项
    RANGE_EDIT_LIGHT_INTENSITY_MAX,     // 光照最大值编辑项
    RANGE_EDIT_SOIL_MOISTURE_MIN,      // 土壤湿度最小值编辑项
    RANGE_EDIT_SOIL_MOISTURE_MAX,      // 土壤湿度最大值编辑项
    RANGE_EDIT_RAIN_GAUGE_MAX,         // 降雨量最大值编辑项
    RANGE_EDIT_COUNT                   // 编辑项总数（用于循环判断）
} RangeEditIndex_t;  // 枚举类型名称为RangeEditIndex_t

typedef enum {  // 定义阈值编辑状态枚举类型
    RANGE_EDIT_STATE_BROWSING = 0,  // 浏览状态，只能查看和选择编辑项
    RANGE_EDIT_STATE_EDITING         // 编辑状态，可以修改当前编辑项的数值
} RangeEditState_t;  // 枚举类型名称为RangeEditState_t

extern uint8_t pumpState;  // 声明外部变量pumpState，表示水泵状态（0=关闭，1=开启）

#define FLASH_RANGE_CONFIG_ADDR  0x000000  // 定义Flash中阈值配置的存储地址为0x000000

void App_SaveRangeConfig(void);  // 声明函数App_SaveRangeConfig，用于保存阈值配置到Flash
void App_LoadRangeConfig(void);  // 声明函数App_LoadRangeConfig，用于从Flash加载阈值配置

void App_InitDrivers(void);       // 声明函数App_InitDrivers，用于初始化所有硬件驱动
void App_CreateTasks(void);       // 声明函数App_CreateTasks，用于创建FreeRTOS任务
void App_CreateQueues(void);      // 声明函数App_CreateQueues，用于创建FreeRTOS队列
void App_CreateSemaphores(void);  // 声明函数App_CreateSemaphores，用于创建FreeRTOS信号量

#endif  // 结束头文件包含保护
