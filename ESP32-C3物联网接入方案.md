# GD32F103C8T6 + ESP32-C3 物联网接入方案

## ? 目录
- [整体架构方案](#整体架构方案)
- [硬件连接方案](#硬件连接方案)
- [ESP32-C3固件架构](#esp32-c3固件架构)
- [通信协议设计](#通信协议设计)
- [UniApp/微信小程序方案](#uniapp微信小程序方案)
- [实施步骤](#实施步骤)
- [关键技术要点](#关键技术要点)
- [安全性考虑](#安全性考虑)
- [成本估算](#成本估算)

---

## 整体架构方案

### 系统架构图

```
┌─────────────────┐         ┌─────────────────┐         ┌─────────────────┐
│   GD32F103C8T6  │ UART    │   ESP32-C3      │ WiFi    │    OneNet云平台  │
│   (主控制器)     │?──────?│   (WiFi模块)    │?──────?│   (MQTT Broker)  │
└─────────────────┘         └─────────────────┘         └─────────────────┘
        │                                                       │
        │                                                       │
        ▼                                                       ▼
┌─────────────────┐         ┌─────────────────┐         ┌─────────────────┐
│  传感器/执行器  │         │   微信小程序     │         │   UniApp应用     │
│  - AHT20温湿度  │         │   (用户界面)    │         │   (用户界面)    │
│  - BMP280气压   │         └─────────────────┘         └─────────────────┘
│  - 光照传感器   │
│  - 土壤湿度     │
│  - 降雨量传感器 │
│  - LED/蜂鸣器   │
│  - 水泵控制     │
└─────────────────┘
```

### 技术栈概览
- **主控制器**: GD32F103C8T6 + FreeRTOS
- **WiFi模块**: ESP32-C3
- **通信协议**: UART + MQTT
- **云平台**: OneNet
- **移动应用**: UniApp / 微信小程序

---

## 硬件连接方案

### GD32F103C8T6 ? ESP32-C3 连接方案

#### UART通信连接（推荐方案）
```
GD32F103C8T6              ESP32-C3
────────────────────────────────────
PA9 (USART0_TX)  ──────?  GPIO20 (UART1_RX)
PA10 (USART0_RX)  ?─────  GPIO21 (UART1_TX)
GND               ──────  GND
3.3V              ──────  3.3V (或5V转3.3V)
```

#### 可选控制引脚
```
GD32F103C8T6              ESP32-C3
────────────────────────────────────
PA0 (GPIO)        ──────?  GPIO19 (ESP32复位控制)
PA1 (GPIO)        ──────?  GPIO18 (ESP32唤醒/状态检测)
```

#### 电源方案
- GD32的3.3V输出为ESP32-C3供电（ESP32-C3功耗约200-300mA）
- 建议增加100μF电容滤波
- 如果GD32供电能力不足，可使用独立LDO为ESP32-C3供电

#### 硬件清单

| 组件 | 型号/规格 | 数量 | 备注 |
|------|----------|------|------|
| ESP32-C3开发板 | ESP32-C3-DevKitM-1 | 1 | WiFi模块 |
| 杜邦线 | 公对母/母对母 | 若干 | 连接线材 |
| 电容 | 100μF/16V | 1 | 电源滤波 |
| LDO稳压器 | AMS1117-3.3（可选） | 1 | 独立供电 |

---

## ESP32-C3固件架构

### 技术栈选择
- **开发框架**: ESP-IDF v5.x 或 Arduino ESP32
- **MQTT库**: ESP-MQTT (ESP-IDF) 或 PubSubClient (Arduino)
- **OneNet SDK**: OneNet MQTT SDK
- **JSON解析**: cJSON 或 ArduinoJson
- **WiFi管理**: ESP WiFi库

### 固件架构设计
```
ESP32-C3固件架构
├── main/
│   ├── app_main.c              # 主程序入口
│   ├── wifi_manager.c/h        # WiFi连接管理
│   ├── mqtt_client.c/h         # MQTT客户端管理
│   ├── onenet_service.c/h      # OneNet服务封装
│   ├── uart_comm.c/h           # 与GD32的UART通信
│   ├── protocol_parser.c/h     # 通信协议解析
│   └── data_manager.c/h        # 数据管理和缓存
├── components/
│   ├── protocol/               # 通信协议定义
│   └── config/                 # 配置文件管理
└── CMakeLists.txt
```

### 核心功能模块

#### 1. WiFi管理模块
```c
// wifi_manager.h
typedef struct {
    char ssid[32];
    char password[64];
    bool auto_reconnect;
    uint32_t reconnect_interval;
} wifi_config_t;

// 功能函数
void wifi_init(wifi_config_t *config);
void wifi_connect(void);
void wifi_disconnect(void);
bool wifi_is_connected(void);
void wifi_set_status_callback(void (*callback)(bool connected));
```

#### 2. MQTT客户端模块
```c
// mqtt_client.h
typedef struct {
    char broker_url[128];
    uint16_t port;
    char client_id[64];
    char username[64];
    char password[64];
    uint16_t keep_alive;
} mqtt_config_t;

// 功能函数
void mqtt_init(mqtt_config_t *config);
void mqtt_connect(void);
void mqtt_disconnect(void);
void mqtt_publish(const char *topic, const char *payload);
void mqtt_subscribe(const char *topic);
void mqtt_set_message_callback(void (*callback)(const char *topic, const char *payload));
```

#### 3. OneNet服务模块
```c
// onenet_service.h
typedef struct {
    char device_id[64];
    char api_key[128];
    char product_id[64];
} onenet_config_t;

// 功能函数
void onenet_init(onenet_config_t *config);
void onenet_connect(void);
void onenet_upload_sensor_data(float temperature, float humidity,
                                float pressure, uint16_t light,
                                uint8_t soil_moisture, uint8_t rain);
void onenet_upload_thresholds(FarmSafeRange_t *thresholds);
void onenet_send_alert(const char *alert_type, const char *message);
void onenet_set_command_callback(void (*callback)(const char *command, cJSON *params));
```

### OneNet MQTT连接配置
```json
{
  "instanceId": "your_instance_id",
  "productId": "your_product_id",
  "deviceId": "your_device_id",
  "apiKey": "your_api_key",
  "mqttUrl": "mqtt://183.230.40.39",
  "mqttPort": 6002
}
```

---

## 通信协议设计

### GD32 ? ESP32-C3 通信协议设计

#### 协议帧格式
```
┌──────┬──────┬──────┬──────┬──────┬──────┬──────┐
│ HEAD │ LEN  │ TYPE │ SEQ  │ DATA │ CRC  │ TAIL │
│ 0xAA │ 1B   │ 1B   │ 1B   │ N B  │ 2B   │ 0x55 │
└──────┴──────┴──────┴──────┴──────┴──────┴──────┘
```

**帧格式说明**
- `HEAD`: 帧头，固定为0xAA
- `LEN`: 数据长度（TYPE+SEQ+DATA+CRC），不包括HEAD和TAIL
- `TYPE`: 消息类型
- `SEQ`: 序列号（0-255，循环使用）
- `DATA`: 数据载荷
- `CRC`: CRC16校验（从TYPE到DATA）
- `TAIL`: 帧尾，固定为0x55

#### 消息类型定义
```c
// protocol.h - 消息类型定义
typedef enum {
    // GD32 -> ESP32-C3 消息类型
    MSG_SENSOR_DATA = 0x01,        // 传感器数据上传
    MSG_THRESHOLD_CONFIG = 0x02,    // 阈值配置同步
    MSG_STATUS_REPORT = 0x03,      // 状态报告
    MSG_ALERT_NOTIFICATION = 0x04,  // 告警通知

    // ESP32-C3 -> GD32 消息类型
    MSG_WIFI_STATUS = 0x81,        // WiFi状态通知
    MSG_CLOUD_STATUS = 0x82,       // 云平台连接状态
    MSG_REMOTE_COMMAND = 0x83,     // 远程控制命令
    MSG_TIME_SYNC = 0x84,          // 时间同步
    MSG_THRESHOLD_UPDATE = 0x85,   // 阈值更新命令

    // 通用消息类型
    MSG_ACK = 0xFF,                // 确认消息
    MSG_NACK = 0xFE                // 否认消息
} MessageType_t;
```

#### 数据结构定义
```c
// protocol.h - 数据结构定义

// 传感器数据结构（对应GD32的SensorData_t）
#pragma pack(push, 1)
typedef struct {
    float temperature;      // 温度 (°C)
    float humidity;         // 湿度 (%)
    float pressure;         // 气压 (hPa)
    uint16_t light_intensity;  // 光照强度
    uint8_t soil_moisture;  // 土壤湿度 (%)
    uint8_t rain_gauge;     // 降雨量 (%)
    uint8_t pump_state;     // 水泵状态 (0:关, 1:开)
    uint32_t timestamp;     // 时间戳
} SensorDataMsg_t;

// 阈值配置结构（对应GD32的FarmSafeRange_t）
typedef struct {
    float min_temperature;
    float max_temperature;
    float min_humidity;
    float max_humidity;
    uint16_t min_light_intensity;
    uint16_t max_light_intensity;
    uint8_t min_soil_moisture;
    uint8_t max_soil_moisture;
    uint8_t max_rain_gauge;
} ThresholdConfigMsg_t;

// 状态报告结构
typedef struct {
    uint8_t device_status;    // 设备状态 (0:正常, 1:异常)
    uint8_t pump_state;       // 水泵状态
    uint8_t led_state;        // LED状态
    uint8_t buzzer_state;     // 蜂鸣器状态
    uint16_t encoder_value;    // 编码器值
    uint8_t current_page;     // 当前显示页面
} StatusReportMsg_t;

// 告警通知结构
typedef struct {
    uint8_t alert_type;       // 告警类型
    uint8_t severity;         // 严重程度 (0:低, 1:中, 2:高)
    char message[64];         // 告警消息
    uint32_t timestamp;       // 时间戳
} AlertNotificationMsg_t;

// WiFi状态结构
typedef struct {
    uint8_t connected;        // 连接状态 (0:断开, 1:连接)
    char ssid[32];           // SSID
    int8_t rssi;             // 信号强度 (dBm)
    uint32_t ip_address;     // IP地址
} WifiStatusMsg_t;

// 云平台状态结构
typedef struct {
    uint8_t connected;        // 连接状态 (0:断开, 1:连接)
    uint8_t platform_type;    // 平台类型 (1:OneNet)
    char device_id[64];      // 设备ID
} CloudStatusMsg_t;

// 远程控制命令结构
typedef struct {
    uint8_t command_type;     // 命令类型
    uint8_t target_device;    // 目标设备
    uint8_t action;           // 动作 (0:关闭, 1:开启)
    uint8_t params[16];      // 命令参数
} RemoteCommandMsg_t;

// 时间同步结构
typedef struct {
    uint32_t timestamp;       // Unix时间戳
    int8_t timezone;          // 时区
} TimeSyncMsg_t;
#pragma pack(pop)
```

#### GD32端通信模块设计

**在GD32项目中添加ESP32通信驱动**
```
GD32F103C8T6/Driver/driver_esp32/
├── driver_esp32.c
└── driver_esp32.h
```

**driver_esp32.h**
```c
#ifndef DRIVER_ESP32_H
#define DRIVER_ESP32_H

#include "gd32f10x.h"
#include <stdint.h>
#include <stdbool.h>

// 协议定义
#define FRAME_HEAD          0xAA
#define FRAME_TAIL          0x55
#define MAX_DATA_LEN        128
#define FRAME_BUFFER_SIZE   256

// 消息类型枚举
typedef enum {
    MSG_SENSOR_DATA = 0x01,
    MSG_THRESHOLD_CONFIG = 0x02,
    MSG_STATUS_REPORT = 0x03,
    MSG_ALERT_NOTIFICATION = 0x04,
    MSG_WIFI_STATUS = 0x81,
    MSG_CLOUD_STATUS = 0x82,
    MSG_REMOTE_COMMAND = 0x83,
    MSG_TIME_SYNC = 0x84,
    MSG_THRESHOLD_UPDATE = 0x85,
    MSG_ACK = 0xFF,
    MSG_NACK = 0xFE
} MessageType_t;

// 回调函数类型定义
typedef void (*WifiStatusCallback_t)(bool connected, int8_t rssi);
typedef void (*CloudStatusCallback_t)(bool connected);
typedef void (*RemoteCommandCallback_t)(uint8_t command_type, uint8_t target_device, uint8_t action, uint8_t *params);
typedef void (*TimeSyncCallback_t)(uint32_t timestamp);
typedef void (*ThresholdUpdateCallback_t)(void *thresholds);

// 初始化函数
void esp32_comm_init(void);

// 发送函数
bool esp32_send_sensor_data(float temperature, float humidity, float pressure,
                              uint16_t light, uint8_t soil, uint8_t rain, uint8_t pump);
bool esp32_send_threshold_config(void *thresholds);
bool esp32_send_status_report(uint8_t status, uint8_t pump, uint8_t led, uint8_t buzzer,
                                uint16_t encoder, uint8_t page);
bool esp32_send_alert(uint8_t alert_type, uint8_t severity, const char *message);

// 接收处理函数
void esp32_process_rx_data(void);

// 回调函数设置
void esp32_set_wifi_status_callback(WifiStatusCallback_t callback);
void esp32_set_cloud_status_callback(CloudStatusCallback_t callback);
void esp32_set_remote_command_callback(RemoteCommandCallback_t callback);
void esp32_set_time_sync_callback(TimeSyncCallback_t callback);
void esp32_set_threshold_update_callback(ThresholdUpdateCallback_t callback);

// 状态查询
bool esp32_is_wifi_connected(void);
bool esp32_is_cloud_connected(void);

#endif // DRIVER_ESP32_H
```

#### ESP32-C3端通信模块设计

**uart_comm.c - ESP32-C3端通信实现**
```c
#include "uart_comm.h"
#include "protocol_parser.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define UART_NUM UART_NUM_1
#define BUF_SIZE (1024)
#define RX_TASK_STACK_SIZE (4096)
#define TX_TASK_STACK_SIZE (4096)

static QueueHandle_t uart_event_queue;
static TaskHandle_t rx_task_handle = NULL;
static TaskHandle_t tx_task_handle = NULL;

// 回调函数
static WifiStatusCallback_t wifi_status_cb = NULL;
static CloudStatusCallback_t cloud_status_cb = NULL;
static RemoteCommandCallback_t remote_command_cb = NULL;

void uart_comm_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_event_queue, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, GPIO_NUM_21, GPIO_NUM_20, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(uart_rx_task, "uart_rx_task", RX_TASK_STACK_SIZE, NULL, 12, &rx_task_handle);
    xTaskCreate(uart_tx_task, "uart_tx_task", TX_TASK_STACK_SIZE, NULL, 11, &tx_task_handle);
}

void uart_rx_task(void *arg)
{
    uart_event_t event;
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE + 1);

    while (1) {
        if (xQueueReceive(uart_event_queue, &event, portMAX_DELAY)) {
            bzero(data, BUF_SIZE + 1);

            if (event.type == UART_DATA) {
                int len = uart_read_bytes(UART_NUM, data, event.size, portMAX_DELAY);
                if (len > 0) {
                    protocol_parse_frame(data, len);
                }
            }
        }
    }

    free(data);
    vTaskDelete(NULL);
}

bool uart_send_frame(uint8_t *frame, uint16_t len)
{
    return uart_write_bytes(UART_NUM, (const char *)frame, len) == len;
}
```

---

## UniApp/微信小程序方案

### 技术栈选择

#### UniApp方案（跨平台）
- **框架**: UniApp (Vue3 + TypeScript)
- **UI组件库**: uView UI 或 UniUI
- **MQTT客户端**: mqtt.js 或 uni-mqtt
- **图表库**: ECharts 或 uCharts
- **状态管理**: Pinia

#### 微信小程序方案（原生）

- **框架**: 微信小程序原生开发
- **UI组件库**: Vant Weapp 或 WeUI
- **MQTT客户端**: 微信小程序MQTT插件
- **图表库**: ECharts for 微信小程序
- **状态管理**: MobX 或 Redux

### 应用架构设计

```
UniApp/微信小程序架构
├── pages/                    # 页面
│   ├── index/               # 首页（实时数据）
│   ├── monitor/             # 监控页面
│   ├── control/             # 控制页面
│   ├── settings/            # 设置页面
│   ├── history/             # 历史数据
│   └── alert/               # 告警页面
├── components/              # 组件
│   ├── sensor-card/         # 传感器卡片
│   ├── control-panel/       # 控制面板
│   ├── threshold-slider/    # 阈值调节
│   ├── chart-view/          # 图表组件
│   └── alert-list/          # 告警列表
├── services/                # 服务层
│   ├── mqtt.service.ts      # MQTT服务
│   ├── onenet.service.ts   # OneNet服务
│   └── storage.service.ts  # 本地存储
├── stores/                  # 状态管理
│   ├── device.store.ts      # 设备状态
│   ├── sensor.store.ts      # 传感器数据
│   └── settings.store.ts    # 设置配置
└── utils/                   # 工具函数
    ├── mqtt.ts             # MQTT工具
    ├── format.ts           # 格式化工具
    └── validate.ts         # 验证工具
```

### OneNet MQTT集成

#### OneNet服务实现
```typescript
// onenet.service.ts
interface OneNetConfig {
  instanceId: string;        // 实例ID
  productId: string;         // 产品ID
  deviceId: string;         // 设备ID
  apiKey: string;           // API Key
  mqttUrl: string;          // MQTT地址
  mqttPort: number;         // MQTT端口
}

class OneNetService {
  private config: OneNetConfig;
  private mqttClient: any;
  private isConnected: boolean = false;

  constructor(config: OneNetConfig) {
    this.config = config;
  }

  // 连接OneNet MQTT
  connect(): Promise<void> {
    return new Promise((resolve, reject) => {
      const options = {
        clientId: this.config.deviceId,
        username: this.config.productId,
        password: this.config.apiKey,
        clean: true,
        reconnectPeriod: 5000
      };

      this.mqttClient = mqtt.connect(
        `${this.config.mqttUrl}:${this.config.mqttPort}/mqtt`,
        options
      );

      this.mqttClient.on('connect', () => {
        this.isConnected = true;
        this.subscribeTopics();
        resolve();
      });

      this.mqttClient.on('error', (error: Error) => {
        this.isConnected = false;
        reject(error);
      });

      this.mqttClient.on('message', (topic: string, message: Buffer) => {
        this.handleMessage(topic, message);
      });
    });
  }

  // 订阅主题
  private subscribeTopics() {
    const topics = [
      `$sys/prop/${this.config.productId}/${this.config.deviceId}/post/reply`,
      `$sys/prop/${this.config.productId}/${this.config.deviceId}/set`,
      `$sys/prop/${this.config.productId}/${this.config.deviceId}/set/reply`
    ];

    topics.forEach(topic => {
      this.mqttClient.subscribe(topic);
    });
  }

  // 发布传感器数据
  publishSensorData(data: SensorData): void {
    const payload = {
      id: Date.now().toString(),
      version: '1.0',
      params: {
        temperature: data.temperature,
        humidity: data.humidity,
        pressure: data.pressure,
        light: data.light,
        soilMoisture: data.soilMoisture,
        rainGauge: data.rainGauge,
        pumpState: data.pumpState
      }
    };

    const topic = `$sys/prop/${this.config.productId}/${this.config.deviceId}/post`;
    this.mqttClient.publish(topic, JSON.stringify(payload));
  }

  // 发布控制命令
  publishControlCommand(command: ControlCommand): void {
    const payload = {
      id: Date.now().toString(),
      version: '1.0',
      params: command
    };

    const topic = `$sys/prop/${this.config.productId}/${this.config.deviceId}/post`;
    this.mqttClient.publish(topic, JSON.stringify(payload));
  }

  // 处理接收到的消息
  private handleMessage(topic: string, message: Buffer) {
    const data = JSON.parse(message.toString());

    if (topic.includes('/set')) {
      // 处理远程控制命令
      this.handleRemoteCommand(data);
    } else if (topic.includes('/reply')) {
      // 处理响应消息
      this.handleReply(data);
    }
  }
}
```

### 页面功能设计

#### 1. 首页（实时数据展示）
```typescript
// pages/index/index.vue
<template>
  <view class="container">
    <!-- 设备状态卡片 -->
    <device-status-card :status="deviceStatus" />

    <!-- 传感器数据卡片 -->
    <sensor-card
      title="温度"
      :value="sensorData.temperature"
      unit="°C"
      :threshold="{min: settings.minTemperature, max: settings.maxTemperature}"
    />
    <sensor-card
      title="湿度"
      :value="sensorData.humidity"
      unit="%"
      :threshold="{min: settings.minHumidity, max: settings.maxHumidity}"
    />
    <sensor-card
      title="光照强度"
      :value="sensorData.light"
      unit="Lux"
      :threshold="{min: settings.minLight, max: settings.maxLight}"
    />

    <!-- 快捷控制按钮 -->
    <control-panel
      :pumpState="sensorData.pumpState"
      @pump-toggle="togglePump"
    />
  </view>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import { useDeviceStore } from '@/stores/device.store';
import { useSensorStore } from '@/stores/sensor.store';

const deviceStore = useDeviceStore();
const sensorStore = useSensorStore();

const deviceStatus = computed(() => deviceStore.status);
const sensorData = computed(() => sensorStore.data);
const settings = computed(() => deviceStore.settings);

onMounted(() => {
  // 连接MQTT
  deviceStore.connectMQTT();
  // 订阅传感器数据
  sensorStore.subscribeSensorData();
});

onUnmounted(() => {
  deviceStore.disconnectMQTT();
});

function togglePump() {
  deviceStore.sendControlCommand({
    target: 'pump',
    action: sensorData.value.pumpState ? 'off' : 'on'
  });
}
</script>
```

#### 2. 控制页面
```typescript
// pages/control/control.vue
<template>
  <view class="control-page">
    <view class="control-section">
      <view class="section-title">设备控制</view>

      <!-- 水泵控制 -->
      <control-item
        title="水泵"
        :state="deviceState.pump"
        @toggle="togglePump"
      />

      <!-- LED控制 -->
      <control-item
        title="LED指示灯"
        :state="deviceState.led"
        @toggle="toggleLED"
      />

      <!-- 蜂鸣器控制 -->
      <control-item
        title="蜂鸣器"
        :state="deviceState.buzzer"
        @toggle="toggleBuzzer"
      />
    </view>

    <view class="control-section">
      <view class="section-title">阈值设置</view>

      <!-- 温度阈值 -->
      <threshold-slider
        title="温度范围"
        :min="0"
        :max="50"
        v-model:min="settings.minTemperature"
        v-model:max="settings.maxTemperature"
        unit="°C"
        @change="updateThreshold"
      />

      <!-- 湿度阈值 -->
      <threshold-slider
        title="湿度范围"
        :min="0"
        :max="100"
        v-model:min="settings.minHumidity"
        v-model:max="settings.maxHumidity"
        unit="%"
        @change="updateThreshold"
      />
    </view>
  </view>
</template>
```

#### 3. 监控页面（图表展示）
```typescript
// pages/monitor/monitor.vue
<template>
  <view class="monitor-page">
    <!-- 时间范围选择 -->
    <time-range-selector v-model="timeRange" @change="loadHistoryData" />

    <!-- 温度趋势图 -->
    <chart-view
      title="温度趋势"
      :data="chartData.temperature"
      type="line"
      :options="chartOptions"
    />

    <!-- 湿度趋势图 -->
    <chart-view
      title="湿度趋势"
      :data="chartData.humidity"
      type="line"
      :options="chartOptions"
    />

    <!-- 光照趋势图 -->
    <chart-view
      title="光照趋势"
      :data="chartData.light"
      type="bar"
      :options="chartOptions"
    />
  </view>
</template>
```

### 数据流程设计

```
数据流程图
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│ GD32传感器   │ UART    │ ESP32-C3    │ WiFi    │ OneNet云平台 │
│ 数据采集     │────────?│ MQTT客户端  │────────?│ MQTT Broker  │
└─────────────┘         └─────────────┘         └─────────────┘
                                                        │
                                                        │ MQTT
                                                        ▼
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│ 微信小程序   │ MQTT    │ OneNet云平台 │ MQTT    │ ESP32-C3    │
│ 数据展示     │?────────│ MQTT Broker  │?────────│ MQTT客户端  │
│ 远程控制     │────────?│             │────────?│             │
└─────────────┘         └─────────────┘         └─────────────┘
                                                        │
                                                        │ UART
                                                        ▼
                                              ┌─────────────┐
                                              │ GD32主控制器 │
                                              │ 执行控制命令 │
                                              └─────────────┘
```

---

## 实施步骤

### 第一阶段：硬件准备（1-2天）

#### 1. 采购硬件
- ESP32-C3开发板（推荐：ESP32-C3-DevKitM-1）
- 连接线材（杜邦线、排针）
- 电源模块（如需要）

#### 2. 硬件连接
- 按照上述连接方案连接GD32和ESP32-C3
- 测试UART通信（使用串口助手）
- 验证电源供电稳定性

#### 3. 硬件测试清单
- [ ] ESP32-C3供电正常
- [ ] UART通信正常（波特率115200）
- [ ] 信号完整性良好
- [ ] 电源稳定性良好

### 第二阶段：ESP32-C3固件开发（5-7天）

#### 1. 开发环境搭建
- 安装ESP-IDF或Arduino IDE
- 配置开发工具链
- 创建ESP32-C3项目

#### 2. 基础功能开发

- WiFi连接管理模块
- UART通信模块
- 协议解析模块

#### 3. OneNet集成
- 注册OneNet账号，创建产品和设备
- 实现MQTT客户端
- 实现数据上传和命令接收

#### 4. 测试调试
- 单元测试各模块
- 集成测试整体功能
- 性能优化

#### 5. 开发任务清单
- [ ] WiFi连接管理模块
- [ ] MQTT客户端模块
- [ ] OneNet服务模块
- [ ] UART通信模块
- [ ] 协议解析模块
- [ ] 数据管理模块
- [ ] 单元测试
- [ ] 集成测试

### 第三阶段：GD32端集成（3-5天）

#### 1. 驱动开发
- 创建ESP32通信驱动
- 实现协议封装
- 集成到现有FreeRTOS任务

#### 2. 功能集成
- 修改Sensor_Task，添加ESP32数据上传
- 添加远程控制处理
- 实现阈值同步功能

#### 3. 测试验证
- 测试GD32与ESP32通信
- 测试数据上传功能
- 测试远程控制功能

#### 4. 集成任务清单
- [ ] 创建driver_esp32驱动
- [ ] 实现协议帧封装/解析
- [ ] 集成到Sensor_Task
- [ ] 添加远程控制处理
- [ ] 实现阈值同步
- [ ] 通信测试
- [ ] 功能测试

### 第四阶段：移动应用开发（7-10天）

#### 1. UniApp/小程序开发
- 创建项目框架
- 实现MQTT连接
- 开发各页面功能

#### 2. OneNet集成
- 配置OneNet连接参数
- 实现数据订阅和发布
- 实现远程控制功能

#### 3. UI/UX优化
- 界面美化
- 交互优化
- 错误处理

#### 4. 开发任务清单
- [ ] 创建UniApp/小程序项目
- [ ] 实现MQTT服务
- [ ] 开发首页（实时数据）
- [ ] 开发控制页面
- [ ] 开发监控页面
- [ ] 开发设置页面
- [ ] 开发历史数据页面
- [ ] 开发告警页面
- [ ] UI/UX优化
- [ ] 功能测试

### 第五阶段：系统测试（3-5天）

#### 1. 功能测试
- 端到端功能测试
- 异常场景测试
- 性能测试

#### 2. 稳定性测试
- 长时间运行测试
- 网络异常恢复测试
- 边界条件测试

#### 3. 用户验收测试
- 实际使用场景测试
- 用户体验评估
- 问题修复

#### 4. 测试任务清单
- [ ] 功能测试
- [ ] 异常场景测试
- [ ] 性能测试
- [ ] 长时间运行测试
- [ ] 网络异常恢复测试
- [ ] 边界条件测试
- [ ] 用户验收测试
- [ ] 问题修复

---

## 关键技术要点

### OneNet MQTT配置
```json
{
  "instanceId": "your_instance_id",
  "productId": "your_product_id",
  "deviceId": "your_device_id",
  "apiKey": "your_api_key",
  "mqttUrl": "mqtt://183.230.40.39",
  "mqttPort": 6002
}
```

### 数据上传频率建议
- **实时数据**: 5-10秒上传一次
- **历史数据**: 1分钟上传一次
- **告警数据**: 实时上传
- **状态数据**: 30秒上传一次

### 断网处理策略
- **数据缓存**: ESP32-C3本地缓存最近100条数据
- **自动重连**: 断网后自动重连，重连间隔5秒
- **数据补传**: 重连成功后自动补传缓存数据
- **状态指示**: 通过LED指示网络状态

### 性能优化建议
- 使用DMA传输UART数据
- 优化MQTT消息大小
- 实现数据压缩
- 合理设置心跳间隔
- 使用连接池管理

---

## 安全性考虑

### 数据安全
- 使用TLS/SSL加密MQTT通信
- API Key定期更换
- 敏感数据加密存储

### 设备安全
- 设备认证机制
- 访问权限控制
- 固件OTA升级安全

### 网络安全
- 防止DDoS攻击
- 数据包校验
- 异常连接检测

### 安全实施清单
- [ ] MQTT TLS/SSL加密
- [ ] API Key管理
- [ ] 设备认证
- [ ] 访问权限控制
- [ ] 数据包校验
- [ ] 异常检测
- [ ] 固件签名验证

---

## 成本估算

### 硬件成本
| 项目 | 型号/规格 | 数量 | 单价 | 小计 |
|------|----------|------|------|------|
| ESP32-C3开发板 | ESP32-C3-DevKitM-1 | 1 | ?15-30 | ?15-30 |
| 连接线材 | 杜邦线、排针 | 若干 | ?5-10 | ?5-10 |
| 电源模块 | AMS1117-3.3（可选） | 1 | ?10-20 | ?10-20 |
| **总计** | | | | **?30-60** |

### 软件成本
| 项目 | 费用 | 备注 |
|------|------|------|
| OneNet云平台 | 免费 | 基础功能免费 |
| 开发工具 | 免费 | ESP-IDF、Arduino IDE |
| **总计** | **?0** | |

### 运营成本
| 项目 | 费用 | 备注 |
|------|------|------|
| 网络流量 | 根据使用情况 | 家用宽带 |
| 云服务 | 免费 | OneNet免费版 |
| **总计** | **?0-50/月** | |

### 总体成本估算
- **一次性投入**: ?30-60（硬件）
- **月度运营**: ?0-50（网络）
- **年度成本**: ?30-660

---

## 总结

本方案提供了一个完整的GD32F103C8T6 + ESP32-C3物联网接入解决方案，具有以下优势：

### 技术优势
1. **技术成熟**: 使用成熟的技术栈（ESP32-C3、MQTT、OneNet）
2. **架构清晰**: 模块化设计，易于理解和维护
3. **扩展性强**: 支持多种传感器和执行器
4. **性能优异**: 低功耗、高稳定性

### 成本优势
1. **硬件成本低**: ESP32-C3开发板价格低廉
2. **云服务免费**: OneNet免费版满足基本需求
3. **开发成本低**: 开源工具和框架

### 实施优势
1. **分阶段实施**: 逐步推进，降低风险
2. **测试充分**: 每个阶段都有完整的测试计划
3. **文档完善**: 详细的开发文档和实施指南

### 用户体验优势
1. **多平台支持**: UniApp支持多平台部署
2. **界面友好**: 直观的用户界面设计
3. **功能完整**: 实时监控、远程控制、历史数据等

### 建议实施顺序
1. **硬件准备**: 采购和连接硬件
2. **ESP32固件**: 开发和测试ESP32-C3固件
3. **GD32集成**: 在GD32上集成ESP32通信
4. **移动应用**: 开发UniApp/微信小程序
5. **系统测试**: 全面测试和优化

### 后续扩展方向
1. **AI功能**: 添加智能分析和预测
2. **语音控制**: 集成语音助手
3. **视频监控**: 添加摄像头支持
4. **多设备管理**: 支持多设备集中管理
5. **数据分析**: 深度数据分析和报表

---

## 附录

### A. 参考资源
- ESP32-C3官方文档: https://docs.espressif.com/projects/esp-idf/
- OneNet官方文档: https://open.iot.10086.cn/doc/
- UniApp官方文档: https://uniapp.dcloud.net.cn/
- 微信小程序文档: https://developers.weixin.qq.com/miniprogram/dev/framework/

### B. 常见问题

#### Q1: ESP32-C3供电不足怎么办？
A: 使用独立LDO（如AMS1117-3.3）为ESP32-C3供电，确保供电能力足够。

#### Q2: MQTT连接不稳定怎么办？
A: 检查网络质量，增加心跳间隔，实现自动重连机制。

#### Q3: 如何提高数据传输效率？
A: 使用数据压缩，优化消息格式，合理设置上传频率。

#### Q4: 如何保证数据安全？

A: 使用TLS/SSL加密，定期更换API Key，实现设备认证。

### C. 联系支持

如有问题，请联系技术支持团队或查阅相关文档。

---

**文档版本**: v1.0
**最后更新**: 2026-03-16
**作者**: AI Assistant
