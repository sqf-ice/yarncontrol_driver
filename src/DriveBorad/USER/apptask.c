#include "main.h"

u8 start_stop = 0;

TaskHandle_t xHandleTaskKey = NULL;
TaskHandle_t xHandleTaskSend485 = NULL;
TaskHandle_t xHandleTaskRev485 = NULL;
TaskHandle_t xHandleTaskSample = NULL;

SemaphoreHandle_t  xSemaphore_rs485 = NULL;
TimerHandle_t xTimerUser = NULL;

u8 key_reset = 0;
u16 key_reset_time = 0;

u8 Device_Process = PROCESS_STOP;
u8 old_Device_Process = PROCESS_STOP;

u16 k3_short_cnt = 0;
u8 k3_short_flag = 0;
u8 link_err = 0;

void vTaskTaskKey(void *pvParameters)
{
  u8 ucKeyCode;
  while(1)
  {
    ucKeyCode = bsp_GetKey();
    if(ucKeyCode != KEY_NONE)
    {
      switch(ucKeyCode)
      {
        case KEY_DOWN_K1://上限位,传感器清零,电机停止
//          motor_dir = MOTOR_STOP;
//          motor_control(motor_dir); 
          SetNull();//传感器重新校准
//          NULL_IDLE = (u32)hx711_filter();//获得传感器的零点输出，存于NULL中，主要用于天平开机时	 
          NULL_IDLE = Read_HX711_Count() / 100;
          device_info.hx711_offset = (u32)NULLMASS;
          __set_PRIMASK(1);
          Write_flash(USER_FLASH_ADDR,(u16*)&device_info,sizeof(device_info) / 2);
          __set_PRIMASK(0);
          LED1_ON();
//          bsp_StartHardTimer(1,50000,(void *)TIM_CallBack1);
          if((device_info.onoff == 0) || (start_stop == 0))
          {//复位到上限位无开机信号，开始正转一分钟
            Device_Process = PROCESS_RESET_2;
            key_reset = 0;
          }
          else
          {
            Device_Process = PROCESS_RUNNING;
          }
          printf("上限位校准零点\r\n");
          break;
        case KEY_LONG_K1:
          printf("上限位\r\n");
          break;    
        case KEY_DOWN_K2:	//下限位
          Device_Process = PROCESS_RESET;
          printf("下限位复位\r\n");
          break;  
        case KEY_LONG_K2:
          Device_Process = PROCESS_RESET;
          printf("下限位复位\r\n");
          break;  
        case KEY_DOWN_K3:
          if(k3_short_flag == 0)
          {
            k3_short_flag = 1;
            printf("k3 click.\r\n");
          }
          else if(k3_short_flag == 1)
          {
            if(k3_short_cnt < 200)
            {//两次按键时间小于100ms，认为是双击
              k3_short_flag = 2;
              old_Device_Process = Device_Process;
              Device_Process = PROCESS_PAUSE;
              printf("k3 double click.\r\n");
            }
            else
            {
              k3_short_flag = 0;
              k3_short_cnt = 0;
              printf("cancel double click.\r\n");
            }
          }
          else if(k3_short_flag == 2)
          {
            k3_short_flag = 0;
            k3_short_cnt = 0;
            Device_Process = old_Device_Process;
            printf("continue...\r\n");
          }
          break;
        case KEY_LONG_K3://长按复位按钮
          Device_Process = PROCESS_RESET;
          printf("下限位复位\r\n");
          break;
          /* 其他的键值不处理 */
        default:                     
          break;
      }
      ucKeyCode = 0;
    }
    if(k3_short_flag == 1)
    {
      k3_short_cnt++;
      if(k3_short_cnt > 200)
      {
        k3_short_cnt = 0;
        k3_short_flag = 0;
        printf("cancel double click1.\r\n");
      }
    }
    vTaskDelay(10);
    IWDG_Feed();
  }
}

void vTaskTaskSend485(void *pvParameters)
{
  while(1)
  {
    bsp_KeyScan();
    vTaskDelay(10);
    IWDG_Feed();
  }
}

void vTaskRev485(void *pvParameters)
{
  BaseType_t xResult;
  u8 res;
  m_frame_typedef rxframe;
  static u8 err = 0;
  const TickType_t xMaxBlockTime = pdMS_TO_TICKS(300); /* 设置最大等待时间为300ms */
  while(1)
  {
    xResult = xSemaphoreTake(xSemaphore_rs485, (TickType_t)xMaxBlockTime);
    if(xResult == pdTRUE)
    {
      if(m_ctrl_dev.frameok)
      {
        res = mb_unpack_frame(&rxframe);
        if(res == MR_OK)
        {
          modbus_action(&rxframe,load_value);
          link_err = 0;//收到主控命令正确，链接错误标志清零
        }
        else
        {
          err++;
          if(err >= 20)
          {
//            __disable_fault_irq();
            NVIC_SystemReset();
          }
        }
      }
    }
    else
    {
      GPIO_ToggleBits(LED1_PORT,LED1_PIN);
    }
    IWDG_Feed();
  }
}

