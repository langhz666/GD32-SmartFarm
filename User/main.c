/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-08 21:22:10
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 21:21:50
 * @FilePath: \GD32F103C8T6\User\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "gd32f10x.h"
#include "gd32f10x_libopt.h"
#include "app.h"
#include <stdio.h>

int main(void)
{
    App_InitDrivers();
    App_CreateQueues();
    App_CreateSemaphores();
    App_CreateTasks();
    vTaskStartScheduler();

    printf("ERROR: Scheduler failed to start!\r\n");
    while(1);
}

