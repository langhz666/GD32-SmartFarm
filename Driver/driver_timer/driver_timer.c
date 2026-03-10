#include "driver_timer/driver_timer.h"

extern uint8_t led_blink_mode;

/**
 * @description: 初始化 TIMER2 为 500ms 的周期中断
 */
void timer2_config(void)
{
    timer_parameter_struct timer_initpara;

    /* 1. 使能 TIMER2 的外设时钟 */
    rcu_periph_clock_enable(RCU_TIMER2);

    /* 2. 配置 NVIC 中断优先级 (确保它的优先级低于你的串口接收中断，防止漏接数据) */
    nvic_irq_enable(TIMER2_IRQn, 2, 0); // 抢占优先级 2，子优先级 0

    /* 3. 复位并配置 TIMER2 参数 */
    timer_deinit(TIMER2);

    /* * 时间计算公式：中断时间 = (Prescaler + 1) * (Period + 1) / 内部时钟频率
     * 假设系统时钟频率为 108MHz:
     * 预分频器 (prescaler) 设为 10800-1，此时定时器的工作频率为 108MHz / 10800 = 10kHz (即 0.1ms 计一次数)
     * 周期 (period) 设为 5000-1，即计数 5000 次触发一次中断，5000 * 0.1ms = 500ms
     */
    timer_initpara.prescaler         = 10800 - 1; 
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 5000 - 1; 
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_init(TIMER2, &timer_initpara);

    /* 4. 使能 TIMER2 更新中断 (Update Interrupt) */
    timer_interrupt_enable(TIMER2, TIMER_INT_UP);

    /* 5. 使能 TIMER2 计数器，让它一直在后台跑 */
    timer_enable(TIMER2);
}

void TIMER2_IRQHandler(void)
{
    /* 检查是否是 TIMER2 的更新中断 (UP) 触发的 */
    if(SET == timer_interrupt_flag_get(TIMER2, TIMER_INT_FLAG_UP))
    {
        /* 1. 清除中断标志位！(极其重要，不清除的话程序会永远卡在这个中断里出不去) */
        timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_UP);

        /* 2. 判断当前是否处于闪烁模式 */
        if(led_blink_mode == 1)
        {
            led_toggle(); // 执行一次翻转
        }
    }
}
