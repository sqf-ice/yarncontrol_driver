#include "bsp.h"

u8 lcd_rev_buf[50];
u16 lcd_rev_len = 0;
u8 lcd_busy = 0;

void bsp_InitSdwe050c17(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /* 打开GPIO时钟 */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;		/* 设为输出口 */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;		/* 设为推挽模式 */
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	/* 上下拉电阻不使能 */
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	/* IO口最大速度 */
  GPIO_InitStructure.GPIO_Pin = LCD_POWER_PIN;
  GPIO_Init(LCD_POWER_PORT, &GPIO_InitStructure);
  
//  LCD_POWER_OFF();
  LCD_POWER_ON();
}

void Init_JINGSHA_GUI(void)
{
  u8 i;
  u8 name[20],name_1[20];
  u8 name_len;
  u8 id[10];
  memset(name,0,20);
  memset(name_1,0,20);
  memset(id,0,10);
  memcpy(name_1,SlavePara.filename,SlavePara.filename_len);
//  sprintf((char *)name,"%d号机-%s",device_info.master_id,name_1);
  memcpy(id,device_info.device_id,device_info.device_id_len);
  sprintf((char *)name,"%s-%s.CSV",id,name_1);
  name_len = strlen((char const*)name);
  Sdwe_disString(PAGE1_TEXT_FILE_NUM,name,name_len);
//  Sdwe_disString(PAGE1_SYSTEM_STATE,"系统正常...",strlen("系统正常..."));
  Sdwe_disString(PAGE1_SYSTEM_STATE,(u8 *)system_state_dis[device_info.system_state],strlen(system_state_dis[device_info.system_state]));
  
  for(i=0;i<30;i++)
  {
    Sdwe_disDigi(PAGE1_SET_VALUE1 + i,SlavePara.value_set[i] / 10,2);
    vTaskDelay(5);
    Sdwe_writeIcon(PAGE1_SLAVE_ONOFF1 + i,SlavePara.onoff[i]);
    vTaskDelay(5);
  }
}

//调用图片
//addr：0x03
void Sdwe_disPicture(u8 picture)
{
  u8 sendbuf[20];
  sendbuf[0] = 0xA5;
  sendbuf[1] = 0x5A;
  sendbuf[2] = 0x04; 
  sendbuf[3] = VGUS_REGISTER_WRITE;
  sendbuf[4] = VGUS_PIC_ID;
  sendbuf[5] = 0x00;
  sendbuf[6] = picture;
  lcd_busy = 1;
  UART4ToPC(sendbuf,7);
  lcd_busy = 0;
}

//在指定位置显示数字  
void Sdwe_disDigi(u16 addr,u32 data,u8 bytes)
{
  u8 sendbuf[20];
  sendbuf[0] = 0xA5;
  sendbuf[1] = 0x5A;
  lcd_busy = 1;
  if(bytes == 2)
  {
    sendbuf[2] = 0x05; 
    sendbuf[3] = VGUS_VARIABLE_WRITE;
    sendbuf[4] = (addr & 0xff00) >> 8;
    sendbuf[5] = addr & 0x00ff;
    sendbuf[6] = (data & 0xff00) >> 8;
    sendbuf[7] = data & 0x00ff;
    UART4ToPC(sendbuf,8);
  }
  else if(bytes == 4)
  {
    sendbuf[2] = 0x07; 
    sendbuf[3] = VGUS_VARIABLE_WRITE;
    sendbuf[4] = (addr & 0xff00) >> 8;
    sendbuf[5] = addr & 0x00ff;
    sendbuf[6] = (data & 0xff000000) >> 24;
    sendbuf[7] = (data & 0xff0000) >> 16;
    sendbuf[8] = (data & 0xff00) >> 8;
    sendbuf[9] = data & 0xff;
    UART4ToPC(sendbuf,10);
  }
  lcd_busy = 0;
}

//在指定位置显示字符
//addr：变量存储器地址
//*str:要显示的字符指针
//len:要显示的字符长度
void Sdwe_disString(u16 addr,u8 *str,u16 len)
{
  u8 i,sendbuf[50];
  sendbuf[0] = 0xA5;
  sendbuf[1] = 0x5A;
  sendbuf[2] = len + 5; 
  sendbuf[3] = VGUS_VARIABLE_WRITE;
  sendbuf[4] = (addr & 0xff00) >> 8;
  sendbuf[5] = addr & 0x00ff;
  for(i=0;i<len;i++)
    sendbuf[6 + i] = str[i];
  sendbuf[6 + len] = 0xFF;
  sendbuf[7 + len] = 0xFF;
  lcd_busy = 1;
  UART4ToPC(sendbuf,8 + len);
  lcd_busy = 0;
}

