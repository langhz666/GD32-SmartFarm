#include "driver_aht20/driver_aht20.h"
#include "delay.h"
#include "driver_aht20.h"


#define AHT20_IIC_ADDR      0x38

/* AHT20 命令宏定义 */
#define AHT20_ADDR_WRITE  ((AHT20_IIC_ADDR << 1) | 0x00) // 0x70
#define AHT20_ADDR_READ   ((AHT20_IIC_ADDR << 1) | 0x01) // 0x71

#define AHT20_CMD_INIT    0xBE  // 初始化命令
#define AHT20_CMD_MEASURE 0xAC  // 触发测量命令

/* ========================= GPIO初始化 ========================= */
/*
 * @brief  初始化软件IIC使用的GPIO（PB10=SCL, PB11=SDA）
 * @note   配置为开漏输出，符合I2C总线电气特性
 */
static void IIC_GpioInit(void)
{
    /* 使能GPIOB时钟 */
    rcu_periph_clock_enable(AHT20_IIC_RCU_PORT);

    /* PB10/PB11 配置为开漏输出 */
    gpio_init(AHT20_IIC_PORT, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, AHT20_IIC_SCL_PIN | AHT20_IIC_SDA_PIN);

    /* 默认释放总线，进入空闲态（SCL=1, SDA=1） */
    AHT20_IIC_SCL_HIGH();
    AHT20_IIC_SDA_HIGH();
}

/*
 * @brief  软件IIC初始化
 */
void AHT20_IIC_Init(void)
{
    IIC_GpioInit();
}


/* ========================= IIC基本时序 ========================= */
/*
 * @brief  产生IIC起始信号（Start）
 * @note   起始条件：SCL高电平期间，SDA由高变低
 */
void AHT20_IIC_Start(void)
{
    /* 确保总线空闲态 */
    AHT20_IIC_SDA_HIGH();
    AHT20_IIC_SCL_HIGH();
    AHT20_IIC_DELAY_US(4);

    /* 产生Start条件 */
    AHT20_IIC_SDA_LOW();
    AHT20_IIC_DELAY_US(4);

    /* 拉低SCL，进入数据传输阶段 */
    AHT20_IIC_SCL_LOW();
    AHT20_IIC_DELAY_US(4);
}

/*
 * @brief  产生IIC停止信号（Stop）
 * @note   停止条件：SCL高电平期间，SDA由低变高
 */
void AHT20_IIC_Stop(void)
{
    /* 先保持SDA为低 */
    AHT20_IIC_SDA_LOW();
    AHT20_IIC_DELAY_US(4);

    /* 拉高SCL */
    AHT20_IIC_SCL_HIGH();
    AHT20_IIC_DELAY_US(4);

    /* 产生Stop条件 */
    AHT20_IIC_SDA_HIGH();
    AHT20_IIC_DELAY_US(4);
}


/* ========================= 发送/接收1字节 ========================= */
/*
 * @brief  发送1个字节（高位在前，MSB first）
 * @param  tx_byte: 待发送数据
 */
void AHT20_IIC_SendByte(uint8_t tx_byte)
{
    for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++)
    {
        /* 1) 在SCL低电平期间准备当前位数据（先发最高位） */
        if (tx_byte & 0x80U)
        {
            AHT20_IIC_SDA_HIGH();   /* 当前位为1：释放SDA */
        }
        else
        {
            AHT20_IIC_SDA_LOW();    /* 当前位为0：拉低SDA */
        }

        tx_byte <<= 1;
        AHT20_IIC_DELAY_US(4);

        /* 2) 拉高SCL：从机在高电平期间采样SDA */
        AHT20_IIC_SCL_HIGH();
        AHT20_IIC_DELAY_US(4);

        /* 3) 拉低SCL：本位发送结束，准备下一位 */
        AHT20_IIC_SCL_LOW();
        AHT20_IIC_DELAY_US(4);
    }
}

/*
 * @brief  读取1个字节（高位在前，MSB first）
 * @return 读取到的1字节数据
 */