void vTaskSample(void *pvParameters)
{
  u32 diff;
  InitMass();
//  start_stop = 1;
  while(1)
  {
    get_mass();
    //PA0电压=采样值/4096*3.3
    //采样电阻两端电压V=PA0电压/放大倍数11
    //电机电流=采样电流=采样电压V/采样电阻0.01
    value_current = (float)Average_filter() / 4096 * 3.3 / 11 / 0.01;
    printf("%.3fkg,%.3fA\r\n",(float)load_value / 1000,value_current);
    if(value_current >= SAMPLE_CURRENT_MAX)
    {//过载
      Device_Process = PROCESS_OVERCURRENTS;
    }
    switch(Device_Process)
    {
      case PROCESS_STOP://停止
        if((device_info.onoff == 1) && (start_stop == 1))
        {
          Device_Process = PROCESS_RUNNING;
        }
        break;
      case PROCESS_RUNNING://运行
        if((device_info.onoff == 1) && (start_stop == 1))
        {
          diff = abs(load_value - device_info.weight_value);
          if((diff <= 200) || (load_value <= ZERO))
          {//当前重量和设定值相差0.2kg，或当前重量小于零点值时，停止不动
            motor_dir = MOTOR_STOP;
            motor_control(motor_dir);  
          }
          else
          {//差值大于0.5kg时，连续运转
            u16 speed;
            u8 mode;
            if(diff > 500)
            {
              speed = 850;
            }
            else if((diff > 400) && (diff <= 500))
            {
              speed = 800;
            }
            else if((diff > 300) && (diff <= 400))
            {
              speed = 750;
            }
            else if((diff > 200) && (diff <= 300))
            {
              speed = 500;
            }
            if(load_value > device_info.weight_value)
            {
              motor_speed(speed);
              motor_dir = MOTOR_REVERSE;
              motor_control(motor_dir);
            }
            else if(load_value < device_info.weight_value)
            {
              motor_speed(speed);
              motor_dir = MOTOR_FORWARD;
              motor_control(motor_dir);
            }
          }
        }
        else 
        {
          Device_Process = PROCESS_STOP;
          motor_dir = MOTOR_STOP;
          motor_control(motor_dir);
        }
        break;
      case PROCESS_RESET://复位
        motor_dir = MOTOR_REVERSE;
        motor_control(motor_dir);
//        Device_Process = PROCESS_RESET_1;
        break;
//      case PROCESS_RESET_1:
//        break; 
      case PROCESS_RESET_2:
        if((device_info.onoff == 1) && (start_stop == 1))
        {
          Device_Process = PROCESS_RUNNING;
          key_reset = 0;
          key_reset_time = 0;
        }
        else
        {
          motor_dir = MOTOR_FORWARD;
          motor_control(motor_dir);
          if(key_reset == 0)
          {
            key_reset = 1;
            key_reset_time = 0;
          }
          else
          {
            if(key_reset_time >= 25)
            {
              Device_Process = PROCESS_STOP;
              motor_dir = MOTOR_STOP;
              motor_control(motor_dir);
            }
          }
        }
        break;    
      case PROCESS_OVERCURRENTS:
        motor_dir = MOTOR_STOP;
        motor_control(motor_dir);
        break; 
      case PROCESS_PAUSE:
        motor_dir = MOTOR_STOP;
        motor_control(motor_dir);
        break;
      default:
        break;
    }
    vTaskDelay(100);
    IWDG_Feed();
  }
}

void AppTaskCreate (void)
{
  xTaskCreate( vTaskTaskKey,            /* 任务函数  */
              "vTaskTaskKey",           /* 任务名    */
              128,                     /* stack大小，单位word，也就是4字节 */
              NULL,                    /* 任务参数  */
              1,                       /* 任务优先级*/
              &xHandleTaskKey);  /* 任务句柄  */
  xTaskCreate( vTaskTaskSend485,   /* 任务函数  */
              "vTaskTaskSend485",     /* 任务名    */
              128,               /* stack大小，单位word，也就是4字节 */
              NULL,              /* 任务参数  */
              2,                 /* 任务优先级*/
              &xHandleTaskSend485 );  /* 任务句柄  */
  
  xTaskCreate( vTaskSample,            /* 任务函数  */
              "vTaskSample",           /* 任务名    */
              256,                     /* stack大小，单位word，也就是4字节 */
              NULL,                    /* 任务参数  */
              3,                       /* 任务优先级*/
              &xHandleTaskSample);  /* 任务句柄  */
  xTaskCreate( vTaskRev485,           /* 任务函数  */
              "vTaskRev485",         /* 任务名    */
              512,                /* stack大小，单位word，也就是4字节 */
              NULL,               /* 任务参数  */
              4,                  /* 任务优先级*/
              &xHandleTaskRev485 ); /* 任务句柄  */
}

void UserTimerCallback(TimerHandle_t xTimer)
{
  if(key_reset == 1)
  {
    if(Device_Process != PROCESS_PAUSE)
      key_reset_time++;
  }
  else
    key_reset_time = 0;
  link_err++;
  if(link_err >= 10)
  {//10s内未收到链接命令，重启
    link_err = 0;
    NVIC_SystemReset();
  }
}

/*
*********************************************************************************************************
*	函 数 名: AppObjCreate
*	功能说明: 创建任务通信机制
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void AppObjCreate(void)
{
  /* 创建二值信号量，首次创建信号量计数值是0 */
  xSemaphore_rs485 = xSemaphoreCreateBinary();
  
  if(xSemaphore_rs485 == NULL)
  {
    printf("xSemaphore_rs485 failure\r\n");
    /* 没有创建成功，用户可以在这里加入创建失败的处理机制 */
  }
  
  xTimerUser = xTimerCreate("Timer",          /* 定时器名字 */
                             1000,    /* 定时器周期,单位时钟节拍 */
                             pdTRUE,          /* 周期性 */
                             (void *)0,      /* 定时器ID */
                             UserTimerCallback); /* 定时器回调函数 */
    
  if(xTimerUser != NULL)
  {
    
    if(xTimerStart(xTimerUser, 1000) != pdPASS)
    {
      /* 定时器还没有进入激活状态 */
    }
  }
}