void SDWE_WARNNING(u16 addr,u8 *str)
{
  Sdwe_disString(addr,str,strlen((char const*)str));
  bsp_StartHardTimer(1 ,500000, (void *)TIM_CallBack1);
}

void Sdwe_clearString(u16 addr)
{
  u8 sendbuf[10];
  sendbuf[0] = 0xA5;
  sendbuf[1] = 0x5A;
  sendbuf[2] = 5; 
  sendbuf[3] = VGUS_VARIABLE_WRITE;
  sendbuf[4] = (addr & 0xff00) >> 8;
  sendbuf[5] = addr & 0x00ff;
  sendbuf[6] = 0xFF;
  sendbuf[7] = 0xFF;
  lcd_busy = 1;
  UART4ToPC(sendbuf,8);
  lcd_busy = 0;
}

void Sdwe_writeIcon(u16 addr,u16 state)
{
  u8 sendbuf[10];
  sendbuf[0] = 0xA5;
  sendbuf[1] = 0x5A;
  sendbuf[2] = 5; 
  sendbuf[3] = VGUS_VARIABLE_WRITE;
  sendbuf[4] = (addr & 0xff00) >> 8;
  sendbuf[5] = addr & 0x00ff;
  sendbuf[6] = (state & 0xff00) >> 8;
  sendbuf[7] = state & 0x00ff;
  lcd_busy = 1;
  UART4ToPC(sendbuf,8);
  lcd_busy = 0;
}

void Sdwe_ClearReadDisk(void)
{
  u8 i;
  for(i=0;i<10;i++)
    Sdwe_writeIcon(PAGE_U_ICON_SELECT1,0);
}

void Sdwe_readIcon(u16 addr)
{
  u8 sendbuf[10];
  sendbuf[0] = 0xA5;
  sendbuf[1] = 0x5A;
  sendbuf[2] = 4; 
  sendbuf[3] = VGUS_VARIABLE_WRITE;
  sendbuf[4] = (addr & 0xff00) >> 8;
  sendbuf[5] = addr & 0x00ff;
  sendbuf[6] = 1;
  lcd_busy = 1;
  UART4ToPC(sendbuf,7);
  lcd_busy = 0;
}

void Sdwe_readRTC(void)
{
  u8 sendbuf[10];
  sendbuf[0] = 0xA5;
  sendbuf[1] = 0x5A;
  sendbuf[2] = 3; 
  sendbuf[3] = VGUS_REGISTER_READ;
  sendbuf[4] = 0x20;
  sendbuf[5] = 7;
//  while(lcd_busy);
  UART4ToPC(sendbuf,6);
}

void Sdwe_clear_filename(u8 file_count)
{
  Sdwe_clearString(PAGE_HISTORY_TEXT_FILENAME1 + file_count * 20);
  Sdwe_clearString(PAGE_HISTORY_TEXT_FILENAME1 + file_count * 20 + 10);
}

void Sdwe_refresh_filename(FILE_INFO file,u8 file_count)
{
  u8 time_buf[30];
  u8 len;
  u8 name[20],name_1[20];
  u8 name_len;
  u8 id[10];
  memset(id,0,10);
  memset(time_buf,0,30);
  len = sizeof(FILE_INFO);
  W25QXX_Read((u8 *)&file,(u32)(W25QXX_ADDR_JINGSHA + FILE_SIZE * file_count),len);
  if(file.filename_len <= 10)
  {
    memset(name,0,20);
    memset(name_1,0,20);
    memcpy(name_1,file.filename,file.filename_len);
//    sprintf((char *)name,"%d号机-%s",device_info.master_id,name_1);
    memcpy(id,device_info.device_id,device_info.device_id_len);
    sprintf((char *)name,"%s-%s.CSV",id,name_1);
    name_len = strlen((char const*)name);
    Sdwe_disString(PAGE_HISTORY_TEXT_FILENAME1 + file_count * 20,name,name_len);//显示文件名
    sprintf((char *)time_buf,"20%02d/%02d/%02d %02d:%02d:%02d",file.year,file.month,file.day,
            file.hour,file.minute,file.second);
    len = strlen((char const*)time_buf);
    Sdwe_disString(PAGE_HISTORY_TEXT_FILENAME1 + 10 + file_count * 20,time_buf,strlen((char const*)time_buf));//显示日期时间
  }
}
void Sdwe_refresh_allname(u8 file_count)
{
  u8 i;
  FILE_INFO file;
  for(i=0;i<10;i++)
    Sdwe_clear_filename(i);
  for(i=0;i<file_count;i++)
    Sdwe_refresh_filename(file,i);
}

