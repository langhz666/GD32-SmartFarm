#include "driver_w25q64/driver_w25q64.h"

/**
  * @brief  初始化 W25Q64 的引脚
  */
void W25Q64_Init(void)
{
    rcu_periph_clock_enable(W25Q_RCU);

    // CS, CLK, MOSI 配置为推挽输出
    gpio_init(W25Q_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, W25Q_CS_PIN | W25Q_CLK_PIN | W25Q_MOSI_PIN);
    // MISO 配置为上拉输入
    gpio_init(W25Q_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, W25Q_MISO_PIN);

    // 初始化时置高 CS 片选，取消选中芯片
    W25Q_CS_HIGH();
    // SPI 模式 0，空闲时时钟线为低电平
    W25Q_CLK_LOW(); 
}

/**
  * @brief  软件模拟 SPI 发送并接收一个字节 (核心函数)
  * @param  byte: 要发送的字节
  * @retval 接收到的字节
  */
static uint8_t SPI_SwapByte(uint8_t byte)
{
    uint8_t i, rx_data = 0;
    
    // SPI 传输是 MSB (最高位) 在前
    for (i = 0; i < 8; i++) 
    {
        // 1. 准备好要发出的数据 (MOSI)
        if (byte & 0x80) W25Q_MOSI_HIGH();
        else             W25Q_MOSI_LOW();
        byte <<= 1; // 移位，为下一次做准备
        
        // 2. 拉高时钟线产生上升沿，W25Q64 在此时读取我们发出的 MOSI
        W25Q_CLK_HIGH();
        
        // 3. 读取 W25Q64 发来的数据 (MISO)
        rx_data <<= 1;
        if (W25Q_MISO_READ()) rx_data |= 0x01;
        
        // 4. 拉低时钟线产生下降沿，完成一个位的传输，W25Q64 在此时准备下一位数据
        W25Q_CLK_LOW();
    }
    return rx_data;
}

/**
  * @brief  读取厂商和设备 ID
  * @retval 返回值，如 W25Q64 会返回 0x4017 或类似 ID
  */
uint16_t W25Q64_ReadID(void)
{
    uint16_t temp = 0;
    
    W25Q_CS_LOW();                     // 拉低片选，选中芯片
    SPI_SwapByte(W25X_JedecDeviceID);  // 发送读取 ID 指令 0x9F
    SPI_SwapByte(0xFF);                // 读取厂商 ID (接收数据时发送 0xFF 占位)
    temp = SPI_SwapByte(0xFF) << 8;    // 读取设备 ID 高 8 位
    temp |= SPI_SwapByte(0xFF);        // 读取设备 ID 低 8 位
    W25Q_CS_HIGH();                    // 拉高片选，取消选中
    
    return temp;
}

/**
  * @brief  读取状态寄存器并等待芯片空闲
  */
static void W25Q64_WaitBusy(void)
{
    uint8_t status = 0;
    W25Q_CS_LOW();
    SPI_SwapByte(W25X_ReadStatusReg); // 发送读状态寄存器指令 0x05
    do {
        status = SPI_SwapByte(0xFF);  // 持续读取状态寄存器
    } while ((status & 0x01) == 0x01); // 检查 BUSY 位 (最低位) 是否为 1
    W25Q_CS_HIGH();
}

/**
  * @brief  写使能
  */
static void W25Q64_WriteEnable(void)
{
    W25Q_CS_LOW();
    SPI_SwapByte(W25X_WriteEnable);   // 发送写使能指令 0x06
    W25Q_CS_HIGH();
}

/**
  * @brief  擦除一个扇区 (Sector: 4KB)
  * @param  Dst_Addr: 扇区地址 (必须是 4096 的整数倍)
  */
void W25Q64_EraseSector(uint32_t Dst_Addr)
{
    W25Q64_WriteEnable();             // 擦除前必须先写使能
    W25Q64_WaitBusy();                // 等待芯片空闲
    
    W25Q_CS_LOW();
    SPI_SwapByte(W25X_SectorErase);   // 发送扇区擦除指令 0x20
    SPI_SwapByte((Dst_Addr & 0xFF0000) >> 16); // 发送 24 位地址的高 8 位
    SPI_SwapByte((Dst_Addr & 0xFF00) >> 8);    // 中 8 位
    SPI_SwapByte(Dst_Addr & 0xFF);             // 低 8 位
    W25Q_CS_HIGH();
    
    W25Q64_WaitBusy();                // 等待擦除完成 (擦除需要时间)
}

