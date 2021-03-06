#include "string.h"
#include "route.h"
#include "usart.h"
#include "usart2.h"
#include "delay.h"
#include "gps.h"
#include "lcd.h"
#include "led.h"
#include "malloc.h"
#include "mmc_sd.h"
#include "key.h"
#include "ff.h"
#include "exfuns.h"
#include "stdlib.h"

extern FIL fil;
extern FRESULT res;
extern UINT bww;
extern char buf[100];


nmea_msg gpsx;
route_msg route;

u8 dtbuf[50];   								//打印缓存器

u8 upload=0;

void route_init()
{
//    u8 *sd_data;
    u8 num=0;
    route.counter = 0;

    //在LCD上面，一个点表示10cm
    route.precise = 1.0;
    route.x_offset = 0;
    route.y_offset = 0;
    res=f_open (&fil,"0:/file_manage.txt", FA_OPEN_ALWAYS|FA_READ);

    if(res!=FR_OK)
    {
        printf("\r\nopen file_manage.txt fail .. \r\n");
        printf("open return code: %d\r\n",res);
    } else {
        printf("\r\nopen file_manage.txt success .. \r\n");

        printf("open return code: %d\r\n",res);

        f_read (&fil, buf,100,&bww);
        f_close(&fil);

        clear_file_name();
        clear_file_number();

        num= atoi(buf);//查看文件内记录了多少组数据
        sprintf((char *)route.sd_file_number,"%d",num+1);

        //删除文件后重新建立一个新文件，完成数据的更新工作
        //res=f_unlink("0:/file_manage.txt");

        res=f_open (&fil,"0:/file_manage.txt", FA_CREATE_ALWAYS|FA_WRITE);

        f_puts((char *)route.sd_file_number,&fil);

        f_close(&fil);


        clear_file_name();
        strcat((char *)route.sd_file_name,(char *)"0:/");
        strcat((char *)route.sd_file_name,(char *)route.sd_file_number);
        strcat((char *)route.sd_file_name,(char *)".txt");

        res=f_open (&fil,(char *)route.sd_file_name, FA_CREATE_ALWAYS|FA_WRITE);
        if(res!=FR_OK)
        {
            printf("\r\nopen %s.txt fail .. \r\n",route.sd_file_number);
            printf("open return code: %d\r\n",res);
        } else {
            printf("\r\nopen %s.txt success .. \r\n",route.sd_file_number);
            printf("open return code: %d\r\n",res);
            f_puts((char *)"0,0;",&fil);
            f_close(&fil);
        }

        //strcpy((char *)route.sd_file_number,buf);
		use_gpiob_io();
        LCD_ShowString(200,0,230,24,24,(u8 *)route.sd_file_number);
		use_gpiob_usart3();
		
		route.flag_get_origin_coordinate = FALSE;
		route.receive_gps_data = RECEIVE_GPS_DATA_FALSE;

		route.sd_file_record = SD_RECORD_ENABLE;
		route.sd_file_playback = PALYBACK_FALSE;
		route.draw_center = DRAW_CENTER_INCOMPLETE;
		route.sd_file_show_read_name_counter = 0;
		
    }
}

FRESULT scan_files (char* path,char *buff)
{
    static FILINFO fno; //文件信息结构体
    res = f_opendir(&dir, path);  // 打开文件目录
    if (res == FR_OK) {//如果打开成功循环读出文件名字到buff中
        strcat(buff, ",");
        for (;;) {  //循环读出文件名字，循环次数等于SD卡根目录下的文件数目
            res = f_readdir(&dir, &fno); //读取文件名
            if (res != FR_OK || fno.fname[0] == 0x00)
                break;  //读取错误或者读完所有文件结束就跳出循环
            if (fno.fattrib & AM_ARC && fno.fname[0] != 'F')//读取的是文件夹名字
            {
                route.sd_file_read_name_num ++;
                strcat(buff, fno.fname);	 //复制文件名字到缓存并打印文件名
                strcat(buff, ",");
                //printf("%s\r\n",fno.fname);
            }
        }
        f_closedir(&dir);//关闭文件目录
    }
    return res;// 返回
}