//产量页面
void Sdwe_product_page(PRODUCT_PARA *para)
{
  u32 uncomplete;
  u32 complete;
  u8 on_time_buf[10];
  u8 off_time_buf[10];
  Sdwe_disDigi(PAGE_PRODUCT_A,(int)(para->product_a * 10),4);
  Sdwe_disDigi(PAGE_PRODUCT_B,(int)(para->product_b * 10),4);
  uncomplete = para->product_uncomplete * 10.0;
  complete = para->product_complete * 10.0;
  Sdwe_disDigi(PAGE_PRODUCT_UNCOMPLETE,uncomplete,4);
  Sdwe_disDigi(PAGE_PRODUCT_COMPLETE,complete,4);
  Sdwe_disDigi(PAGE_PRODUCT_KILOCOUNT,para->weicount_kilowei,4);
  Sdwe_disDigi(PAGE_PRODUCT_SPEED,para->speed,2);
  memset(on_time_buf,0,10);
  memset(off_time_buf,0,10);
  sprintf((char *)on_time_buf,"%04d:%02d",para->total_work_time / 3600,para->total_work_time % 3600 / 60);
  sprintf((char *)off_time_buf,"%04d:%02d",para->total_stop_time / 3600,para->total_stop_time % 3600 / 60);
  Sdwe_disString(PAGE_PRODUCT_TIME_ON,on_time_buf,strlen((char const*)on_time_buf));
  Sdwe_disString(PAGE_PRODUCT_TIME_OFF,off_time_buf,strlen((char const*)off_time_buf));
  Sdwe_disDigi(PAGE_PRODUCT_UNCOMPLETE_W,(int)(para->weight_uncomplete * 10),4);
  Sdwe_disDigi(PAGE_PRODUCT_COMPLETE_W,(int)(para->weight_complete * 10),4);
}

//胚料页面
void Sdwe_peiliao_page(PEILIAO_PARA *para)
{
  Sdwe_disDigi(PAGE_PRODUCT_JINGSHA,(int)(para->latitude_weight * 10),2);
  Sdwe_disDigi(PAGE_PRODUCT_WEISHA,(int)(para->longitude_weight * 10),2);
  Sdwe_disDigi(PAGE_PRODUCT_RUBBER,(int)(para->rubber_weight * 10),2);
  Sdwe_disDigi(PAGE_PRODUCT_FINAL,(int)(para->final_weight * 10),2);
  Sdwe_disDigi(PAGE_PRODUCT_ZHIJI,(int)(para->loom_num),2);
  Sdwe_disDigi(PAGE_PRODUCT_LOSS,(int)(para->loss),2);
  Sdwe_disDigi(PAGE_PRODUCT_TOTAL_METER,(int)(para->total_meter_set),4);
  Sdwe_disDigi(PAGE_PRODUCT_TOTAL_WEIGHT,(int)(para->total_weitht_set),4);
  Sdwe_disDigi(PAGE_PRODUCT_KAIDU,(int)(para->kaidu_set * 10),2);
  Sdwe_disDigi(PAGE_PRODUCT_WEIMI,(int)(para->weimi_set * 10),2);
  Sdwe_disDigi(PAGE_PRODUCT_WEISHU_DIS,(int)(para->weimi_dis_set),2);
}

void Sdwe_change_class_time(DEVICE_INFO *para)
{
  Sdwe_disDigi(PAGE_CHANGE_HOUR,(int)(para->class_para.class_time_hour),2);
  Sdwe_disDigi(PAGE_CHANGE_MINUTE,(int)(para->class_para.class_time_minute),2);
  Sdwe_writeIcon(PAGE_CHANGE_SWITCH,para->class_para.class_enable_onoff);
}

