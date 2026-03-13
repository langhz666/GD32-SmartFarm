# FreeRTOS 移植建议 - GD32F103C8T6

## ? 当前项目分析

你的项目特点：
- **芯片**：GD32F103C8T6（ARM Cortex-M3，64KB Flash，20KB RAM）
- **已有驱动**：LED、OLED、按键、ADC、蓝牙、W25Q64 Flash、温湿度传感器、气压传感器等
- **当前架构**：裸机轮询 + 中断

## ? FreeRTOS 移植建议

### 1. FreeRTOS 版本选择

推荐使用 **FreeRTOS V10.4.6** 或更高版本，这是最稳定的版本之一。

### 2. 核心移植步骤

#### 步骤 1：获取 FreeRTOS 源码

- 从官网下载：https://www.freertos.org/
- 或者使用 GitHub：https://github.com/FreeRTOS/FreeRTOS

#### 步骤 2：项目结构建议

```
GD32F103C8T6/
├── FreeRTOS/
│   ├── Source/
│   │   ├── include/           # FreeRTOS 头文件
│   │   ├── portable/          # 移植相关文件
│   │   │   ├── GCC/ARM_CM3/   # Cortex-M3 移植文件
│   │   │   │   ├── port.c
│   │   │   │   └── portmacro.h
│   │   │   └── MemMang/       # 内存管理
│   │   │       └── heap_4.c   # 推荐 heap_4
│   │   ├── tasks.c
│   │   ├── queue.c
│   │   ├── timers.c
│   │   └── ...
│   └── FreeRTOSConfig.h      # 配置文件（需要自己创建）
```

#### 步骤 3：需要修改的文件

**A. 修改 `gd32f10x_it.c`**

```c
// SysTick_Handler 改为调用 FreeRTOS 的系统滴答处理
extern void xPortSysTickHandler(void);
void SysTick_Handler(void)
{
    #if (INCLUDE_xTaskGetSchedulerState != 0)
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    #endif
    {
        xPortSysTickHandler();
    }
}

// SVC_Handler、PendSV_Handler 需要删除或注释掉
// FreeRTOS 会自己实现这些中断处理函数
```

**B. 修改 `main.c`**
```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// 任务优先级定义
#define TASK_PRIORITY_LED         (tskIDLE_PRIORITY + 2)
#define TASK_PRIORITY_OLED        (tskIDLE_PRIORITY + 2)
#define TASK_PRIORITY_KEY         (tskIDLE_PRIORITY + 3)
#define TASK_PRIORITY_SENSOR      (tskIDLE_PRIORITY + 1)
#define TASK_PRIORITY_BLUETOOTH   (tskIDLE_PRIORITY + 1)

// 任务堆栈大小
#define TASK_STACK_SIZE_LED       128
#define TASK_STACK_SIZE_OLED      256
#define TASK_STACK_SIZE_KEY       128
#define TASK_STACK_SIZE_SENSOR    256
#define TASK_STACK_SIZE_BLUETOOTH 256

// 任务句柄
TaskHandle_t LED_TaskHandle = NULL;
TaskHandle_t OLED_TaskHandle = NULL;
TaskHandle_t Key_TaskHandle = NULL;
TaskHandle_t Sensor_TaskHandle = NULL;
TaskHandle_t Bluetooth_TaskHandle = NULL;

// 任务函数声明
void LED_Task(void *pvParameters);
void OLED_Task(void *pvParameters);
void Key_Task(void *pvParameters);
void Sensor_Task(void *pvParameters);
void Bluetooth_Task(void *pvParameters);

int main(void)
{
    // 硬件初始化
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);  // FreeRTOS 要求
    led_init();
    DelayInit();
    usart_config();
    blt_config();
    timer2_config();
    OLED_Init();
    Encoder_Init();
    IIC_Light_Init();
    AHT20_Init();
    BMP280_Init();
    Key_Init();
    W25Q64_Init();
    Buzzer_PWM_Init();
    ADC_DMA_MultiChannel_Init();

    // 创建任务
    xTaskCreate(LED_Task, "LED", TASK_STACK_SIZE_LED, NULL,
                TASK_PRIORITY_LED, &LED_TaskHandle);

    xTaskCreate(OLED_Task, "OLED", TASK_STACK_SIZE_OLED, NULL,
                TASK_PRIORITY_OLED, &OLED_TaskHandle);

    xTaskCreate(Key_Task, "Key", TASK_STACK_SIZE_KEY, NULL,
                TASK_PRIORITY_KEY, &Key_TaskHandle);

    xTaskCreate(Sensor_Task, "Sensor", TASK_STACK_SIZE_SENSOR, NULL,
                TASK_PRIORITY_SENSOR, &Sensor_TaskHandle);

    xTaskCreate(Bluetooth_Task, "Bluetooth", TASK_STACK_SIZE_BLUETOOTH, NULL,
                TASK_PRIORITY_BLUETOOTH, &Bluetooth_TaskHandle);

    // 启动调度器
    vTaskStartScheduler();

    // 如果程序运行到这里，说明启动失败
    while(1);
}
```

