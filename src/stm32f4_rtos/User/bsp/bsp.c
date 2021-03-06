/*
*********************************************************************************************************
*
*	模块名称 : BSP模块(For STM32F4XX)
*	文件名称 : bsp.c
*	版    本 : V1.1
*	说    明 : 这是硬件底层驱动程序的主文件。每个c文件可以 #include "bsp.h" 来包含所有的外设驱动模块。
*			   bsp = Borad surport packet 板级支持包
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2013-03-01  armfly   正式发布
*		V1.1    2013-06-20  armfly   规范注释，添加必要说明
*
*	Copyright (C), 2013-2014, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "includes.h"

/*
*********************************************************************************************************
*	函 数 名: bsp_Init
*	功能说明: 初始化所有的硬件设备。该函数配置CPU寄存器和外设的寄存器并初始化一些全局变量。只需要调用一次
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_Init(void)
{
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  my_mem_init(SRAMIN);
  bsp_InitUart(); 	/* 初始化串口 */
  bsp_Init_Relay();
  bsp_InitStepMotor();
  bsp_InitDWT();       /* 初始DWT */
  SMEC_Test();
  bsp_InitSdwe050c17();
  bsp_InitHardTimer(); /* 初始化TIM2定时器 */
//  bsp_Init_SW_ONOFF();
  bsp_Init_SEVRO_EN();
  W25QXX_Init();
  bsp_InitLed(); 		/* 初始LED指示灯端口 */
  bsp_InitKey();
  read_device_info();
  bkp_init();
  para_init(&SlavePara);
  Dac1_Init();
//  Adc_Init();
  mb_init(0);
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