void Sdwe_period_page(DEVICE_INFO *para)
{
  u8 buf[10],i;
  Sdwe_writeIcon(PAGE_PERIOD_ENABLE,para->period_para.period_enable_onoff);
  Sdwe_disDigi(PAGE_PERIOD_YEAR,(int)(para->period_para.period_year),2);
  Sdwe_disDigi(PAGE_PERIOD_MONTH,(int)(para->period_para.period_month),2);
  Sdwe_disDigi(PAGE_PERIOD_DAY,(int)(para->period_para.period_day),2);
  for(i=0;i<para->period_para.period_password_len;i++)
    buf[i] = '*';
  Sdwe_disString(PAGE_PERIOD_PASSWORD_DIS,buf,para->period_para.period_password_len);
}

void Sdwe_hidden_page(DEVICE_INFO *para)
{
  Sdwe_writeIcon(PAGE_HIDDEN_JINGSHA,para->func_onoff.jingsha);
  Sdwe_writeIcon(PAGE_HIDDEN_CHANNENG,para->func_onoff.channeng);
  Sdwe_writeIcon(PAGE_HIDDEN_WEIMI,para->func_onoff.weimi);
}

void Sdwe_stop_page(DEVICE_INFO *para)
{
  u8 buf[20];
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[0] / 3600,para->stop_para.stop_time[0] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_WAIT_TRANSFER,buf,strlen((char const*)buf));
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[1] / 3600,para->stop_para.stop_time[1] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_TRANSFER,buf,strlen((char const*)buf));
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[2] / 3600,para->stop_para.stop_time[2] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_WAIT_PPC,buf,strlen((char const*)buf));
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[3] / 3600,para->stop_para.stop_time[3] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_WAIT_MATERIAL,buf,strlen((char const*)buf));
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[4] / 3600,para->stop_para.stop_time[4] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_NO_MATERIAL,buf,strlen((char const*)buf));
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[5] / 3600,para->stop_para.stop_time[5] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_CLEAN,buf,strlen((char const*)buf));
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[6] / 3600,para->stop_para.stop_time[6] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_TECH_ADJUST,buf,strlen((char const*)buf));
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[7] / 3600,para->stop_para.stop_time[7] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_DEVICE_ADJUST,buf,strlen((char const*)buf));
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[8] / 3600,para->stop_para.stop_time[8] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_REPAIR,buf,strlen((char const*)buf));
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[9] / 3600,para->stop_para.stop_time[9] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_WAIT_QAD,buf,strlen((char const*)buf));
  memset(buf,0,20);
  sprintf((char *)buf,"%04d:%02d",para->stop_para.stop_time[10] / 3600,para->stop_para.stop_time[10] % 3600 / 60);
  Sdwe_disString(PAGE_STOP_REPLACE_PAN,buf,strlen((char const*)buf));
}

