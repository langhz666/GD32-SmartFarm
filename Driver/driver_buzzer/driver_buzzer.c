#include "driver_buzzer/driver_buzzer.h"
#include "driver_buzzer.h"
 // 请根据你的具体型号修改头文件，例如 gd32f30x.h



void Buzzer_PWM_Init(void) 
{
    timer_parameter_struct timer_initpara;
    timer_oc_parameter_struct timer_ocintpara;

    // 1. 使能 GPIOA 和 TIMER2 (通用定时器) 的时钟
    rcu_periph_clock_enable(BUZZER_RCU);
    rcu_periph_clock_enable(BUZZER_TIMER);

    // 2. 配置 PA6 为复用推挽输出
    gpio_init(BUZZER_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, BUZZER_PIN);

    // 3. 配置通用定时器 TIMER2 基础参数
    timer_deinit(TIMER2);
    
    // 预分频：120M / 120 = 1MHz 的计数时钟
    timer_initpara.prescaler         = 108 - 1; 
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 1000 - 1; // 初始周期设为 1kHz
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    // timer_initpara.repetitioncounter = 0; // 通用定时器没有这个参数，不需要配
    timer_init(TIMER2, &timer_initpara);

    // 4. 配置 PWM 通道输出 (PA6 对应 TIMER_CH_0)
    timer_oc_parameter_struct_init(&timer_ocintpara);
    
    // 开启主通道输出
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE; 
    // 关闭互补通道输出 (通用定时器也没有互补通道)
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE; 
    
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    
    // 绑定到 TIMER_CH_0
    timer_channel_output_config(TIMER2, TIMER_CH_0, &timer_ocintpara); 

    // 设置初始占空比为 0 (静音)
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_0, 1000-1); //占空比为100%
    timer_channel_output_mode_config(TIMER2, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER2, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);

    // 5. 使能 TIMER2
    timer_enable(TIMER2);
}

// freq: 频率(Hz), volume: 音量(0~50%)
void Buzzer_SetSound(uint16_t freq, uint8_t volume) 
{
    if (volume == 0 || freq == 0) {
        timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_0, 1000 - 1); 
        return;
    }
    
    // 根据 1MHz 计数频率计算周期
    uint32_t period = 1000000 / freq;
    
    if(volume > 50) volume = 50; 
    uint32_t pulse = (period * volume) / 100;
    
    // 更新 TIMER2 的周期和占空比
    timer_autoreload_value_config(TIMER2, period - 1);
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_0, pulse);
}


void Buzzer_On(void)
{ 
    Buzzer_SetSound(2000, 50); // 以 2kHz 频率，50%最大音量发声
}

void Buzzer_Off(void)
{ 
    Buzzer_SetSound(0, 0); // 停止发声
}