/**
  * @brief  页编程 (写入数据)，最大只能写一页 (256字节)
  * @param  WriteAddr: 写入地址
  * @param  pBuffer: 数据指针
  * @param  NumByteToWrite: 写入字节数 (1 ~ 256)
  */
void W25Q64_PageProgram(uint32_t WriteAddr, uint8_t* pBuffer, uint16_t NumByteToWrite)
{
    W25Q64_WriteEnable();             // 写入前必须先写使能
    W25Q64_WaitBusy();                // 等待芯片空闲
    
    W25Q_CS_LOW();
    SPI_SwapByte(W25X_PageProgram);   // 发送页编程指令 0x02
    SPI_SwapByte((WriteAddr & 0xFF0000) >> 16);
    SPI_SwapByte((WriteAddr & 0xFF00) >> 8);
    SPI_SwapByte(WriteAddr & 0xFF);
    
    for (uint16_t i = 0; i < NumByteToWrite; i++) {
        SPI_SwapByte(pBuffer[i]);     // 循环写入数据
    }
    W25Q_CS_HIGH();
    
    W25Q64_WaitBusy();                // 等待写入完成
}

/**
  * @brief  读取连续的数据
  * @param  ReadAddr: 起始地址
  * @param  pBuffer: 存放读取数据的数组指针
  * @param  NumByteToRead: 要读取的字节数 (读取数量没有限制，可以跨页)
  */
void W25Q64_ReadData(uint32_t ReadAddr, uint8_t* pBuffer, uint16_t NumByteToRead)
{
    W25Q64_WaitBusy();                // 等待芯片空闲
    
    W25Q_CS_LOW();
    SPI_SwapByte(W25X_ReadData);      // 发送读取数据指令 0x03
    SPI_SwapByte((ReadAddr & 0xFF0000) >> 16);
    SPI_SwapByte((ReadAddr & 0xFF00) >> 8);
    SPI_SwapByte(ReadAddr & 0xFF);
    
    for (uint16_t i = 0; i < NumByteToRead; i++) {
        pBuffer[i] = SPI_SwapByte(0xFF); // 连续读取，发送 0xFF 提供时钟信号
    }
    W25Q_CS_HIGH();
}

void W25Q64_Test(void)
{ 
    uint16_t flash_id = 0;
    uint16_t data_length = sizeof(WriteBuffer); // 计算字符串的长度（包含结尾的'\0'）
    W25Q64_Init();
    printf("\r\n=== W25Q64 Flash Read/Write Test ===\r\n");
    flash_id = W25Q64_ReadID();
    printf("Read Flash ID: 0x%04X\r\n", flash_id);
    if (flash_id == 0xFFFF || flash_id == 0x0000) 
    {
        printf("Error: W25Q64 not found! Please check wiring and power.\r\n");
        while(1); // 停机报错
    }
    else 
    {
        printf("W25Q64 detected successfully!\r\n");
    }

    // 4. 擦除测试扇区 (写入前必须擦除！)
    printf("Erasing Sector 0 (Address 0x%06X)...\r\n", TEST_FLASH_ADDR);
    W25Q64_EraseSector(TEST_FLASH_ADDR);
    printf("Erase completed.\r\n");

    // 5. 将测试字符串写入 Flash
    printf("Writing data to Flash...\r\n");
    W25Q64_PageProgram(TEST_FLASH_ADDR, WriteBuffer, data_length); 
    printf("Write completed.\r\n");

    // 6. 从同一地址将数据读回到 ReadBuffer 中
    printf("Reading data from Flash...\r\n");
    W25Q64_ReadData(TEST_FLASH_ADDR, ReadBuffer, data_length);
    printf("Read Output: %s\r\n", ReadBuffer);
    
    // 7. 校验写入的数据和读出的数据是否完全一致
    if(strcmp((char*)WriteBuffer, (char*)ReadBuffer) == 0)
    {
        printf("\r\n>>> TEST PASS! Data matches perfectly. <<<\r\n");
    }
    else
    {
        printf("\r\n>>> TEST FAIL! Data mismatch. <<<\r\n");
    }
}