void show_route()
{
    u16 i,rxlen;
    float tp;
    u8 key;

    if(USART3_RX_STA&0X8000)		//接收到一次数据了
    {
        rxlen=USART3_RX_STA&0X7FFF;	//得到数据长度
        for(i=0; i<rxlen; i++)USART1_TX_BUF[i]=USART3_RX_BUF[i];
        USART3_RX_STA=0;		   	//启动下一次接收
        USART1_TX_BUF[i]=0;			//自动添加结束符
        GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//分析字符串
        if (route.flag_get_origin_coordinate == FALSE) {
            route.counter ++ ;
            if (route.counter < 10) {
                return;
            }
            route.flag_get_origin_coordinate = TRUE;
            tp=gpsx.longitude;
            route.x_init = tp / 100000;
            tp=gpsx.latitude;
            route.y_init = tp / 100000;
        }
        //获取当前经纬度
        tp=gpsx.longitude;//经度
        route.x_current = tp / 100000;
        tp=gpsx.latitude;//纬度
        route.y_current = tp / 100000;

        //经度每隔0.00001度，距离相差约0.55米
        //纬度每隔0.00001度，距离相差约1.1米
        route.x_offset_last = route.x_offset;
        route.y_offset_last = route.y_offset;
        route.x_offset = (route.x_current - route.x_init) * 100000.0 * 0.55;
        route.y_offset = (-route.y_current + route.y_init) * 100000.0 * 1.1;
		Gps_Msg_Show();				//显示信息
        route_lcd_show();
        route.receive_gps_data = RECEIVE_GPS_DATA_TURE;

    }
    //如果没有启用数据回放，那么就开始准备记录数据
    if(route.sd_file_playback == PALYBACK_FALSE)
    {
        key=KEY_Scan(0);
        if(key==KEY0_PRES)
        {
            //upload=!upload;
            //如果按下按键后，检测到正在记录数据，那么就停止记录
            if(route.sd_file_record == SD_RECORD_ENABLE)
            {
				//将正在记录的文件内添加终止标记符“#”
				route_data_record_end();
                route.sd_file_record = SD_RECORD_DISABLE;
                LED0 = 1;//LED灯熄灭
				//LCD显示操作提示
				use_gpiob_io();
				LCD_Clear(WHITE);
				POINT_COLOR=RED;//设置字体为红色
				LCD_ShowString(20,70,220,16,16,(u8*)"GPS Data Saved!");
				POINT_COLOR=BLUE;//设置字体为白色
				LCD_ShowString(20,90,220,16,16,(u8*)"Press KEY0 to Restart GPS");
				LCD_ShowString(20,110,220,16,16,(u8*)"Press KEY1 to Playback GPS");
				LCD_ShowString(20,130,220,16,16,(u8*)"Make Your Choice");
            } else {
                //如果按下按键后，检测到没有记录数据，那么就开始新一次记录数据
                LCD_Clear(WHITE);
				use_gpiob_usart3();
                route_init();
                route.sd_file_record = SD_RECORD_ENABLE;
            }
        }
		//如果数据记录被关闭，那么检测是否使能数据回放
		if(route.sd_file_record == SD_RECORD_DISABLE)
		{
			//在没有记录GPS数据的前提下
			if(key==KEY1_PRES)
			{
				//如果KEY1按下，说明要开始回放历史轨迹数据
				LCD_Clear(WHITE);
				use_gpiob_io();
				LCD_ShowString(20,90,220,16,16,(u8*)"Please Select the File That");
				LCD_ShowString(20,110,220,16,16,(u8*)"You Want to Playback");
				route.sd_file_playback = PALYBACK_TURE;
				route_data_playback();
				route.sd_file_playback = PALYBACK_FALSE;
			}
		}
    } else {
		//继续回放历史轨迹数据
		//route_data_playback();
	}
    
    if(route.sd_file_record == SD_RECORD_ENABLE && route.receive_gps_data == RECEIVE_GPS_DATA_TURE)
    {
        route.receive_gps_data = RECEIVE_GPS_DATA_FALSE;
        LED0=!LED0;
        route_data_record();
    }

}

