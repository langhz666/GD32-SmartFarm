#include "driver_light/iic_light.h"
#include "delay.h"


/* ========================= GPIO初始化 ========================= */
/*
 * @brief  初始化软件IIC使用的GPIO（PA0=SCL, PA1=SDA）
 * @note   配置为开漏输出，符合I2C总线电气特性
 */
static void IIC_Light_GpioInit(void)
{
    /* 使能GPIOA时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* PA0/PA1 配置为开漏输出 */
    gpio_init(GPIOA, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_0 | GPIO_PIN_1);

    /* 默认释放总线，进入空闲态（SCL=1, SDA=1） */
    IIC_SCL_HIGH();
    IIC_SDA_HIGH();
}

/*
 * @brief  软件IIC初始化
 */
void IIC_Light_Init(void)
{
    IIC_Light_GpioInit();
}


/* ========================= IIC基本时序 ========================= */
/*
 * @brief  产生IIC起始信号（Start）
 * @note   起始条件：SCL高电平期间，SDA由高变低
 */
void IIC_Light_Start(void)
{
    /* 确保总线空闲态 */
    IIC_SDA_HIGH();
    IIC_SCL_HIGH();
    IIC_DELAY_US(4);

    /* 产生Start条件 */
    IIC_SDA_LOW();
    IIC_DELAY_US(4);

    /* 拉低SCL，进入数据传输阶段 */
    IIC_SCL_LOW();
    IIC_DELAY_US(4);
}

/*
 * @brief  产生IIC停止信号（Stop）
 * @note   停止条件：SCL高电平期间，SDA由低变高
 */
void IIC_Light_Stop(void)
{
    /* 先保持SDA为低 */
    IIC_SDA_LOW();
    IIC_DELAY_US(4);

    /* 拉高SCL */
    IIC_SCL_HIGH();
    IIC_DELAY_US(4);

    /* 产生Stop条件 */
    IIC_SDA_HIGH();
    IIC_DELAY_US(4);
}


/* ========================= 发送/接收1字节 ========================= */
/*
 * @brief  发送1个字节（高位在前，MSB first）
 * @param  tx_byte: 待发送数据
 */
void IIC_Light_SendByte(uint8_t tx_byte)
{
    for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++)
    {
        /* 1) 在SCL低电平期间准备当前位数据（先发最高位） */
        if (tx_byte & 0x80U)
        {
            IIC_SDA_HIGH();   /* 当前位为1：释放SDA */
        }
        else
        {
            IIC_SDA_LOW();    /* 当前位为0：拉低SDA */
        }

        tx_byte <<= 1;
        IIC_DELAY_US(4);

        /* 2) 拉高SCL：从机在高电平期间采样SDA */
        IIC_SCL_HIGH();
        IIC_DELAY_US(4);

        /* 3) 拉低SCL：本位发送结束，准备下一位 */
        IIC_SCL_LOW();
        IIC_DELAY_US(4);
    }
}

/*
 * @brief  读取1个字节（高位在前，MSB first）
 * @return 读取到的1字节数据
 */
uint8_t IIC_Light_ReadByte(void)
{
    uint8_t rx_byte = 0U;

    /* 释放SDA，由从机驱动数据线 */
    IIC_SDA_HIGH();

    for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++)
    {
        rx_byte <<= 1;

        /* 拉高SCL，从机输出当前位，主机采样 */
        IIC_SCL_HIGH();
        IIC_DELAY_US(4);

        if (IIC_READ_SDA() != 0U)
        {
            rx_byte |= 0x01U;
        }

        IIC_DELAY_US(4);

        /* 拉低SCL，准备读取下一位 */
        IIC_SCL_LOW();
        IIC_DELAY_US(4);
    }

    return rx_byte;
}
/* ========================= ACK/NACK相关 ========================= */
/*
 * @brief  等待从机应答（ACK）
 * @return 0 - 成功收到ACK（SDA被从机拉低）
 * @return 1 - 超时未收到ACK（发生错误）
 */
uint8_t IIC_Light_WaitAck(void)
{
    uint8_t err_times = 0U;

    /* 主机释放SDA，让从机驱动应答位 */
    IIC_SDA_HIGH();
    IIC_DELAY_US(4);

    /* 拉高SCL，开始ACK采样 */
    IIC_SCL_HIGH();
    IIC_DELAY_US(4);

    /* 等待从机拉低SDA */
    while (IIC_READ_SDA() != 0U)
    {
        err_times++;

        if (err_times > 250U)
        {
            /* 超时无应答，发送停止信号退出 */
            IIC_Light_Stop();
            return 1;  // <--- 修改这里：超时返回 1 (表示 Error)
        }
    }

    /* 完成ACK采样后拉低SCL */
    IIC_SCL_LOW();
    IIC_DELAY_US(4);

    return 0; // <--- 修改这里：成功返回 0 (表示 OK)
}

/*
 * @brief  主机发送ACK（应答）
 * @note   用于主机读数据时，告知从机“继续发送下一字节”
 */
void IIC_Light_SendAck(void)
{
    IIC_SDA_LOW();          /* ACK位 = 0 */
    IIC_DELAY_US(4);

    IIC_SCL_HIGH();         /* 第9个时钟脉冲 */
    IIC_DELAY_US(4);

    IIC_SCL_LOW();
    IIC_DELAY_US(4);

    IIC_SDA_HIGH();         /* 释放SDA */
}

/*
 * @brief  主机发送NACK（非应答）
 * @note   用于主机读完最后一个字节后，告知从机“结束读取”
 */
void IIC_Light_SendNack(void)
{
    IIC_SDA_HIGH();         /* NACK位 = 1（释放SDA） */
    IIC_DELAY_US(4);

    IIC_SCL_HIGH();         /* 第9个时钟脉冲 */
    IIC_DELAY_US(4);

    IIC_SCL_LOW();
    IIC_DELAY_US(4);
}


/**
 * @brief  I2C write data
 * @param  SlaveAddress: slave address
 * @param  Data: data buffer
 * @param  len: data length
 * @retval 0: success 1: fail
 */
uint8_t IIC_Light_WriteData(uint8_t SlaveAddress, uint8_t *Data, uint8_t len) 
{
  uint8_t i = 0;
  IIC_Light_Start();
  IIC_Light_SendByte(SlaveAddress);
  if (IIC_Light_WaitAck()) 
  {
    IIC_Light_Stop();
    return 1;
  }
  for (i = 0; i < len; i++)
  {
    IIC_Light_SendByte(Data[i]);
    if (IIC_Light_WaitAck())
    {
      IIC_Light_Stop();
      return 1;
    }
  }
  IIC_Light_Stop();
  return 0;
}

/**
 * @brief  I2C read data
 * @param  SlaveAddress: slave address
 * @param  Data: data buffer
 * @param  len: data length
 * @retval 0: success 1: fail
 */
uint8_t IIC_Light_ReadData(uint8_t SlaveAddress, uint8_t *Data, uint8_t len)
{
  uint8_t i = 0;
  IIC_Light_Start();
  IIC_Light_SendByte(SlaveAddress + 1);
  if (IIC_Light_WaitAck())
  {
    IIC_Light_Stop();
    return 1;
  }
  for (i = 0; i < len; i++)
  {
    Data[i] = IIC_Light_ReadByte();
    if (i == (len - 1)) IIC_Light_SendNack();
    else IIC_Light_SendAck();
  }
  IIC_Light_Stop();
  return 0;
}