uint8_t AHT20_IIC_ReadByte(void)
{
    uint8_t rx_byte = 0U;

    /* 释放SDA，由从机驱动数据线 */
    AHT20_IIC_SDA_HIGH();

    for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++)
    {
        rx_byte <<= 1;

        /* 拉高SCL，从机输出当前位，主机采样 */
        AHT20_IIC_SCL_HIGH();
        AHT20_IIC_DELAY_US(4);

        if (AHT20_IIC_READ_SDA() != 0U)
        {
            rx_byte |= 0x01U;
        }

        AHT20_IIC_DELAY_US(4);

        /* 拉低SCL，准备读取下一位 */
        AHT20_IIC_SCL_LOW();
        AHT20_IIC_DELAY_US(4);
    }

    return rx_byte;
}


/* ========================= ACK/NACK相关 ========================= */
/*
 * @brief  等待从机应答（ACK）
 * @return true  - 收到ACK（SDA被从机拉低）
 * @return false - 超时未收到ACK
 * @note   第9个时钟周期由主机产生，从机在此期间拉低SDA表示ACK
 */
bool AHT20_IIC_WaitAck(void)
{
    uint8_t err_times = 0U;

    /* 主机释放SDA，让从机驱动应答位 */
    AHT20_IIC_SDA_HIGH();
    AHT20_IIC_DELAY_US(4);

    /* 拉高SCL，开始ACK采样 */
    AHT20_IIC_SCL_HIGH();
    AHT20_IIC_DELAY_US(4);

    /* 等待从机拉低SDA */
    while (AHT20_IIC_READ_SDA() != 0U)
    {
        err_times++;

        if (err_times > 250U)
        {
            /* 超时无应答，发送停止信号退出 */
            AHT20_IIC_Stop();
            return false;
        }
    }

    /* 完成ACK采样后拉低SCL */
    AHT20_IIC_SCL_LOW();
    AHT20_IIC_DELAY_US(4);
    return true;
}

/*
 * @brief  主机发送ACK（应答）
 * @note   用于主机读数据时，告知从机“继续发送下一字节”
 */
void AHT20_IIC_SendAck(void)
{
    AHT20_IIC_SDA_LOW();          /* ACK位 = 0 */
    AHT20_IIC_DELAY_US(4);

    AHT20_IIC_SCL_HIGH();         /* 第9个时钟脉冲 */
    AHT20_IIC_DELAY_US(4);

    AHT20_IIC_SCL_LOW();
    AHT20_IIC_DELAY_US(4);

    AHT20_IIC_SDA_HIGH();         /* 释放SDA */
}

/*
 * @brief  主机发送NACK（非应答）
 * @note   用于主机读完最后一个字节后，告知从机“结束读取”
 */
void AHT20_IIC_SendNack(void)
{
    AHT20_IIC_SDA_HIGH();         /* NACK位 = 1（释放SDA） */
    AHT20_IIC_DELAY_US(4);

    AHT20_IIC_SCL_HIGH();         /* 第9个时钟脉冲 */
    AHT20_IIC_DELAY_US(4);

    AHT20_IIC_SCL_LOW();
    AHT20_IIC_DELAY_US(4);
}

/*
 * @brief  AHT20 传感器初始化
 * @return true - 成功, false - 失败
 */
bool AHT20_Init(void)
{
    uint8_t status;
    
    /* 1. 底层 IIC 引脚初始化 */
    AHT20_IIC_Init();
    
    /* 2. 上电后延时 40ms，等待传感器就绪 */
    AHT20_IIC_DELAY_US(40000); 
    
    /* 3. 读取状态字，检查是否需要校准 */
    AHT20_IIC_Start();
    AHT20_IIC_SendByte(AHT20_ADDR_READ);
    if (!AHT20_IIC_WaitAck()) 
    {
        AHT20_IIC_Stop();
        return false; // 传感器无响应
    }
    
    status = AHT20_IIC_ReadByte();
    AHT20_IIC_SendNack();
    AHT20_IIC_Stop();
    
    /* 4. 判断状态字的第 3 位 (Bit[3]) 是否为 1，1 表示已校准 */
    if ((status & 0x08) == 0x00)
    {
        /* 未校准，发送初始化命令 */
        AHT20_IIC_Start();
        AHT20_IIC_SendByte(AHT20_ADDR_WRITE);
        AHT20_IIC_WaitAck();
        AHT20_IIC_SendByte(AHT20_CMD_INIT); // 0xBE
        AHT20_IIC_WaitAck();
        AHT20_IIC_SendByte(0x08);           // 参数 1
        AHT20_IIC_WaitAck();
        AHT20_IIC_SendByte(0x00);           // 参数 2
        AHT20_IIC_WaitAck();
        AHT20_IIC_Stop();
        
        AHT20_IIC_DELAY_US(10000); // 等待初始化完成
    }
    return true;
}

