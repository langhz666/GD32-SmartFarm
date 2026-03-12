#include "driver_bmp280/driver_bmp280.h"
#include "delay.h"

/* * BMP280 IIC地址：
 * 模块上的 SDO 引脚默认接地，此时地址为 0x76；如果接到 3.3V，地址为 0x77
 */
#define BMP280_IIC_ADDR     0x77

/* BMP280 读写地址宏定义 */
#define BMP280_ADDR_WRITE   ((BMP280_IIC_ADDR << 1) | 0x00) // 0xEC
#define BMP280_ADDR_READ    ((BMP280_IIC_ADDR << 1) | 0x01) // 0xED

/* 保存出厂校准参数的结构体 */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_CalibData_t;

static BMP280_CalibData_t bmp280_cal;
static int32_t t_fine; // 官方补偿算法所需的全局中间变量


/* ========================= GPIO初始化 ========================= */
/*
 * @brief  初始化软件IIC使用的GPIO（PB10=SCL, PB11=SDA）
 * @note   配置为开漏输出，符合I2C总线电气特性
 */
static void IIC_GpioInit(void)
{
    /* 使能GPIOB时钟 */
    rcu_periph_clock_enable(BMP280_IIC_RCU_PORT);

    /* PB10/PB11 配置为开漏输出 */
    gpio_init(BMP280_IIC_PORT, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, BMP280_IIC_SCL_PIN | BMP280_IIC_SDA_PIN);

    /* 默认释放总线，进入空闲态（SCL=1, SDA=1） */
    BMP280_IIC_SCL_HIGH();
    BMP280_IIC_SDA_HIGH();
}

/*
 * @brief  软件IIC初始化
 */
void BMP280_IIC_Init(void)
{
    IIC_GpioInit();
}

/* ========================= IIC基本时序 ========================= */
void BMP280_IIC_Start(void)
{
    BMP280_IIC_SDA_HIGH();
    BMP280_IIC_SCL_HIGH();
    BMP280_IIC_DELAY_US(4);

    BMP280_IIC_SDA_LOW();
    BMP280_IIC_DELAY_US(4);

    BMP280_IIC_SCL_LOW();
    BMP280_IIC_DELAY_US(4);
}

void BMP280_IIC_Stop(void)
{
    BMP280_IIC_SDA_LOW();
    BMP280_IIC_DELAY_US(4);

    BMP280_IIC_SCL_HIGH();
    BMP280_IIC_DELAY_US(4);

    BMP280_IIC_SDA_HIGH();
    BMP280_IIC_DELAY_US(4);
}

void BMP280_IIC_SendByte(uint8_t tx_byte)
{
    for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++)
    {
        if (tx_byte & 0x80U)
        {
            BMP280_IIC_SDA_HIGH();
        }
        else
        {
            BMP280_IIC_SDA_LOW();
        }

        tx_byte <<= 1;
        BMP280_IIC_DELAY_US(4);

        BMP280_IIC_SCL_HIGH();
        BMP280_IIC_DELAY_US(4);

        BMP280_IIC_SCL_LOW();
        BMP280_IIC_DELAY_US(4);
    }
}

uint8_t BMP280_IIC_ReadByte(void)
{
    uint8_t rx_byte = 0U;

    BMP280_IIC_SDA_HIGH();

    for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++)
    {
        rx_byte <<= 1;

        BMP280_IIC_SCL_HIGH();
        BMP280_IIC_DELAY_US(4);

        if (BMP280_IIC_READ_SDA() != 0U)
        {
            rx_byte |= 0x01U;
        }

        BMP280_IIC_DELAY_US(4);

        BMP280_IIC_SCL_LOW();
        BMP280_IIC_DELAY_US(4);
    }

    return rx_byte;
}

bool BMP280_IIC_WaitAck(void)
{
    uint8_t err_times = 0U;

    BMP280_IIC_SDA_HIGH();
    BMP280_IIC_DELAY_US(4);

    BMP280_IIC_SCL_HIGH();
    BMP280_IIC_DELAY_US(4);

    while (BMP280_IIC_READ_SDA() != 0U)
    {
        err_times++;
        if (err_times > 250U)
        {
            BMP280_IIC_Stop();
            return false;
        }
    }

    BMP280_IIC_SCL_LOW();
    BMP280_IIC_DELAY_US(4);
    return true;
}

void BMP280_IIC_SendAck(void)
{
    BMP280_IIC_SDA_LOW();
    BMP280_IIC_DELAY_US(4);
    BMP280_IIC_SCL_HIGH();
    BMP280_IIC_DELAY_US(4);
    BMP280_IIC_SCL_LOW();
    BMP280_IIC_DELAY_US(4);
    BMP280_IIC_SDA_HIGH();
}

void BMP280_IIC_SendNack(void)
{
    BMP280_IIC_SDA_HIGH();
    BMP280_IIC_DELAY_US(4);
    BMP280_IIC_SCL_HIGH();
    BMP280_IIC_DELAY_US(4);
    BMP280_IIC_SCL_LOW();
    BMP280_IIC_DELAY_US(4);
}


/* ========================= BMP280 内部读写封装 ========================= */
/*
 * @brief  向 BMP280 指定寄存器写入 1 字节数据
 */
