#include "driver_adc/driver_adc.h"
#include "delay.h"
#include "driver_adc.h"


// 定义一个全局数组存放多通道 ADC 数据
// adc_buffer[0] 存通道 4 (土壤) 的数据
// adc_buffer[1] 存通道 5 (雨滴) 的数据
uint16_t adc_buffer[ADC_CHANNEL_COUNT] = {0};



void ADC_DMA_MultiChannel_Init(void)
{
    /* 1. 开启外设时钟 (注意这里多开启了 DMA0 的时钟) */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_DMA0); // 开启 DMA 时钟

    /* 2. 配置时钟分频与 GPIO */
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV8); 
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_4 | GPIO_PIN_5);

    /* 3. 配置 DMA (ADC0 通常对应 DMA0 的 Channel0) */
    dma_parameter_struct dma_data_parameter;
    dma_deinit(DMA0, DMA_CH0);
    
    dma_data_parameter.periph_addr = (uint32_t)(&ADC_RDATA(ADC0)); // 源地址：ADC0 的数据寄存器
    dma_data_parameter.periph_inc  = DMA_PERIPH_INCREASE_DISABLE;  // 外设地址不递增
    dma_data_parameter.memory_addr = (uint32_t)(adc_buffer);       // 目标地址：我们定义的数组
    dma_data_parameter.memory_inc  = DMA_MEMORY_INCREASE_ENABLE;   // 内存地址递增 (adc_buffer[0]->[1])
    dma_data_parameter.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;  // 数据宽度：16位
    dma_data_parameter.memory_width = DMA_MEMORY_WIDTH_16BIT;      
    dma_data_parameter.direction   = DMA_PERIPHERAL_TO_MEMORY;     // 传输方向：外设到内存
    dma_data_parameter.number      = ADC_CHANNEL_COUNT;            // 传输数量：2 个通道
    dma_data_parameter.priority    = DMA_PRIORITY_HIGH;            // 优先级：高
    
    dma_init(DMA0, DMA_CH0, &dma_data_parameter);
    dma_circulation_enable(DMA0, DMA_CH0); // 开启 DMA 循环模式
    dma_channel_enable(DMA0, DMA_CH0);     // 使能 DMA 通道

    /* 4. 配置 ADC */
    adc_deinit(ADC0);
    adc_mode_config(ADC_MODE_FREE); 
    
    // 【关键配置】开启扫描模式，开启连续转换模式
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE); 
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);

    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE); 
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    // 【关键配置】设置序列长度为 2，并规定转换顺序
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, ADC_CHANNEL_COUNT);
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_4, ADC_SAMPLETIME_55POINT5); // 第 1 个转换通道 4
    adc_regular_channel_config(ADC0, 1, ADC_CHANNEL_5, ADC_SAMPLETIME_55POINT5); // 第 2 个转换通道 5

    /* 5. 使能 ADC 与 DMA 请求，并校准 */
    adc_enable(ADC0);
    DelayNms(1); 
    adc_calibration_enable(ADC0);

    adc_dma_mode_enable(ADC0); // 开启 ADC 的 DMA 传输请求

    /* 6. 软件触发，启动第一次转换（之后它就会自己一直在后台无限循环转了） */
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}
