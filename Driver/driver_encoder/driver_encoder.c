#include "driver_encoder/driver_encoder.h"

int16_t Encoder_Count;                  // 全局变量，用于计数旋转编码器的增量值

/**
  * 函   数：旋转编码器初始化
  * 参   数：无
  * 返 回 值：无
  */
void Encoder_Init(void)
{
    /* 1. 开启时钟 */
    rcu_periph_clock_enable(RCU_GPIOB);        // 开启GPIOB的时钟
    rcu_periph_clock_enable(RCU_AF);           // 开启AFIO（复用功能）的时钟，外部中断必须开启

    /* 2. GPIO初始化 */
    // GD32不需要结构体，直接调用函数。将PB0和PB1初始化为上拉输入，速度50MHz
    gpio_init(GPIOB, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_0 | GPIO_PIN_1);

    /* 3. AFIO选择中断引脚 */
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_0); // 选择PB0为外部中断引脚
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_1); // 选择PB1为外部中断引脚

    /* 4. EXTI初始化 */
    exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_FALLING); // 配置EXTI0线为中断模式，下降沿触发
    exti_init(EXTI_1, EXTI_INTERRUPT, EXTI_TRIG_FALLING); // 配置EXTI1线为中断模式，下降沿触发
    
    // 初始化后先清除一下中断标志位，防止一上电就误触发进中断
    exti_interrupt_flag_clear(EXTI_0);
    exti_interrupt_flag_clear(EXTI_1);

    /* 5. NVIC中断分组 */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);     // 配置NVIC为分组2 (抢占优先级0~3，响应优先级0~3)

    /* 6. NVIC配置 */
    // 使能EXTI0中断，抢占优先级为1，响应优先级为1
    nvic_irq_enable(EXTI0_IRQn, 1, 1);
    // 使能EXTI1中断，抢占优先级为1，响应优先级为2
    nvic_irq_enable(EXTI1_IRQn, 1, 2);
}

/**
  * 函   数：旋转编码器获取增量值
  * 参   数：无
  * 返 回 值：自上此调用此函数后，旋转编码器的增量值
  */
int16_t Encoder_Get(void)
{
    int16_t Temp;
    Temp = Encoder_Count;
    Encoder_Count = 0;
    return Temp;
}

/**
  * 函   数：EXTI0外部中断函数
  * 注意事项：中断函数名在GD32启动文件中与STM32一致，保持不变即可
  */
void EXTI0_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_0) == SET)        // 判断是否是外部中断0号线触发的中断
    {
        /* 如果出现数据乱跳的现象，可再次判断引脚电平，以避免抖动 */
        if (gpio_input_bit_get(GPIOB, GPIO_PIN_0) == RESET)
        {
            if (gpio_input_bit_get(GPIOB, GPIO_PIN_1) == RESET) // PB0下降沿，检测PB1电平判断方向
            {
                Encoder_Count --;                      // 此方向定义为反转，计数变量自减
            }
        }
        exti_interrupt_flag_clear(EXTI_0);             // 清除外部中断0号线的中断标志位
    }
}

/**
  * 函   数：EXTI1外部中断函数
  */
void EXTI1_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_1) == SET)        // 判断是否是外部中断1号线触发的中断
    {
        /* 如果出现数据乱跳的现象，可再次判断引脚电平，以避免抖动 */
        if (gpio_input_bit_get(GPIOB, GPIO_PIN_1) == RESET)
        {
            if (gpio_input_bit_get(GPIOB, GPIO_PIN_0) == RESET) // PB1下降沿，检测PB0电平判断方向
            {
                Encoder_Count ++;                      // 此方向定义为正转，计数变量自增
            }
        }
        exti_interrupt_flag_clear(EXTI_1);             // 清除外部中断1号线的中断标志位
    }
}
