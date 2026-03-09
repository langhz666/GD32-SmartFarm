#include <stdint.h>
#include "gd32f10x.h"
#include "delay.h"

/**
***********************************************************
* @brief DWT初始化配置
* @param 无
* @return 无
***********************************************************
*/
void DelayInit(void) 
{
    /* 关闭 TRC（Trace 功能） */
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
    /* 打开 TRC，使能 DWT 模块 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* 关闭 DWT 计数功能 */
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
    /* 打开 DWT 计数功能 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* 计数寄存器清零，确保延时从0开始 */
    DWT->CYCCNT = 0;
}

/**
***********************************************************
* @brief 微秒级延时函数
* @param nUs：需要延时的微秒数
*        最大延时时间 = (2^32 / AHB总线主频) * 10^6 us
* @return 无
***********************************************************
*/
void DelayNus(uint32_t nUs)
{
    uint32_t tickStart = DWT->CYCCNT;
    uint32_t ahbFreq = 0;

    /* GD32F103 获取AHB总线时钟频率（区别于F30x） */
    ahbFreq = rcu_clock_freq_get(CK_AHB);

    /* 转换为nUs对应的时钟周期数 */
    uint32_t targetTicks = nUs * (ahbFreq / 1000000);

    /* 防止计数器溢出，处理跨溢出的延时逻辑 */
    if ((DWT->CYCCNT + targetTicks) < tickStart)
    {
        /* 先等待计数器溢出到0 */
        while (DWT->CYCCNT > tickStart);
        /* 再等待剩余的计数 */
        while (DWT->CYCCNT < (targetTicks - (0xFFFFFFFF - tickStart + 1)));
    }
    else
    {
        /* 常规延时：等待计数达到目标值 */
        while ((DWT->CYCCNT - tickStart) < targetTicks);
    }
}

/**
***********************************************************
* @brief 毫秒级延时函数
* @param nMs：需要延时的毫秒数
* @return 无
***********************************************************
*/
void DelayNms(uint32_t nMs)
{
    for (uint32_t i = 0; i < nMs; i++)
    {
        DelayNus(1000); // 每次延时1000微秒（1毫秒）
    }
}
