#include "driver_bluetooth/driver_bluetooth.h"
#include "gd32f10x_libopt.h"
#include "gd32f10x.h"
#include "driver_usart/driver_usart.h"
#include <stdio.h>
#include "delay.h"
#include <string.h>

extern uint8_t usart0_rx_buffer[];
extern volatile uint16_t usart0_rx_count;
extern volatile uint8_t usart0_rx_flag;

#define BUF_SIZE 128

/* 定义一个接收缓冲区和计数器 */

uint8_t blt_rx_buffer[BLT_RX_BUF_SIZE];
volatile uint16_t blt_rx_count = 0;
volatile uint8_t blt_rx_complete_flag = 0;

/* 如果取消下面这行的注释，系统将使用重映射的 USART1 引脚 (PD5/PD6 作为 USART1 的发送/接收引脚) 
   而不是默认引脚 (PA2/PA3) */
//#define USART1_REMAP

/**
 * @description: 初始化并配置 USART1
 * @return {*}
 */
void blt_config(void)
{
    /* 1. 使能 USART1 外设的时钟，这是使用串口的前提 */
    rcu_periph_clock_enable(RCU_USART1);
    
    #if defined USART1_REMAP
        /* --- 使用引脚重映射 (PD5/PD6) 的配置分支 --- */

        rcu_periph_clock_enable(RCU_GPIOD);/* 使能 GPIOD 的时钟 */
        rcu_periph_clock_enable(RCU_AF);/* 使能复用功能 (AF) 时钟，因为我们要改变引脚的默认映射 */
        gpio_pin_remap_config(GPIO_USART1_REMAP, ENABLE);/* 开启 USART1 的引脚重映射功能 */
        gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);/* 配置 PD5 为 USART1_Tx (发送引脚) 模式：复用推挽输出 (AF_PP)，速度：50MHz */
        gpio_init(GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_6);/* 配置 PD6 为 USART1_Rx (接收引脚) 模式：浮空输入 (IN_FLOATING)，速度：50MHz */
    #else
        /* --- 使用默认引脚 (PA2/PA3) 的配置分支 --- */
        rcu_periph_clock_enable(RCU_GPIOA);/* 使能 GPIOA 的时钟 */
        gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);/* 配置 PA2 为 USART1_Tx (发送引脚) 模式：复用推挽输出 (AF_PP)，速度：50MHz */
        gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_3);/* 配置 PA3 为 USART1_Rx (接收引脚) 模式：浮空输入 (IN_FLOATING)，速度：50MHz */
    #endif

    /* 2. USART1 工作参数配置 */
    usart_deinit(USART1);/* 复位 USART1，清除之前的遗留配置 */
    usart_baudrate_set(USART1, 9600U);/* 设置波特率为 9600 Baud (常用于蓝牙调试) */
    usart_word_length_set(USART1, USART_WL_8BIT);/* 设置数据字长为 8 位 */
    usart_stop_bit_set(USART1, USART_STB_1BIT);/* 设置停止位为 1 位 */
    usart_parity_config(USART1, USART_PM_NONE);/* 不使用奇偶校验 */
    usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);/* 禁用 RTS (请求发送) 硬件流控 */
    usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);/* 禁用 CTS (清除发送) 硬件流控 */
    
    /* 3. 使能收发功能与串口外设 */
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);/* 使能 USART1 接收功能 */
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);/* 使能 USART1 发送功能 */


    /* 4. 使能接收中断，以便在收到数据时能够及时处理 */
    nvic_irq_enable(USART1_IRQn, 1, 0); /* 配置 USART1 的中断优先级 (抢占优先级 1，子优先级 0，可根据你的系统调整) */
    usart_interrupt_enable(USART1, USART_INT_RBNE); /* 使能 USART1 接收缓冲区非空中断 (RBNE) */
    
    usart_enable(USART1);/* 最后，使能 USART1 外设 */
}

/**
 * @description: 通过蓝牙(USART1)发送一个字节
 * @param {uint8_t} byte 要发送的字节数据
 */
void blt_send_byte(uint8_t byte)
{
    
    usart_data_transmit(USART1, byte);/* 将数据写入发送数据寄存器 */
    while (RESET == usart_flag_get(USART1, USART_FLAG_TBE));/* 等待发送数据缓冲区为空 (TBE: Transmit Buffer Empty) */
}

/**
 * @description: 通过蓝牙(USART1)发送字符串
 * @param {char} *str 要发送的字符串指针
 */
void blt_send_string(char *str)
{
    /* 遍历字符串直到遇到结束符 '\0' */
    while (*str != '\0')
    {
        blt_send_byte((uint8_t)(*str));
        str++;
    }
    /* 等待发送完成 (TC: Transmission Complete)，确保最后一个字节也发完 */
    while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
}



