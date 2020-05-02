/*
 * route.h
 *
 *  Created on: 2020年4月20日
 *      Author: multimicro
 */

#ifndef ROUTE_H_
#define ROUTE_H_

#include "sys.h"

__packed typedef struct
{
#define		TRUE			1
#define		FALSE			0
#define		LCD_MAX_LEN_X	240
#define		LCD_MAX_LEN_Y	320
#define		SD_RECORD_ENABLE	1
#define		SD_RECORD_DISABLE	0
#define		RECEIVE_GPS_DATA_TURE	1
#define		RECEIVE_GPS_DATA_FALSE	0
#define		PALYBACK_TURE	1
#define		PALYBACK_FALSE	0
#define		SHOW_READ_NAME_FIRST_TURE	1
#define		SHOW_READ_NAME_FIRST_FALSE	0
#define 	DRAW_CENTER_COMPLETE	1
#define 	DRAW_CENTER_INCOMPLETE	0


    float x_init;					//初始化x轴坐标
    float y_init;					//初始化y轴坐标
    u8 flag_get_origin_coordinate;
    u16 counter;

    float x_current;				//当前x轴坐标
    float y_current;				//当前y轴坐标
    float x_offset;					//当前x轴坐标相对中心坐标的偏移量
    float y_offset;					//当前y轴坐标相对中心坐标的偏移量
    float x_offset_last;			//上一次x轴坐标相对中心坐标的偏移量
    float y_offset_last;			//上一次y轴坐标相对中心坐标的偏移量
    float precise;					//轨迹在LCD屏幕上的显示精度
    u8	receive_gps_data;
    char x_offset_char[4];
    char y_offset_char[4];
	u8 draw_center;

    u8	sd_file_first_read_flag;
    u8 sd_file_number[5];
    u8 sd_file_name[10];
    u8 sd_file_record;
    u8 sd_file_playback;
    char sd_file_read_name[300];
    u16 sd_file_read_name_num;
    char *sd_file_strx1;
    char *sd_file_strx2;
    char sd_file_show_read_name[10];
    int sd_file_show_read_name_counter;

}route_msg;

void clear_file_name(void);
void clear_file_number(void);
void Gps_Msg_Show(void);
void show_route(void);
void route_lcd_show(void);
void route_init(void);
void route_data_record(void);
void route_data_playback(void);
void show_history_file(void);
void show_history_route(void);
void route_data_record_end(void);

#endif /* ROUTE_H_ */