#### 步骤 4：创建 FreeRTOSConfig.h
```c
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configTICK_RATE_HZ                      1000
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                128
#define configTOTAL_HEAP_SIZE                   15360  // 15KB，根据实际RAM调整
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                   0
#define configUSE_TIME_SLICING                 1
#define configUSE_NEWLIB_REENTRANT             0
#define configENABLE_BACKWARD_COMPATIBILITY     1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

#define configCHECK_FOR_STACK_OVERFLOW         2
#define configRECORD_STACK_HIGH_ADDRESS         1

#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

#define configASSERT(x) if((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet              1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY     5
#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configPRIO_BITS                         4

#define xPortPendSVHandler                      PendSV_Handler
#define vPortSVCHandler                         SVC_Handler

#endif /* FREERTOS_CONFIG_H */
```

### 3. 任务划分建议

基于你的硬件，建议这样划分任务：

| 任务 | 功能 | 优先级 | 周期 |
|------|------|--------|------|
| **LED_Task** | LED 状态指示、闪烁控制 | 中 | 100ms |
| **OLED_Task** | OLED 显示刷新 | 中 | 200ms |
| **Key_Task** | 按键扫描、编码器读取 | 高 | 10ms |
| **Sensor_Task** | 传感器数据采集（温湿度、气压、光照等） | 低 | 1000ms |
| **Bluetooth_Task** | 蓝牙通信处理 | 低 | 异步 |
| **ADC_Task** | ADC 数据采集（如需要） | 低 | 100ms |

### 4. 资源管理建议

#### 使用队列进行任务间通信

```c
// 创建队列
QueueHandle_t SensorDataQueue;
QueueHandle_t KeyEventQueue;

// 在 main.c 中创建
SensorDataQueue = xQueueCreate(5, sizeof(SensorData_t));
KeyEventQueue = xQueueCreate(10, sizeof(uint8_t));
```

#### 使用互斥量保护共享资源

```c
// OLED 显示互斥量
SemaphoreHandle_t OLED_Mutex;

// 创建
OLED_Mutex = xSemaphoreCreateMutex();

// 使用
if (xSemaphoreTake(OLED_Mutex, portMAX_DELAY) == pdTRUE)
{
    OLED_ShowString(1, 1, "Data", OLED_8X16);
    OLED_Update();
    xSemaphoreGive(OLED_Mutex);
}
```

### 5. 注意事项

?? **重要提醒：**

1. **NVIC 优先级分组**：必须设置为 `NVIC_PRIGROUP_PRE4_SUB0`，FreeRTOS 要求
2. **SysTick**：FreeRTOS 会接管 SysTick，需要修改中断处理
3. **中断优先级**：使用 FreeRTOS API 的中断优先级必须高于 `configMAX_SYSCALL_INTERRUPT_PRIORITY`
4. **堆栈大小**：GD32F103C8T6 只有 20KB RAM，需要合理分配堆栈和堆
5. **Printf 重定向**：如果使用 printf，需要注意重定向到 FreeRTOS 的串口输出
6. **W25Q64 Flash**：Flash 操作时间较长，建议在单独任务中进行