void USART1_IRQHandler(void)
{
    /* 1. 处理溢出错误 (ORE)，防止无限中断卡死 */
    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_ERR_ORERR)) 
    {
        /* 清除 ORE 标志：GD32/STM32 的标准做法是先读状态寄存器，再读数据寄存器 */
        usart_data_receive(USART1); 
    }

    /* 2. 处理正常的接收中断 */
    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE))
    {
        uint8_t res = (uint8_t)usart_data_receive(USART1);
        
        if (blt_rx_count < BLT_RX_BUF_SIZE) 
        {
            blt_rx_buffer[blt_rx_count] = res;
            blt_rx_count++;
            
            /* 接收到换行符，标记接收完成 */
            if (res == '\n') 
            {
                blt_rx_complete_flag = 1; 
                /* 加上字符串结束符，方便后续用 printf 或 strcmp 处理 */
                if (blt_rx_count < BLT_RX_BUF_SIZE) {
                    blt_rx_buffer[blt_rx_count] = '\0'; 
                }
            }
        }
        else
        {
            /* 缓冲区满了但没收到 \n，属于异常超长数据，强制复位计数器 */
            blt_rx_count = 0; 
        }
    }
}

void blt_forward_from_pc(void)
{
    if (usart0_rx_flag == 1)
    {
#if BLT_DEBUG_ENABLE
        uart_send_string(USART0, "[Debug] GD32 Received: ");
        uart_send_string(USART0, (char*)usart0_rx_buffer);
        uart_send_string(USART0, "[Debug] Length: ");
        printf("%d\r\n", usart0_rx_count);
        
        uart_send_string(USART0, "[Debug] Sending to USART1...\r\n");
#endif
        uart_send_string(USART1, (char*)usart0_rx_buffer);
#if BLT_DEBUG_ENABLE
        uart_send_string(USART0, "[Debug] Sent to USART1\r\n");
#endif
        
        DelayNms(500);
        
        memset(usart0_rx_buffer, 0, BUF_SIZE);
        usart0_rx_count = 0;
        usart0_rx_flag = 0;
    }
}

void blt_forward_to_pc(void)
{
    if (blt_rx_complete_flag == 1)
    {
#if BLT_DEBUG_ENABLE
        uart_send_string(USART0, "[Debug] Received from USART1: ");
#endif
        uart_send_string(USART0, (char*)blt_rx_buffer);
        
        blt_led_control((char*)blt_rx_buffer);
        
#if BLT_DEBUG_ENABLE
        uart_send_string(USART0, "[Debug] End of USART1 data\r\n");
#endif
        
    }
}

/* 根据你的实际硬件修改这里的宏定义 */
#define LED_PORT GPIOC
#define LED_PIN  GPIO_PIN_13

/**
 * @description: 解析蓝牙接收到的字符串并控制 LED
 * @param {char*} cmd 接收到的字符串缓冲区的指针
 */
void blt_led_control(char *cmd)
{
    /* 使用 strstr 查找子串，可以忽略末尾的 \r 或 \n 干扰。
       支持发送 "1"、"ON" 或 "on" 开灯。
    */
    if (strstr(cmd, "1") != NULL || strstr(cmd, "ON") != NULL || strstr(cmd, "on") != NULL) 
    {
        /* 控制引脚电平。
           注意：很多开发板的 LED 是拉低点亮 (低电平有效)。
           如果是低电平点亮，请使用 gpio_bit_reset(LED_PORT, LED_PIN); 
           这里演示高电平点亮：
        */
        gpio_bit_set(LED_PORT, LED_PIN); 
        
#if BLT_DEBUG_ENABLE
        uart_send_string(USART0, "[Action] Command match: Turn ON LED\r\n");
#endif
        /* 给手机 APP 发送执行反馈 */
        blt_send_string("Action: LED ON\r\n"); 
    }
    /* 支持发送 "0"、"OFF" 或 "off" 关灯 */
    else if (strstr(cmd, "0") != NULL || strstr(cmd, "OFF") != NULL || strstr(cmd, "off") != NULL) 
    {
        /* 熄灭 LED */
        gpio_bit_reset(LED_PORT, LED_PIN);
        
#if BLT_DEBUG_ENABLE
        uart_send_string(USART0, "[Action] Command match: Turn OFF LED\r\n");
#endif
        blt_send_string("Action: LED OFF\r\n");
    }
    else 
    {
        /* 收到无法解析的指令 */
#if BLT_DEBUG_ENABLE
        uart_send_string(USART0, "[Action] Unknown command\r\n");
#endif
        blt_send_string("Error: Unknown command\r\n");
    }
}