void route_lcd_show()
{
    u16 x1,y1,x2,y2;
	u8 buf[15]={'\0'};
	u8 i = 0;

    x1 = LCD_MAX_LEN_X / 2 + route.x_offset;
    y1 = LCD_MAX_LEN_Y / 2 + route.y_offset;
    x2 = LCD_MAX_LEN_X / 2 + route.x_offset_last;
    y2 = LCD_MAX_LEN_Y / 2 + route.y_offset_last;

    //	LCD_DrawPoint(x1,y1);

	if(route.sd_file_playback != PALYBACK_TURE)
	{
		//若没有回放数据，则执行下面语句
		use_gpiob_io();
		sprintf((char *)buf,"x:%.0f",route.x_offset);	//得到x偏移值
		LCD_Fill(16,40,16+40,40+16,WHITE);//清除显示
		LCD_ShowString(0,40,50,16,16,(u8*)buf);
		for(i=0;i<sizeof(buf);i++)buf[i]='\0';

		if((int)route.y_offset == 0)
			sprintf((char *)buf," y:%.0f",route.y_offset);//得到y偏移值
		else
			sprintf((char *)buf," y:%.0f",-route.y_offset);//得到y偏移值
		LCD_Fill(116,40,116+40,40+16,WHITE);//清除显示
		LCD_ShowString(100,40,50,16,16,(u8*)buf);
		
	}
	
	if( (route.sd_file_record == SD_RECORD_DISABLE) && (route.sd_file_playback == PALYBACK_FALSE) )
	{
		//若没有开启记录数据，并且没有数据回放，则禁止绘图
		return;
	}
	
    LCD_DrawLine(x1,y1,x2,y2);
    LCD_Draw_Circle(x1,y1,1);
	
	POINT_COLOR=RED;//设置字体为红色
	LCD_Draw_Circle(LCD_MAX_LEN_X / 2,LCD_MAX_LEN_Y / 2,3);//绘制中心坐标
	POINT_COLOR=BLUE;//设置字体为蓝色

	if(route.sd_file_playback != PALYBACK_TURE)
	{
		use_gpiob_usart3();
	}
}

void Gps_Msg_Show(void)
{
    float tp;
    POINT_COLOR=BLUE;
	use_gpiob_io();
    tp=gpsx.longitude;
    sprintf((char *)dtbuf,"Longitude:%.6f %1c   ",tp/=100000,gpsx.ewhemi);	//得到经度字符串
    LCD_ShowString(0,0,200,16,16,dtbuf);
    tp=gpsx.latitude;
    sprintf((char *)dtbuf,"Latitude : %.6f %1c   ",tp/=100000,gpsx.nshemi);	//得到纬度字符串
    LCD_ShowString(0,20,200,16,16,dtbuf);
	use_gpiob_usart3();
}

void route_data_record_end()
{
	char gps_longtitude_start[12]={0};
	char gps_latitude_start[12]={0};
	char gps_longtitude_end[12]={0};
	char gps_latitude_end[12]={0};
	char gps_data[55]={0};
	//将正在记录的文件内添加终止标记符“#”
	clear_file_name();

    strcat((char *)route.sd_file_name,(char *)"0:/");
    strcat((char *)route.sd_file_name,(char *)route.sd_file_number);
    strcat((char *)route.sd_file_name,(char *)".txt");

    res=f_open (&fil,(char *)route.sd_file_name, FA_OPEN_ALWAYS|FA_WRITE);
    if(res!=FR_OK) 
    {
        printf("\r\nopen %s.txt fail .. \r\n",route.sd_file_number);
        printf("open return code: %d\r\n",res);
    } else {
        printf("\r\nopen %s.txt success .. \r\n",route.sd_file_number);
        printf("open return code: %d\r\n",res);
        res = f_lseek(&fil,f_size(&fil));//找到文件数据末尾
        printf("f_lseek return code: %d\r\n",res);
		
        sprintf(gps_longtitude_start,"%.6f",route.x_init);
		sprintf(gps_longtitude_end,"%.6f",route.y_init);
		sprintf(gps_latitude_start,"%.6f",route.x_current);
		sprintf(gps_latitude_end,"%.6f",route.y_current);
		strcat(gps_data,"#");
		strcat(gps_data,gps_longtitude_start);
		strcat(gps_data,",");
		strcat(gps_data,gps_longtitude_end);
		strcat(gps_data,"#");
		strcat(gps_data,gps_latitude_start);
		strcat(gps_data,",");
		strcat(gps_data,gps_latitude_end);
		strcat(gps_data,"#");
		
		f_puts(gps_data,&fil);
        f_close(&fil);
    }
}

