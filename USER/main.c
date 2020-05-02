#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "lcd.h"
#include "key.h"
//#include "usmart.h"
#include "usart2.h"
#include "malloc.h"
#include "MMC_SD.h"
#include "ff.h"
#include "exfuns.h"
#include "route.h"
#include "gps.h"
#include "string.h"
//���� ALIENTEK Mini STM32�����巶������29 �޸�
//FATFS ʵ��
//����֧�֣�www.openedv.com
//������������ӿƼ����޹�˾

/*******��������*****/
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
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);// �����ж����ȼ�����2
    delay_init();	    	 //��ʱ������ʼ��
    uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
    USART3_Init(38400);  //��ʼ������3������Ϊ115200
    exfuns_init();		//Ϊfatfs��ر��������ڴ�
    LCD_Init();			//��ʼ��Һ��
    LED_Init();         //LED��ʼ��
    KEY_Init();
    //usmart_dev.init(72);
    mem_init();			//��ʼ���ڴ��

    POINT_COLOR=RED;//��������Ϊ��ɫ
    use_gpiob_io();
    LCD_ShowString(60,50,200,16,16,"Mini STM32");
    LCD_ShowString(60,70,200,16,16,"This Program is");
    LCD_ShowString(60,90,200,16,16,"Developed By MLTBEANS");
    LCD_ShowString(60,110,200,16,16,"2020/04/28");

    while(SD_Initialize())					//���SD��
    {
        LCD_ShowString(60,150,200,16,16,"SD Card Error!");
        delay_ms(200);
        LCD_Fill(60,150,240,150+16,WHITE);//�����ʾ
        delay_ms(200);
        LED0=!LED0;//DS0��˸
    }
    exfuns_init();							//Ϊfatfs��ر��������ڴ�
    f_mount(fs[0],"0:",1); 					//����SD��
    //f_mount(fs[1],"1:",1); 					//����FLASH.
    while(exf_getfree("0",&total,&free))	//�õ�SD������������ʣ������
    {
        LCD_ShowString(60,150,200,16,16,"Fatfs Error!");
        delay_ms(200);
        LCD_Fill(60,150,240,150+16,WHITE);//�����ʾ
        delay_ms(200);
        LED0=!LED0;//DS0��˸
    }
    POINT_COLOR=BLUE;//��������Ϊ��ɫ
    LCD_ShowString(60,150,200,16,16,"FATFS OK!");
    LCD_ShowString(60,170,200,16,16,"SD Total Size:     MB");
    LCD_ShowString(60,190,200,16,16,"SD  Free Size:     MB");
    LCD_ShowNum(172,170,total>>10,5,16);					//��ʾSD�������� MB
    LCD_ShowNum(172,190,free>>10,5,16);						//��ʾSD��ʣ������ MB

    /********************start*************************/

    /********************end***************************/

	LCD_ShowString(30,220,200,16,16,"S1216F8-BD Setting...");
    use_gpiob_usart3();

    if(SkyTra_Cfg_Rate(GPS_HZ)!=0)	//���ö�λ��Ϣ�����ٶ�Ϊ5Hz,˳���ж�GPSģ���Ƿ���λ.
    {
		//https://www.cnblogs.com/88223100/p/GPRM_GNRMC_Transform.html
        //LCD_ShowString(30,220,200,16,16,"S1216F8-BD Setting...");
        do
        {
            USART3_Init(9600);			//��ʼ������3������Ϊ9600
            SkyTra_Cfg_Prt(3);			//��������ģ��Ĳ�����Ϊ38400
            USART3_Init(38400);			//��ʼ������3������Ϊ38400
            key=SkyTra_Cfg_Tp(100000);	//������Ϊ100ms
        } while(SkyTra_Cfg_Rate(GPS_HZ)!=0&&key!=0);//����SkyTraF8-BD�ĸ�������Ϊ5Hz
        
    }
	use_gpiob_io();
	LCD_ShowString(30,220,200,16,16,"S1216F8-BD Set Done!!");	
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
    LCD_Clear(WHITE);
	
	use_gpiob_usart3();
    route_init();

    while(1)
    {
        show_route();
    }
}