static void BMP280_WriteReg(uint8_t reg, uint8_t data)
{
    BMP280_IIC_Start();
    BMP280_IIC_SendByte(BMP280_ADDR_WRITE);
    BMP280_IIC_WaitAck();
    BMP280_IIC_SendByte(reg);
    BMP280_IIC_WaitAck();
    BMP280_IIC_SendByte(data);
    BMP280_IIC_WaitAck();
    BMP280_IIC_Stop();
}

/*
 * @brief  从 BMP280 指定寄存器连续读取多字节数据
 */
static void BMP280_ReadRegs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    BMP280_IIC_Start();
    BMP280_IIC_SendByte(BMP280_ADDR_WRITE);
    BMP280_IIC_WaitAck();
    BMP280_IIC_SendByte(reg);
    BMP280_IIC_WaitAck();
    
    BMP280_IIC_Start(); // 重复起始信号
    BMP280_IIC_SendByte(BMP280_ADDR_READ);
    BMP280_IIC_WaitAck();
    
    for (uint8_t i = 0; i < len; i++) 
    {
        buf[i] = BMP280_IIC_ReadByte();
        if (i == len - 1) 
        {
            BMP280_IIC_SendNack(); // 最后一个字节回复 NACK
        } 
        else 
        {
            BMP280_IIC_SendAck();  // 前面的字节回复 ACK
        }
    }
    BMP280_IIC_Stop();
}


/* ========================= BMP280 应用层函数 ========================= */
/*
 * @brief  BMP280 传感器初始化
 * @return true - 成功, false - 失败 (未读到正确的设备ID)
 */
bool BMP280_Init(void)
{
    uint8_t chip_id;
    uint8_t calib[24];
    
    /* 1. 底层 IIC 引脚初始化 */
    BMP280_IIC_Init();
    
    /* 2. 读取芯片 ID 进行通信验证 (BMP280 的 ID 固定为 0x58) */
    BMP280_ReadRegs(0xD0, &chip_id, 1);
    if (chip_id != 0x58) 
    {
        return false; // 通信失败或不是 BMP280
    }
    
    /* 3. 读取 0x88 到 0xA1 的 24 字节出厂补偿参数 */
    BMP280_ReadRegs(0x88, calib, 24);
    
    /* 拼装补偿数据 (低位在前，高位在后) */
    bmp280_cal.dig_T1 = (calib[1] << 8) | calib[0];
    bmp280_cal.dig_T2 = (calib[3] << 8) | calib[2];
    bmp280_cal.dig_T3 = (calib[5] << 8) | calib[4];
    bmp280_cal.dig_P1 = (calib[7] << 8) | calib[6];
    bmp280_cal.dig_P2 = (calib[9] << 8) | calib[8];
    bmp280_cal.dig_P3 = (calib[11] << 8) | calib[10];
    bmp280_cal.dig_P4 = (calib[13] << 8) | calib[12];
    bmp280_cal.dig_P5 = (calib[15] << 8) | calib[14];
    bmp280_cal.dig_P6 = (calib[17] << 8) | calib[16];
    bmp280_cal.dig_P7 = (calib[19] << 8) | calib[18];
    bmp280_cal.dig_P8 = (calib[21] << 8) | calib[20];
    bmp280_cal.dig_P9 = (calib[23] << 8) | calib[22];

    /* 4. 配置测量控制寄存器 0xF4
     * 0x27 的含义：温度过采样 x1，气压过采样 x1，Normal mode(连续测量) */
    BMP280_WriteReg(0xF4, 0x27);
    
    /* 5. 配置配置寄存器 0xF5 (关闭 IIR 滤波器，设为 0x00 默认值) */
    BMP280_WriteReg(0xF5, 0x00);
    
    return true;
}

/*
 * @brief  读取 BMP280 温度和气压数据 (Bosch 官方 32位 整数补偿算法)
 * @param  temp: 指向温度变量的指针 (单位: ℃)
 * @param  press: 指向气压变量的指针 (单位: Pa)
 */
void BMP280_Read_Temp_Press(float *temp, float *press)
{
    uint8_t raw[6];
    int32_t adc_T, adc_P;
    int32_t var1, var2, T;
    uint32_t p;

    /* 从 0xF7 开始读取 6 个字节 (压强 MSB...LBS, 温度 MSB...LSB) */
    BMP280_ReadRegs(0xF7, raw, 6);

    adc_P = (raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4);
    adc_T = (raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4);

    /* ================= 阶段 1：温度补偿算法 ===================== */
    var1 = ((((adc_T >> 3) - ((int32_t)bmp280_cal.dig_T1 << 1))) * ((int32_t)bmp280_cal.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)bmp280_cal.dig_T1)) * ((adc_T >> 4) - ((int32_t)bmp280_cal.dig_T1))) >> 12) * ((int32_t)bmp280_cal.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    *temp = T / 100.0f;

    /* ================= 阶段 2：气压补偿算法 ===================== */
    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)bmp280_cal.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)bmp280_cal.dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)bmp280_cal.dig_P4) << 16);
    var1 = (((bmp280_cal.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)bmp280_cal.dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)bmp280_cal.dig_P1)) >> 15);
    
    if (var1 == 0) 
    {
        *press = 0; // 防止分母为 0 导致程序崩溃
        return;
    }
    
    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (p < 0x80000000) { p = (p << 1) / ((uint32_t)var1); } 
    else { p = (p / (uint32_t)var1) * 2; }
    
    var1 = (((int32_t)bmp280_cal.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)bmp280_cal.dig_P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + bmp280_cal.dig_P7) >> 4));
    
    *press = (float)p; 
}