### 6. 内存分配建议

GD32F103C8T6 RAM 只有 20KB，建议：
- **FreeRTOS 堆**：15KB（configTOTAL_HEAP_SIZE）
- **任务堆栈**：每个任务 128-256 words（512-1024 bytes）
- **全局变量**：保留 2-3KB

### 7. 测试建议

移植完成后，按以下顺序测试：
1. 创建简单任务（LED 闪烁）
2. 测试任务切换
3. 测试队列通信
4. 测试互斥量
5. 逐步添加各个驱动

### 8. 示例任务代码

#### LED 任务示例

```c
void LED_Task(void *pvParameters)
{
    while(1)
    {
        led_toggle(1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

#### OLED 任务示例
```c
void OLED_Task(void *pvParameters)
{
    char buffer[32];

    while(1)
    {
        if (xSemaphoreTake(OLED_Mutex, portMAX_DELAY) == pdTRUE)
        {
            OLED_Clear();
            sprintf(buffer, "Num:%d", Num);
            OLED_ShowString(1, 1, buffer, OLED_8X16);
            OLED_Update();
            xSemaphoreGive(OLED_Mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

#### 按键任务示例
```c
void Key_Task(void *pvParameters)
{
    uint8_t key;

    while(1)
    {
        key = Key_Scan(0);
        if (key)
        {
            xQueueSend(KeyEventQueue, &key, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

#### 传感器任务示例
```c
void Sensor_Task(void *pvParameters)
{
    SensorData_t data;

    while(1)
    {
        AHT20_Read(&data.temp, &data.humi);
        BMP280_Read(&data.press);
        data.light = Light_Read();

        xQueueSend(SensorDataQueue, &data, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

#### 蓝牙任务示例
```c
void Bluetooth_Task(void *pvParameters)
{
    uint8_t rx_data;

    while(1)
    {
        if (usart_receive_flag)
        {
            rx_data = usart_receive_data;
            usart_receive_flag = 0;

            // 处理接收到的数据
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### 9. 常见问题解决

#### 问题 1：程序启动后卡死
- 检查 NVIC 优先级分组是否正确
- 检查堆栈大小是否足够
- 检查 FreeRTOSConfig.h 配置是否正确

#### 问题 2：任务不切换

- 检查是否调用了 vTaskStartScheduler()
- 检查任务中是否有死循环且没有调用 vTaskDelay()

#### 问题 3：堆栈溢出

- 增加任务堆栈大小
- 启用堆栈溢出检测（configCHECK_FOR_STACK_OVERFLOW）

#### 问题 4：中断不响应

- 检查中断优先级配置
- 确保中断优先级高于 configMAX_SYSCALL_INTERRUPT_PRIORITY

### 10. 调试技巧

1. **使用 FreeRTOS 任务追踪**
   - 启用 configUSE_TRACE_FACILITY
   - 使用 vTaskList() 查看任务状态

2. **使用堆栈使用情况查询**
   - 使用 uxTaskGetStackHighWaterMark() 查看任务堆栈使用情况

3. **使用系统时间统计**
   - 启用 configGENERATE_RUN_TIME_STATS
   - 使用 vTaskGetRunTimeStats() 查看任务运行时间

4. **使用断言**
   - 启用 configASSERT
   - 在关键位置添加断言检查

---

## 总结

FreeRTOS 移植到 GD32F103C8T6 需要注意：
1. 正确配置 FreeRTOSConfig.h
2. 修改中断处理函数
3. 合理分配任务优先级和堆栈大小
4. 使用队列和互斥量进行任务间通信和资源保护
5. 注意内存限制，合理分配 RAM

按照以上步骤和建议，你应该能够成功将 FreeRTOS 移植到你的 GD32F103C8T6 项目中。
