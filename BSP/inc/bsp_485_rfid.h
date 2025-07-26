#ifndef __485_RFID_H
#define	__485_RFID_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>  // ?????


extern uint8_t rx_data_rfid[1];

//// 不精确的延时
//static void _485_delay(__IO uint32_t nCount)
//{
//	for(; nCount != 0; nCount--);
//} 




/*debug*/

#define _485_DEBUG_ON         1
#define _485_DEBUG_ARRAY_ON   1
#define _485_DEBUG_FUNC_ON    1
   
   
// Log define
#define _485_INFO(fmt,arg...)           printf("<<-_485-INFO->> "fmt"\n",##arg)
#define _485_ERROR(fmt,arg...)          printf("<<-_485-ERROR->> "fmt"\n",##arg)
#define _485_DEBUG(fmt,arg...)          do{\
																					 if(_485_DEBUG_ON)\
																					 printf("<<-_485-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
																				 }while(0)

#define _485_DEBUG_ARRAY(array, num)    do{\
                                         int32_t i;\
                                         uint8_t* a = array;\
                                         if(_485_DEBUG_ARRAY_ON)\
                                         {\
                                            printf("<<-_485-DEBUG-ARRAY->>\n");\
                                            for (i = 0; i < (num); i++)\
                                            {\
                                                printf("%02x   ", (a)[i]);\
                                                if ((i + 1 ) %10 == 0)\
                                                {\
                                                    printf("\n");\
                                                }\
                                            }\
                                            printf("\n");\
                                        }\
                                       }while(0)

#define _485_DEBUG_FUNC()               do{\
                                         if(_485_DEBUG_FUNC_ON)\
                                         printf("<<-_485-FUNC->> Func:%s@Line:%d\n",__func__,__LINE__);\
                                       }while(0)


void BSP_RFID_Config(void);

char *get_rfid_rebuff(uint16_t *len);
void clean_rfid_rebuff(void);
void RFID_DMA_Rx_ReStart(void);         //++++++++++++++++++++
uint8_t get_rfid_rx_complete(void);  //+++++++++++++++++++++++++
																			 
void RFID_SendStr(uint8_t *str, uint32_t strlen);


void uart8_tx_cplt(void);
#endif /* __485_H */