void sdew_weimi_page1(WEIMI_PARA *para)
{
  Sdwe_disDigi(PAGE_WEIMI_TOTALWEI_1,para->total_wei_count[0],4);
  Sdwe_disDigi(PAGE_WEIMI_WEI_CM_1,(int)(para->wei_cm_set[0] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_WEI_INCH_1,(int)(para->wei_inch_set[0] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_REALWEI_1,para->real_wei_count[0],4);
  Sdwe_disDigi(PAGE_WEIMI_MEDIANWEI_1,para->total_wei_count[1],4);
  Sdwe_disDigi(PAGE_WEIMI_REAL_MEDIAN_1,para->real_wei_count[1],4);
  
  Sdwe_disDigi(PAGE_WEIMI_TOTALWEI_1 + 2,para->total_wei_count[2],4);
  Sdwe_disDigi(PAGE_WEIMI_WEI_CM_1 + 2,(int)(para->wei_cm_set[1] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_WEI_INCH_1 + 2,(int)(para->wei_inch_set[1] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_REALWEI_1 + 2,para->real_wei_count[2],4);
  Sdwe_disDigi(PAGE_WEIMI_MEDIANWEI_1 + 2,para->total_wei_count[3],4);
  Sdwe_disDigi(PAGE_WEIMI_REAL_MEDIAN_1 + 2,para->real_wei_count[3],4);
  
  Sdwe_disDigi(PAGE_WEIMI_TOTALWEI_1 + 4,para->total_wei_count[4],4);
  Sdwe_disDigi(PAGE_WEIMI_WEI_CM_1 + 4,(int)(para->wei_cm_set[2] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_WEI_INCH_1 + 4,(int)(para->wei_inch_set[2] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_REALWEI_1 + 4,para->real_wei_count[4],4);
  Sdwe_disDigi(PAGE_WEIMI_MEDIANWEI_1 + 4,para->total_wei_count[5],4);
  Sdwe_disDigi(PAGE_WEIMI_REAL_MEDIAN_1 + 4,para->real_wei_count[5],4);
  
  Sdwe_disDigi(PAGE_WEIMI_TOTALWEI_1 + 6,para->total_wei_count[6],4);
  Sdwe_disDigi(PAGE_WEIMI_WEI_CM_1 + 6,(int)(para->wei_cm_set[3] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_WEI_INCH_1 + 6,(int)(para->wei_inch_set[3] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_REALWEI_1 + 6,para->real_wei_count[6],4);
  Sdwe_disDigi(PAGE_WEIMI_MEDIANWEI_1 + 6,para->total_wei_count[7],4);
  Sdwe_disDigi(PAGE_WEIMI_REAL_MEDIAN_1 + 6,para->real_wei_count[7],4);
  
  Sdwe_disDigi(PAGE_WEIMI_TOTALWEI_1 + 8,para->total_wei_count[8],4);
  Sdwe_disDigi(PAGE_WEIMI_WEI_CM_1 + 8,(int)(para->wei_cm_set[4] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_WEI_INCH_1 + 8,(int)(para->wei_inch_set[4] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_REALWEI_1 + 8,para->real_wei_count[8],4);
  Sdwe_disDigi(PAGE_WEIMI_MEDIANWEI_1 + 8,para->total_wei_count[9],4);
  Sdwe_disDigi(PAGE_WEIMI_REAL_MEDIAN_1 + 8,para->real_wei_count[9],4);
}

void sdew_weimi_page2(WEIMI_PARA *para)
{
  Sdwe_disDigi(PAGE_WEIMI_TOTALWEI_1 + 10,para->total_wei_count[10],4);
  Sdwe_disDigi(PAGE_WEIMI_WEI_CM_1 + 10,(int)(para->wei_cm_set[5] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_WEI_INCH_1 + 10,(int)(para->wei_inch_set[5] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_REALWEI_1 + 10,para->real_wei_count[10],4);
  Sdwe_disDigi(PAGE_WEIMI_MEDIANWEI_1 + 10,para->total_wei_count[11],4);
  Sdwe_disDigi(PAGE_WEIMI_REAL_MEDIAN_1 + 10,para->real_wei_count[11],4);
  
  Sdwe_disDigi(PAGE_WEIMI_TOTALWEI_1 + 12,para->total_wei_count[12],4);
  Sdwe_disDigi(PAGE_WEIMI_WEI_CM_1 + 12,(int)(para->wei_cm_set[6] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_WEI_INCH_1 + 12,(int)(para->wei_inch_set[6] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_REALWEI_1 + 12,para->real_wei_count[12],4);
  Sdwe_disDigi(PAGE_WEIMI_MEDIANWEI_1 + 12,para->total_wei_count[13],4);
  Sdwe_disDigi(PAGE_WEIMI_REAL_MEDIAN_1 + 12,para->real_wei_count[13],4);
  
  Sdwe_disDigi(PAGE_WEIMI_TOTALWEI_1 + 14,para->total_wei_count[14],4);
  Sdwe_disDigi(PAGE_WEIMI_WEI_CM_1 + 14,(int)(para->wei_cm_set[7] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_WEI_INCH_1 + 14,(int)(para->wei_inch_set[7] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_REALWEI_1 + 14,para->real_wei_count[14],4);
  Sdwe_disDigi(PAGE_WEIMI_MEDIANWEI_1 + 14,para->total_wei_count[15],4);
  Sdwe_disDigi(PAGE_WEIMI_REAL_MEDIAN_1 + 14,para->real_wei_count[15],4);
  
  Sdwe_disDigi(PAGE_WEIMI_TOTALWEI_1 + 16,para->total_wei_count[16],4);
  Sdwe_disDigi(PAGE_WEIMI_WEI_CM_1 + 16,(int)(para->wei_cm_set[8] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_WEI_INCH_1 + 16,(int)(para->wei_inch_set[8] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_REALWEI_1 + 16,para->real_wei_count[16],4);
  Sdwe_disDigi(PAGE_WEIMI_MEDIANWEI_1 + 16,para->total_wei_count[17],4);
  Sdwe_disDigi(PAGE_WEIMI_REAL_MEDIAN_1 + 16,para->real_wei_count[17],4);
  
  Sdwe_disDigi(PAGE_WEIMI_TOTALWEI_1 + 18,para->total_wei_count[18],4);
  Sdwe_disDigi(PAGE_WEIMI_WEI_CM_1 + 18,(int)(para->wei_cm_set[9] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_WEI_INCH_1 + 18,(int)(para->wei_inch_set[9] * 10),2);
  Sdwe_disDigi(PAGE_WEIMI_REALWEI_1 + 18,para->real_wei_count[18],4);
  Sdwe_disDigi(PAGE_WEIMI_MEDIANWEI_1 + 18,para->total_wei_count[19],4);
  Sdwe_disDigi(PAGE_WEIMI_REAL_MEDIAN_1 + 18,para->real_wei_count[19],4);
}

void sdew_weisha_page1(WEIMI_PARA *para)
{
  Sdwe_disDigi(PAGE_WEIMI_STEP1_SPEED,para->step1_factor[0],2);
  Sdwe_disDigi(PAGE_WEIMI_STEP2_SPEED,para->step2_factor[0],2);
  
  Sdwe_disDigi(PAGE_WEIMI_STEP1_SPEED + 2,para->step1_factor[1],2);
  Sdwe_disDigi(PAGE_WEIMI_STEP2_SPEED + 2,para->step2_factor[1],2);

  Sdwe_disDigi(PAGE_WEIMI_STEP1_SPEED + 4,para->step1_factor[2],2);
  Sdwe_disDigi(PAGE_WEIMI_STEP2_SPEED + 4,para->step2_factor[2],2);
  
  Sdwe_disDigi(PAGE_WEIMI_STEP1_SPEED + 6,para->step1_factor[3],2);
  Sdwe_disDigi(PAGE_WEIMI_STEP2_SPEED + 6,para->step2_factor[3],2);
  
  Sdwe_disDigi(PAGE_WEIMI_STEP1_SPEED + 8,para->step1_factor[4],2);
  Sdwe_disDigi(PAGE_WEIMI_STEP2_SPEED + 8,para->step2_factor[4],2);
}

void sdew_weisha_page2(WEIMI_PARA *para)
{
  Sdwe_disDigi(PAGE_WEIMI_STEP1_SPEED + 10,para->step1_factor[5],2);
  Sdwe_disDigi(PAGE_WEIMI_STEP2_SPEED + 10,para->step2_factor[5],2);
  
  Sdwe_disDigi(PAGE_WEIMI_STEP1_SPEED + 12,para->step1_factor[6],2);
  Sdwe_disDigi(PAGE_WEIMI_STEP2_SPEED + 12,para->step2_factor[6],2);
  
  Sdwe_disDigi(PAGE_WEIMI_STEP1_SPEED + 14,para->step1_factor[7],2);
  Sdwe_disDigi(PAGE_WEIMI_STEP2_SPEED + 14,para->step2_factor[7],2);
  
  Sdwe_disDigi(PAGE_WEIMI_STEP1_SPEED + 16,para->step1_factor[8],2);
  Sdwe_disDigi(PAGE_WEIMI_STEP2_SPEED + 16,para->step2_factor[8],2);
  
  Sdwe_disDigi(PAGE_WEIMI_STEP1_SPEED + 18,para->step1_factor[9],2);
  Sdwe_disDigi(PAGE_WEIMI_STEP2_SPEED + 18,para->step2_factor[9],2);
}

u8 get_valid_length(u8 *buf,u8 len)
{
  u8 i,length;
  for(i=0;i<len;i++)
  {
    if(buf[i] == 0xff)
    {
      length = i;
      break;
    }
  }
  return length;
}

u8 array_compare(u8 *buf1,u8 *buf2,u8 len)
{
  u8 i,flag = 0;
  for(i=0;i<len;i++)
  {
    if(buf1[i] != buf2[i])
    {
      flag = 1;
      break;
    }
  }
  return flag;
}

u8 hex_to_decimal(u8 da)
{
  u8 tmp;
  tmp = (da / 16) * 10 + (da % 16);
  return tmp;
}
