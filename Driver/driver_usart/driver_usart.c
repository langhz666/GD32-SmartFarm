#include "driver_usart/driver_usart.h"
#include "gd32f10x_libopt.h"
#include <stdio.h>


/* 定义接收缓冲区和计数器 */
#define BUF_SIZE 128
uint8_t usart0_rx_buffer[BUF_SIZE];
volatile uint16_t usart0_rx_count;
volatile uint8_t usart0_rx_flag;



/* 如果取消下面这行的注释，系统将使用重映射的 USART0 引脚 (PB6/PB7) 
   而不是默认引脚 (PA9/PA10) */
//#define USART0_REMAP

/**
 * @description: 初始化并配置 USART0
 * @return {*}
 */
void usart_config(void)
{
    /* 1. 使能 USART0 外设的时钟，这是使用串口的前提 */
    rcu_periph_clock_enable(RCU_USART0);
    
    #if defined USART0_REMAP
        /* --- 使用引脚重映射 (PB6/PB7) 的配置分支 --- */

        rcu_periph_clock_enable(RCU_GPIOB);/* 使能 GPIOB 的时钟 */
        rcu_periph_clock_enable(RCU_AF);/* 使能复用功能 (AF) 时钟，因为我们要改变引脚的默认映射 */
        gpio_pin_remap_config(GPIO_USART0_REMAP, ENABLE);/* 开启 USART0 的引脚重映射功能 */
        gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);/* 配置 PB6 为 USART0_Tx (发送引脚) 模式：复用推挽输出 (AF_PP)，速度：50MHz */
        gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_7);/* 配置 PB7 为 USART0_Rx (接收引脚) 模式：浮空输入 (IN_FLOATING)，速度：50MHz */
    #else
        /* --- 使用默认引脚 (PA9/PA10) 的配置分支 --- */
        rcu_periph_clock_enable(RCU_GPIOA);/* 使能 GPIOA 的时钟 */
        gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);/* 配置 PA9 为 USART0_Tx (发送引脚) 模式：复用推挽输出 (AF_PP)，速度：50MHz */
        gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);/* 配置 PA10 为 USART0_Rx (接收引脚) 模式：浮空输入 (IN_FLOATING)，速度：50MHz */
    #endif

    /* 2. USART0 工作参数配置 */
    usart_deinit(USART0);/* 复位 USART0，清除之前的遗留配置 */
    usart_baudrate_set(USART0, 115200U);/* 设置波特率为 115200 (常用于串口调试) */
    usart_word_length_set(USART0, USART_WL_8BIT);/* 设置数据字长为 8 位 */
    usart_stop_bit_set(USART0, USART_STB_1BIT);/* 设置停止位为 1 位 */
    usart_parity_config(USART0, USART_PM_NONE);/* 不使用奇偶校验 */
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);/* 禁用 RTS (请求发送) 硬件流控 */
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);/* 禁用 CTS (清除发送) 硬件流控 */
    
    /* 3. 使能收发功能与串口外设 */
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);/* 使能 USART0 接收功能 */
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);/* 使能 USART0 发送功能 */
    
    /* 4. 使能接收中断 */
    nvic_irq_enable(USART0_IRQn, 0, 0);
    usart_interrupt_enable(USART0, USART_INT_RBNE);
    
    usart_enable(USART0);/* 最后，使能 USART0 外设，让串口正式开始工作 */
}

/**
 * @description: 通用串口字符串发送函数
 * @param {uint32_t} usart_periph 使用的串口外设 (例如 USART0, USART1)
 * @param {char*} str 要发送的字符串首地址
 */
void uart_send_string(uint32_t usart_periph, char *str)
{
    /* 1. 遍历字符串，直到遇到字符串结束符 '\0' */
    while (*str != '\0') 
    {
        /* 将当前字符写入发送数据寄存器 */
        usart_data_transmit(usart_periph, (uint8_t)(*str));
        
        /* 等待发送数据缓冲区为空 (TBE: Transmit Buffer Empty)
           说明这个字节已经被搬运到了移位寄存器，可以放下一个字节了 */
        while (RESET == usart_flag_get(usart_periph, USART_FLAG_TBE));
        
        /* 指针偏移，准备发送下一个字符 */
        str++;
    }
    
    /* 2. 等待整个字符串发送完成 (TC: Transmission Complete)
       确保最后一个字符的最后一个 bit 也已经从 TX 引脚物理发送出去了 */
    while (RESET == usart_flag_get(usart_periph, USART_FLAG_TC));
}


/**
 * @description: 重定向 C 库的 printf 函数到 USART0
 * @param {int} ch
 * @param {FILE} *f
 * @return {*}
 */
int fputc(int ch, FILE *f)
{
    
    usart_data_transmit(USART0, (uint8_t)ch);/* 将字符发送到 USART0 的数据寄存器 */
    /* 轮询等待发送数据缓冲区为空 (TBE: Transmit Buffer Empty)
       如果 RESET == usart_flag_get，说明数据还没被移入移位寄存器，需要死循环等待 */
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;/* 返回发送的字符 */
}



void USART0_IRQHandler(void)
{
    if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE))
    {
        uint8_t res = (uint8_t)usart_data_receive(USART0);
        
        if (usart0_rx_count < BUF_SIZE)
        {
            usart0_rx_buffer[usart0_rx_count] = res;
            usart0_rx_count++;
            
            if (res == '\n')
            {
                usart0_rx_flag = 1;
                if (usart0_rx_count < BUF_SIZE)
                {
                    usart0_rx_buffer[usart0_rx_count] = '\0';
                }
            }
        }
        else
        {
            usart0_rx_count = 0;
        }
    }
}