/*
 * @brief  读取 AHT20 温湿度数据
 * @param  temp: 指向温度变量的指针 (单位: ℃)
 * @param  humi: 指向湿度变量的指针 (单位: %)
 * @return true - 读取成功, false - 读取失败
 */
bool AHT20_Read_Temp_Humi(float *temp, float *humi)
{
    uint8_t dat[6];
    uint32_t humi_raw, temp_raw;

    /* ================= 阶段 1：发送触发测量命令 ================= */
    AHT20_IIC_Start();
    AHT20_IIC_SendByte(AHT20_ADDR_WRITE);
    if (!AHT20_IIC_WaitAck()) 
    {
        AHT20_IIC_Stop();
        return false;
    }
    AHT20_IIC_SendByte(AHT20_CMD_MEASURE); // 0xAC
    AHT20_IIC_WaitAck();
    AHT20_IIC_SendByte(0x33);              // 参数 1
    AHT20_IIC_WaitAck();
    AHT20_IIC_SendByte(0x00);              // 参数 2
    AHT20_IIC_WaitAck();
    AHT20_IIC_Stop();

    /* ================= 阶段 2：等待测量完成 ===================== */
    /* 手册要求至少等待 80ms。
       如果你在桌面终端项目中接入了 FreeRTOS，强烈建议将此处的死延时 
       替换为 vTaskDelay(pdMS_TO_TICKS(80)); 以释放 CPU 资源！ */
    DelayNms(80); 

    /* ================= 阶段 3：读取 6 字节数据 ================== */
    AHT20_IIC_Start();
    AHT20_IIC_SendByte(AHT20_ADDR_READ);
    if (!AHT20_IIC_WaitAck()) 
    {
        AHT20_IIC_Stop();
        return false;
    }

    for (uint8_t i = 0; i < 6; i++)
    {
        dat[i] = AHT20_IIC_ReadByte();
        if (i == 5)
        {
            AHT20_IIC_SendNack(); // 最后一个字节回复 NACK
        }
        else
        {
            AHT20_IIC_SendAck();  // 前 5 个字节回复 ACK
        }
    }
    AHT20_IIC_Stop();

    /* ================= 阶段 4：解析数据 ========================= */
    /* 检查状态字 dat[0] 的最高位 (Bit[7])，0 表示空闲，1 表示仍在忙 */
    if ((dat[0] & 0x80) == 0x00)
    {
        /* 拼接 20 位湿度原始数据：dat[1], dat[2], 以及 dat[3] 的高 4 位 */
        humi_raw = ((uint32_t)dat[1] << 12) | ((uint32_t)dat[2] << 4) | (dat[3] >> 4);
        
        /* 拼接 20 位温度原始数据：dat[3] 的低 4 位，dat[4], 以及 dat[5] */
        temp_raw = (((uint32_t)(dat[3] & 0x0F)) << 16) | ((uint32_t)dat[4] << 8) | dat[5];

        /* 代入公式计算结果 (1048576 = 2^20) */
        *humi = ((float)humi_raw / 1048576.0f) * 100.0f;
        *temp = ((float)temp_raw / 1048576.0f) * 200.0f - 50.0f;
        
        return true;
    }

    return false; // 传感器处于忙碌状态，读取失败
}