void route_data_record() {
    //	u8 n;
    int tmp;
    u8 x_char[10],y_char[10],dat[30];

    if( (int)route.x_offset == (int)route.x_offset_last && (int)route.y_offset == (int)route.y_offset_last)
    {
        //数据重合，说明没有移动，不记录数据
        return;
    }

    memset(x_char,'\0',10);
    memset(y_char,'\0',10);
    memset(dat,'\0',30);

    sprintf((char *)x_char,"%.0f",route.x_offset);
    sprintf((char *)y_char,"%.0f",route.y_offset);

    strcat((char *)dat,(char *)x_char);
    strcat((char *)dat,(char *)",");
    strcat((char *)dat,(char *)y_char);
    strcat((char *)dat,(char *)";");
    printf("\r\n%s\r\n",dat);

    clear_file_name();

    strcat((char *)route.sd_file_name,(char *)"0:/");
    strcat((char *)route.sd_file_name,(char *)route.sd_file_number);
    strcat((char *)route.sd_file_name,(char *)".txt");

    res=f_open (&fil,(char *)route.sd_file_name, FA_OPEN_ALWAYS|FA_WRITE);
    if(res!=FR_OK)
    {
        printf("\r\nopen %s.txt fail .. \r\n",route.sd_file_number);
        printf("open return code: %d\r\n",res);
    } else {
        printf("\r\nopen %s.txt success .. \r\n",route.sd_file_number);
        printf("open return code: %d\r\n",res);
        res = f_lseek(&fil,f_size(&fil));
        printf("f_lseek return code: %d\r\n",res);
        tmp = f_puts((char *)dat,&fil);
        printf("f_puts return code: %d\r\n",tmp);
        f_close(&fil);
    }

}

void route_data_playback()
{
	use_gpiob_io();//设为通用IO口，LCD显示用
	memset(route.sd_file_read_name,0,sizeof(route.sd_file_read_name));
	route.sd_file_read_name_num = 0;
	scan_files("",route.sd_file_read_name);
	printf("\r\n%s\r\n",route.sd_file_read_name);
	//获取到所有的历史数据
	route.sd_file_show_read_name_counter = 0;
	//f_readdir("0:\",route.sd_file_read_name);
	//此处填写选择需要回放的文件名
	show_history_file();
	//此处编写读取待回放文件内的数据
	show_history_route();
	use_gpiob_usart3();//设为复用IO口，串口3用
}

