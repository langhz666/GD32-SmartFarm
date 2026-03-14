#include "driver_encoder/driver_encoder.h"
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"

int16_t Encoder_Count;

void Encoder_Init(void)
{
    // 1. 开启时钟：新增开启 GPIOA 的时钟（为了 PA7）
    rcu_periph_clock_enable(RCU_GPIOA); 
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);

    // 2. 初始化旋转引脚：PB0 和 PB1 (去掉了原来的 PB2)
    gpio_init(GPIOB, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_0 | GPIO_PIN_1);
    
    // 3. 初始化按键引脚：PA7 配置为上拉输入
    gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_7);

    // 4. 配置外部中断 (负责处理 PB0 和 PB1 的旋转，保持不变)
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_0);
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_1);

    exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_init(EXTI_1, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    
    exti_interrupt_flag_clear(EXTI_0);
    exti_interrupt_flag_clear(EXTI_1);

    nvic_irq_enable(EXTI0_IRQn, 1, 1);
    nvic_irq_enable(EXTI1_IRQn, 1, 2);
}

int16_t Encoder_Get(void)
{
    int16_t Temp;
    Temp = Encoder_Count;
    Encoder_Count = 0;
    return Temp;
}

uint8_t Encoder_Key_Scan(uint8_t mode)
{
    static uint8_t key_up = 1;
    static TickType_t last_press_time = 0;
    
    if (mode) key_up = 1;
    
    if (key_up && gpio_input_bit_get(ENCODER_KEY_PORT, ENCODER_KEY_PIN) == RESET)
    {
        if (xTaskGetTickCount() - last_press_time < pdMS_TO_TICKS(200))
        {
            return ENCODER_KEY_NONE;
        }
        
        vTaskDelay(pdMS_TO_TICKS(15));
        
        if (gpio_input_bit_get(ENCODER_KEY_PORT, ENCODER_KEY_PIN) == RESET)
        {
            key_up = 0;
            last_press_time = xTaskGetTickCount();
            return ENCODER_KEY_PRES;
        }
    }
    else if (gpio_input_bit_get(ENCODER_KEY_PORT, ENCODER_KEY_PIN) == SET)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (gpio_input_bit_get(ENCODER_KEY_PORT, ENCODER_KEY_PIN) == SET)
        {
            key_up = 1;
        }
    }
    
    return ENCODER_KEY_NONE;
}

void EXTI0_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_0) == SET)
    {
        if (gpio_input_bit_get(GPIOB, GPIO_PIN_0) == RESET)
        {
            if (gpio_input_bit_get(GPIOB, GPIO_PIN_1) == RESET)
            {
                Encoder_Count --;
            }
        }
        exti_interrupt_flag_clear(EXTI_0);
    }
}

void EXTI1_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_1) == SET)
    {
        if (gpio_input_bit_get(GPIOB, GPIO_PIN_1) == RESET)
        {
            if (gpio_input_bit_get(GPIOB, GPIO_PIN_0) == RESET)
            {
                Encoder_Count ++;
            }
        }
        exti_interrupt_flag_clear(EXTI_1);
    }
}
