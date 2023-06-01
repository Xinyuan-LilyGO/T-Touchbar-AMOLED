#ifndef _TIME_DATE_H
#define _TIME_DATE_H

#include <Arduino.h>



#ifdef __cplusplus       //-----------标准写法-----------
extern "C"{                      //-----------标准写法-----------
#endif                               //-----------标准写法-----------

void time_update(void); 
char get_week(int day, int month, int year);	 	 
void date_time_init_fun(void);

typedef struct  		
{
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int min;
	unsigned int sec;
	unsigned int week;
}date_time_t;  

#ifdef __cplusplus      //-----------标准写法-----------
}                                          //-----------标准写法-----------
#endif                              //-----------标准写法-----------


#endif //_TIME_DATE_H