void show_history_file()
{
    u8 i=0;
    u8 key;
    
    //解析route.sd_file_read_name内部数据
    //,1.txt,2.txt,3.txt,4.txt,

    //获取到第一个逗号
    route.sd_file_strx1 = strstr((const char*)route.sd_file_read_name,(const char*)",");
    for(i=0; i<sizeof(route.sd_file_show_read_name); i++)route.sd_file_show_read_name[i]='\0';
    for(i=0;; i++)
    {
        if(route.sd_file_strx1[i+1] == ',')
            break;
        route.sd_file_show_read_name[i]=route.sd_file_strx1[i+1];
    }
    route.sd_file_show_read_name_counter ++;
    LCD_ShowString(20,200,200,24,24,(u8*)route.sd_file_show_read_name);

    while(1) {

        key=KEY_Scan(0);
        if(key==KEY0_PRES)
        {
            //读取下一个文件
            route.sd_file_show_read_name_counter ++;
            if(route.sd_file_show_read_name_counter > route.sd_file_read_name_num)
            {
                LCD_Fill(20,200,240,200+24,WHITE);
                LCD_ShowString(20,200,200,24,24,(u8*)"Length Upflow!");
				delay_ms(1000);
				LCD_Fill(20,200,240,200+24,WHITE);
				LCD_ShowString(20,200,200,24,24,(u8*)route.sd_file_show_read_name);
                route.sd_file_show_read_name_counter = route.sd_file_read_name_num + 1;
                continue;
            }
            route.sd_file_strx1 = strstr((const char*)(route.sd_file_strx1+1),(const char*)",");
            for(i=0; i<sizeof(route.sd_file_show_read_name); i++)route.sd_file_show_read_name[i]='\0';
            for(i=0;; i++)
            {
                if(route.sd_file_strx1[i+1] == ',')
                    break;
                route.sd_file_show_read_name[i]=route.sd_file_strx1[i+1];
            }
            LCD_Fill(20,130,240,200+24,WHITE);
            LCD_ShowString(20,200,200,24,24,(u8*)route.sd_file_show_read_name);
        }
        else if(key==KEY1_PRES)
        {
            //读取上一个文件
            route.sd_file_show_read_name_counter --;
            if(route.sd_file_show_read_name_counter <= 0)
            {
                LCD_Fill(20,200,240,130+24,WHITE);
                LCD_ShowString(20,200,200,24,24,(u8*)"Length Overflow!");
				delay_ms(1000);
				LCD_Fill(20,130,240,200+24,WHITE);
				LCD_ShowString(20,200,200,24,24,(u8*)route.sd_file_show_read_name);
                route.sd_file_show_read_name_counter = 1;
                continue;
            }

            while(1)
            {
                --route.sd_file_strx1;
                if(route.sd_file_strx1[0] == ',')
                    break;
            }

            //route.sd_file_strx1 = strstr((const char*)(route.sd_file_strx1-1),(const char*)",");
            for(i=0; i<sizeof(route.sd_file_show_read_name); i++)route.sd_file_show_read_name[i]='\0';
            for(i=0;; i++)
            {
                if(route.sd_file_strx1[i+1] == ',')
                    break;
                route.sd_file_show_read_name[i]=route.sd_file_strx1[i+1];
            }
            LCD_Fill(20,200,240,200+24,WHITE);
            LCD_ShowString(20,200,200,24,24,(u8*)route.sd_file_show_read_name);
        } else if(key==WKUP_PRES)
        {
//            LCD_Clear(WHITE);
//            LCD_ShowString(20,200,200,24,24,(u8*)"success");
            break;
        }
    }
    
}


