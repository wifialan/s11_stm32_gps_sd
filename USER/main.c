#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "lcd.h"
#include "key.h"
#include "usart2.h"
#include "malloc.h"
#include "MMC_SD.h"
#include "ff.h"
#include "exfuns.h"
#include "route.h"
#include "gps.h"
#include "string.h"


/*******变量定义*****/
FIL fil;
FRESULT res;
UINT bww;
char buf[100];
extern route_msg route;

#define GPS_HZ	5

int main(void)
{

	u8 key;
    u32 total,free;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);// 设置中断优先级分组2
    delay_init();	    	 	//延时函数初始化
    uart_init(115200);	 		//串口初始化为115200
    USART3_Init(38400);  		//初始化串口3波特率为38400-GPS
    exfuns_init();				//为fatfs相关变量申请内存-SD
    LCD_Init();					//初始化液晶
    LED_Init();         		//LED初始化
    KEY_Init();
    mem_init();			//初始化内存池

    POINT_COLOR=RED;//设置字体为红色
    use_gpiob_io();
    LCD_ShowString(60,50,200,16,16,"Mini STM32");

    while(SD_Initialize())					//检测SD卡
    {
        LCD_ShowString(60,150,200,16,16,"SD Card Error!");
        delay_ms(200);
        LCD_Fill(60,150,240,150+16,WHITE);//清除显示
        delay_ms(200);
        LED0=!LED0;//DS0闪烁
    }
    exfuns_init();							//为fatfs相关变量申请内存
    f_mount(fs[0],"0:",1); 					//挂载SD卡
    while(exf_getfree("0",&total,&free))	//得到SD卡的总容量和剩余容量
    {
        LCD_ShowString(60,150,200,16,16,"Fatfs Error!");
        delay_ms(200);
        LCD_Fill(60,150,240,150+16,WHITE);//清除显示
        delay_ms(200);
        LED0=!LED0;//DS0闪烁
    }
    POINT_COLOR=BLUE;//设置字体为蓝色
    LCD_ShowString(60,150,200,16,16,"FATFS OK!");
    LCD_ShowString(60,170,200,16,16,"SD Total Size:     MB");
    LCD_ShowString(60,190,200,16,16,"SD  Free Size:     MB");
    LCD_ShowNum(172,170,total>>10,5,16);					//显示SD卡总容量 MB
    LCD_ShowNum(172,190,free>>10,5,16);						//显示SD卡剩余容量 MB

    /********************start*************************/

    /********************end***************************/

	LCD_ShowString(30,220,200,16,16,"S1216F8-BD Setting...");
    use_gpiob_usart3();

    if(SkyTra_Cfg_Rate(GPS_HZ)!=0)	//设置定位信息更新速度为5Hz,顺便判断GPS模块是否在位.
    {
		//https://www.cnblogs.com/88223100/p/GPRM_GNRMC_Transform.html
        //LCD_ShowString(30,220,200,16,16,"S1216F8-BD Setting...");
        do
        {
            USART3_Init(9600);			//初始化串口3波特率为9600
            SkyTra_Cfg_Prt(3);			//重新设置模块的波特率为38400
            USART3_Init(38400);			//初始化串口3波特率为38400
            key=SkyTra_Cfg_Tp(100000);	//脉冲宽度为100ms
        } while(SkyTra_Cfg_Rate(GPS_HZ)!=0&&key!=0);//配置SkyTraF8-BD的更新速率为5Hz
        
    }
	use_gpiob_io();
	LCD_ShowString(30,220,200,16,16,"S1216F8-BD Set Done!!");	
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
    LCD_Clear(WHITE);
	
	LCD_ShowString(20,90,220,16,16,(u8*)"Press KEY0 to start");
	LCD_ShowString(20,110,220,16,16,(u8*)"Press KEY1 to playback");
	
	
	//在这里选择是否要回放数据
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY0_PRES)
		{
			//KEY0按下，开始记录数据
			LCD_Clear(WHITE);
			break;
		} else if(key==KEY1_PRES)
		{
			//KEY1按下，开始回放数据
			LCD_Clear(WHITE);
			LCD_ShowString(20,90,220,16,16,(u8*)"Please Select the File That");
			LCD_ShowString(20,110,220,16,16,(u8*)"You Want to Playback");
			route.sd_file_playback = PALYBACK_TURE;
			route_data_playback();
		}
	}
	
	use_gpiob_usart3();
    route_init();

    while(1)
    {
        show_route();
    }
}


