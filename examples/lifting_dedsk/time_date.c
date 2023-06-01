#include "time_date.h"

char time_1s = 0;                           

date_time_t date_time = {0};     

uint8_t  qz_days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char get_week(int day, int month, int year)  
{
	int w = 0;
  
    if(month <= 2)
    {
        month += 12; 
        year--;
    }
	
    w = ((day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400 + 1) % 7 + 7) % 7;
		return w;
}

void date_time_init_fun(void) 
{
	date_time.sec  = 00; 
	date_time.min  = 59;
	date_time.hour = 23;
	date_time.day  = 31;
	date_time.month = 5;	
	date_time.year = 2023;
	date_time.week = get_week(date_time.day, date_time.month, date_time.year);	
}

void time_update(void) 	
{
    if(time_1s) 
    {
        time_1s = 0;
        date_time.sec++;			
        if(date_time.sec == 60)
        {
            date_time.sec = 0;
            date_time.min++;

            if(date_time.min == 60) 
            {
                date_time.min = 0;
                date_time.hour++;
                if(date_time.hour == 24) 
                {
                    date_time.hour = 0;
                    date_time.day++;

                    if(date_time.month == 2) 
                    {
                        if(((date_time.year)%400 == 0) || (((date_time.year)%100 != 0)&&date_time.year%4==0))
                        {
                            if(date_time.day == 30)
                            {
                                date_time.day = 1;
                                date_time.month++;
                            }
                        }
                        else if(date_time.day == 29)
                        {
                            date_time.day = 1;
                            date_time.month++;
                        }
                    }
                    else if(date_time.day == (qz_days[date_time.month]+1)) 
                    {
                        date_time.day = 1;
                        date_time.month++;
                        if(date_time.month == 13) 
                        {
                            date_time.month = 1;
                            date_time.year++;
                        }
                    }
        date_time.week = get_week(date_time.day, date_time.month, date_time.year);	
                }
            }
        }
    }   //if(time_1s) end

             
}