void show_history_route()
{
    u32 data=0;
    u8 i=0;
    char *sd_data;
	char gps_data_char[15];

    clear_file_name();
    strcat((char *)route.sd_file_name,(char *)"0:/");
    strcat((char *)route.sd_file_name,(char *)route.sd_file_show_read_name);

    res=f_open (&fil,(char *)route.sd_file_name, FA_OPEN_ALWAYS|FA_READ);
    if(res!=FR_OK)
    {
        printf("\r\nopen %s fail .. \r\n",route.sd_file_show_read_name);
        printf("open return code: %d\r\n",res);
    } else {
        printf("\r\nopen %s success .. \r\n",route.sd_file_show_read_name);
        printf("open return code: %d\r\n",res);
        data = f_size(&fil);
        sd_data = mymalloc(data+2);
        f_gets(sd_data,data+1,&fil);
        printf("---\r\n%s\r\n---\r\n",sd_data);
        f_close(&fil);

        //0,0;2,-3;2,-3;3,-3;5,-5;10,-8;#
        route.x_offset = 0;
        route.y_offset = 0;//初始值
        route.sd_file_strx2 = strstr((const char*)sd_data,",");
		
		LCD_Clear(WHITE);
		LCD_ShowString(100,300,50,16,16,(u8*)route.sd_file_show_read_name);

        while(1)
        {
            route.x_offset_last = route.x_offset;
            route.y_offset_last = route.y_offset;

            for(i=0; i<sizeof(route.x_offset_char); i++) {
                route.x_offset_char[i]='\0';
                route.y_offset_char[i]='\0';
            }
            //转换x
            route.sd_file_strx2 = strstr((const char*)(route.sd_file_strx2 + 1),";");
			
			//检测分号“;”后面是否为“#”
			if(route.sd_file_strx2[1] == '#')
			{
				//准备解析起始点GPS和终点GPS
				//#12323456,30123456#123123456,30123456#
				//0,0;1,1;#113.123486,30.123466#113.123446,30.123458#
				route.sd_file_strx2 = strstr((const char*)(route.sd_file_strx2 ),"#");
				for(i=0;i<15;i++)gps_data_char[i]=0;
				for(i=0;; i++)
				{
					if(route.sd_file_strx2[i+1] == ',')
						break;
					gps_data_char[i]=route.sd_file_strx2[i+1];
				}
				//提取出来起始点经度
//				gps_data = atoi(gps_data_char);
//				sprintf(gps_data_char,"%.6f",(float)(gps_data / 1000000));
				LCD_ShowString(0,210,10,16,12,(u8*)"E:");
				LCD_ShowString(10,210,70,16,12,(u8*)gps_data_char);
				
				for(i=0;i<15;i++)gps_data_char[i]=0;
				route.sd_file_strx2 = strstr((const char*)(route.sd_file_strx2 + 1),",");
				for(i=0;; i++)
				{
					if(route.sd_file_strx2[i+1] == '#')
						break;
					gps_data_char[i]=route.sd_file_strx2[i+1];
				}
				//提取出来起始点纬度
//				gps_data = atoi(gps_data_char);
//				sprintf(gps_data_char,"%.6f",(float)(gps_data / 1000000));
				LCD_ShowString(100,210,10,16,12,(u8*)"N:");
				LCD_ShowString(110,210,70,16,12,(u8*)gps_data_char);
				
				//终点
				for(i=0;i<15;i++)gps_data_char[i]=0;
				route.sd_file_strx2 = strstr((const char*)(route.sd_file_strx2 ),"#");
				for(i=0;; i++)
				{
					if(route.sd_file_strx2[i+1] == ',')
						break;
					gps_data_char[i]=route.sd_file_strx2[i+1];
				}
				//提取出来终点经度
//				gps_data = atoi(gps_data_char);
//				sprintf(gps_data_char,"%.6f",(float)(gps_data / 1000000));
				LCD_ShowString(0,230,10,16,12,(u8*)"E:");
				LCD_ShowString(10,230,70,16,12,(u8*)gps_data_char);
				
				for(i=0;i<15;i++)gps_data_char[i]=0;
				route.sd_file_strx2 = strstr((const char*)(route.sd_file_strx2 + 1),",");
				for(i=0;; i++)
				{
					if(route.sd_file_strx2[i+1] == '#')
						break;
					gps_data_char[i]=route.sd_file_strx2[i+1];
				}
				//提取出来终点纬度
//				gps_data = atoi(gps_data_char);
//				sprintf(gps_data_char,"%.6f",(float)(gps_data / 1000000));
				LCD_ShowString(100,230,10,16,12,(u8*)"N:");
				LCD_ShowString(110,230,70,16,12,(u8*)gps_data_char);
				
				//解析数据完毕
				LCD_ShowString(20,250,220,16,12,(u8*)"Press KEY0 to Restart GPS");
				LCD_ShowString(20,265,220,16,12,(u8*)"Press KEY1 to Playback GPS");
				LCD_ShowString(20,280,220,16,12,(u8*)"Make Your Choice");
				break;
			}
			
            for(i=0;; i++)
            {
                if(route.sd_file_strx1[i+1] == ',')
                    break;
                route.x_offset_char[i]=route.sd_file_strx2[i+1];
            }
            route.x_offset = atoi(route.x_offset_char);//转换为int型
            //转换y
            route.sd_file_strx2 = strstr((const char*)(route.sd_file_strx2 + 1),",");
            for(i=0;; i++)
            {
                if(route.sd_file_strx2[i+1] == ';')
                    break;
                route.y_offset_char[i]=route.sd_file_strx2[i+1];
            }
            route.y_offset = atoi(route.y_offset_char);//转换为int型
            /*
                x1 = LCD_MAX_LEN_X / 2 + route.x_offset;
            	y1 = LCD_MAX_LEN_Y / 2 + route.y_offset;
            	x2 = LCD_MAX_LEN_X / 2 + route.x_offset_last;
            	y2 = LCD_MAX_LEN_Y / 2 + route.y_offset_last;

            */
            route_lcd_show();
        }

        myfree(sd_data);
    }
}

void clear_file_name()
{
    u8 i = 0;
    for(i=0; i<sizeof(route.sd_file_name); i++)
    {
        route.sd_file_name[i] = '\0';
    }
}

void clear_file_number()
{
    u8 i = 0;
    for(i=0; i<sizeof(route.sd_file_number); i++)
    {
        route.sd_file_number[i] = '\0';
    }
}
