
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"

#include "user_linkled.h"

#include "user_webserver.h"
#ifndef OS_ENTER_CRITICAL
#define OS_ENTER_CRITICAL()
#endif 

#ifndef OS_EXIT_CRITICAL
#define OS_EXIT_CRITICAL()
#endif

#ifndef NOT_OK
#define NOT_OK              0xff                        /* 参数错误                                     */
#endif

#define QUEUE_FULL          8                           /* 队列满                                       */
#define QUEUE_EMPTY         4                           /* 无数据                                       */
#define QUEUE_OK            1                           /* 操作成功                                     */

#define Q_WRITE_MODE        1                           /* 操作成功                                     */
#define Q_WRITE_FRONT_MODE  2                           /* 操作成功                                     */

#ifndef QUEUE_DATA_TYPE
#define QUEUE_DATA_TYPE     uint8
#endif

typedef struct {
    QUEUE_DATA_TYPE     *Out;                   /* 指向数据输出位置         */
    QUEUE_DATA_TYPE     *In;                    /* 指向数据输入位置         */
    QUEUE_DATA_TYPE     *End;                   /* 指向Buf的结束位置        */
    uint16              NData;                  /* 队列中数据个数           */
    uint16              MaxData;                /* 队列中允许存储的数据个数 */
    
    uint8               (* ReadEmpty)();        /* 读空处理函数             */
    uint8               (* WriteFull)();        /* 写满处理函数             */
    QUEUE_DATA_TYPE     Buf[1];                 /* 存储数据的空间           */
} DataQueue;

LOCAL uint8 ReSendBuff[3096];
LOCAL uint8 buff_isBusy_flag = 0;


/*********************************************************************************************************
** 函数名称: QueueCreate
** 功能描述: 初始化数据队列
** 输　入: Buf      ：为队列分配的存储空间地址
**         SizeOfBuf：为队列分配的存储空间大小（字节）
**         ReadEmpty：为队列读空时处理程序
**         WriteFull：为队列写满时处理程序
** 输　出: NOT_OK:参数错误
**         QUEUE_OK:成功
** 全局变量: 无
** 调用模块: OS_ENTER_CRITICAL,OS_EXIT_CRITICAL
**
** 作　者: 陈明计
** 日　期: 2003年7月2日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
LOCAL ICACHE_FLASH_ATTR uint8 QueueCreate(void *Buf,
                  uint32 SizeOfBuf,
				  uint8 (* ReadEmpty)(),
				  uint8 (* WriteFull)()
                          )
{
    DataQueue *Queue;
    
    if (Buf != NULL && SizeOfBuf >= (sizeof(DataQueue)))        /* 判断参数是否有效 */
    {
        Queue = (DataQueue *)Buf;

        OS_ENTER_CRITICAL();
                                                                /* 初始化结构体数据 */
        Queue->MaxData = (SizeOfBuf - (uint32)(((DataQueue *)0)->Buf)) / 
                         sizeof(QUEUE_DATA_TYPE);               /* 计算队列可以存储的数据数目 */
        Queue->End = Queue->Buf + Queue->MaxData;               /* 计算数据缓冲的结束地址 */
        Queue->Out = Queue->Buf;
        Queue->In = Queue->Buf;
        Queue->NData = 0;
        Queue->ReadEmpty = ReadEmpty;
        Queue->WriteFull = WriteFull;

        OS_EXIT_CRITICAL();

        return QUEUE_OK;
    }
    else
    {
        return NOT_OK;
    }
}

/*********************************************************************************************************
** 函数名称: QueueRead
** 功能描述: 获取队列中的数据
** 输　入: Ret:存储返回的消息的地址
**         Buf:指向队列的指针
** 输　出: NOT_OK     ：参数错误
**         QUEUE_OK   ：收到消息
**         QUEUE_EMPTY：无消息
** 全局变量: 无
** 调用模块: OS_ENTER_CRITICAL,OS_EXIT_CRITICAL
**
** 作　者: 陈明计
** 日　期: 2003年7月2日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
LOCAL ICACHE_FLASH_ATTR uint8 QueueRead(QUEUE_DATA_TYPE *Ret, void *Buf)
{
    uint8 err;
    DataQueue *Queue;

    err = NOT_OK;
    if (Buf != NULL)                                            /* 队列是否有效 */
    {                                                           /* 有效 */
        Queue = (DataQueue *)Buf;
        
        OS_ENTER_CRITICAL();
        
        if (Queue->NData > 0)                                   /* 队列是否为空 */
        {                                                       /* 不空         */
            *Ret = Queue->Out[0];                               /* 数据出队     */
            Queue->Out++;                                       /* 调整出队指针 */
            if (Queue->Out >= Queue->End)
            {
                Queue->Out = Queue->Buf;
            }
            Queue->NData--;                                     /* 数据减少      */
            err = QUEUE_OK;
        }
        else
        {                                                       /* 空              */
            err = QUEUE_EMPTY;
            if (Queue->ReadEmpty != NULL)                       /* 调用用户处理函数 */
            {
                err = Queue->ReadEmpty(Ret, Queue);
            }
        }
        OS_EXIT_CRITICAL();
    }
    return err;
}

/*********************************************************************************************************
** 函数名称: QueueWrite
** 功能描述: FIFO方式发送数据
** 输　入: Buf :指向队列的指针
**         Data:消息数据
** 输　出: NOT_OK   :参数错误
**         QUEUE_FULL:队列满
**         QUEUE_OK  :发送成功
** 全局变量: 无
** 调用模块: OS_ENTER_CRITICAL,OS_EXIT_CRITICAL
**
** 作　者: 陈明计
** 日　期: 2003年7月2日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
#ifndef EN_QUEUE_WRITE
#define EN_QUEUE_WRITE      1
#endif

#if EN_QUEUE_WRITE > 0
LOCAL ICACHE_FLASH_ATTR uint8 QueueWrite(void *Buf, QUEUE_DATA_TYPE Data)
{
    uint8 err;
    DataQueue *Queue;

    err = NOT_OK;
    if (Buf != NULL)                                                    /* 队列是否有效 */
    {
        Queue = (DataQueue *)Buf;
        
        OS_ENTER_CRITICAL();
        
        if (Queue->NData < Queue->MaxData)                              /* 队列是否满  */
        {                                                               /* 不满        */
            Queue->In[0] = Data;                                        /* 数据入队    */
            Queue->In++;                                                /* 调整入队指针*/
            if (Queue->In >= Queue->End)
            {
                Queue->In = Queue->Buf;
            }
            Queue->NData++;                                             /* 数据增加    */
            err = QUEUE_OK;
        }
        else
        {                                                               /* 满           */
            err = QUEUE_FULL;
            if (Queue->WriteFull != NULL)                               /* 调用用户处理函数 */
            {
                err = Queue->WriteFull(Queue, Data, Q_WRITE_MODE);
            }
        }
        OS_EXIT_CRITICAL();
    }
    return err;
}
#endif

/*********************************************************************************************************
** 函数名称: QueueWriteFront
** 功能描述: LIFO方式发送数据
** 输　入: Buf:指向队列的指针
**         Data:消息数据
** 输　出: QUEUE_FULL:队列满
**         QUEUE_OK:发送成功
** 全局变量: 无
** 调用模块: OS_ENTER_CRITICAL,OS_EXIT_CRITICAL
**
** 作　者: 陈明计
** 日　期: 2003年7月2日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
#ifndef EN_QUEUE_WRITE_FRONT
#define EN_QUEUE_WRITE_FRONT    0
#endif

#if EN_QUEUE_WRITE_FRONT > 0
LOCAL ICACHE_FLASH_ATTR uint8 QueueWriteFront(void *Buf, QUEUE_DATA_TYPE Data)
{
    uint8 err;
    DataQueue *Queue;

    err = NOT_OK;
    if (Buf != NULL)                                                    /* 队列是否有效 */
    {
        Queue = (DataQueue *)Buf;
        
        OS_ENTER_CRITICAL();
        
        if (Queue->NData < Queue->MaxData)                              /* 队列是否满  */
        {                                                               /* 不满 */
            Queue->Out--;                                               /* 调整出队指针 */
            if (Queue->Out < Queue->Buf)
            {
                Queue->Out = Queue->End - 1;
            }
            Queue->Out[0] = Data;                                       /* 数据入队     */
            Queue->NData++;                                             /* 数据数目增加 */
            err = QUEUE_OK;
        }
        else
        {                                                               /* 满           */
            err = QUEUE_FULL;
            if (Queue->WriteFull != NULL)                               /* 调用用户处理函数 */
            {
                err = Queue->WriteFull(Queue, Data, Q_WRITE_FRONT_MODE);
            }
        }
        OS_EXIT_CRITICAL();
    }
    return err;
}
#endif

/*********************************************************************************************************
** 函数名称: QueueNData
** 功能描述: 取得队列中数据数
** 输　入: Buf:指向队列的指针
** 输　出: 消息数
** 全局变量: 无
** 调用模块: OS_ENTER_CRITICAL,OS_EXIT_CRITICAL
**
** 作　者: 陈明计
** 日　期: 2003年7月2日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
#ifndef EN_QUEUE_NDATA
#define EN_QUEUE_NDATA    1
#endif

#if EN_QUEUE_NDATA > 0
LOCAL ICACHE_FLASH_ATTR uint16 QueueNData(void *Buf)
{
    uint16 temp;
    
    temp = 0;                                                   /* 队列无效返回0 */
    if (Buf != NULL)
    {
        OS_ENTER_CRITICAL();
        temp = ((DataQueue *)Buf)->NData;
        OS_EXIT_CRITICAL();
    }
    return temp;
}

#endif

/*********************************************************************************************************
** 函数名称: QueueSize
** 功能描述: 取得队列总容量
** 输　入: Buf:指向队列的指针
** 输　出: 队列总容量
** 全局变量: 无
** 调用模块: OS_ENTER_CRITICAL,OS_EXIT_CRITICAL
**
** 作　者: 陈明计
** 日　期: 2003年7月2日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
#ifndef EN_QUEUE_SIZE
#define EN_QUEUE_SIZE    1
#endif

#if EN_QUEUE_SIZE > 0
LOCAL ICACHE_FLASH_ATTR uint16 QueueSize(void *Buf)
{
    uint16 temp;
    
    temp = 0;                                                   /* 队列无效返回0 */
    if (Buf != NULL)
    {
        OS_ENTER_CRITICAL();
        temp = ((DataQueue *)Buf)->MaxData;
        OS_EXIT_CRITICAL();
    }
    return temp;
}
#endif

/*********************************************************************************************************
** 函数名称: OSQFlush
** 功能描述: 清空队列
** 输　入: Buf:指向队列的指针
** 输　出: 无
** 全局变量: 无
** 调用模块: OS_ENTER_CRITICAL,OS_EXIT_CRITICAL
**
** 作　者: 陈明计
** 日　期: 2003年7月2日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
#ifndef EN_QUEUE_FLUSH
#define EN_QUEUE_FLUSH    1
#endif

#if EN_QUEUE_FLUSH > 0
LOCAL ICACHE_FLASH_ATTR void QueueFlush(void *Buf)
{
    DataQueue *Queue;
    
    if (Buf != NULL)                                                /* 队列是否有效 */
    {                                                               /* 有效         */
        Queue = (DataQueue *)Buf;
        OS_ENTER_CRITICAL();
        Queue->Out = Queue->Buf;
        Queue->In = Queue->Buf;
        Queue->NData = 0;                                           /* 数据数目为0 */
        OS_EXIT_CRITICAL();
    }
}
#endif

/*********************************************************************************************************
**                            End Of QueueOperation
********************************************************************************************************/
LOCAL struct WIFI_PORT_DATA_t WifiReceiveData;
	  LOCAL  WIFI_PORT_STATUS WifiReceiveStat;
LOCAL ICACHE_FLASH_ATTR uint32 get_framer(uint8 *pusrdata, uint16 length)
{
	uint32 i;

	if(WIFI_RECEIVER_LENGTH == WifiReceiveStat && \
	  0 == pusrdata[0] && 4 <= length)
	{
		os_memset(&WifiReceiveData, 0, sizeof(struct WIFI_PORT_DATA_t));

		for(i = 0; i < 4; i++)
			WifiReceiveData.length = ((WifiReceiveData.length) << 8) + \
									   pusrdata[i];

		WifiReceiveStat = WIFI_RECEIVER_STRING;
	}
	else if(WIFI_RECEIVER_STRING == WifiReceiveStat)
	{
		os_memcpy(WifiReceiveData.framer, pusrdata, WifiReceiveData.length);

		i = WifiReceiveData.length;

		WifiReceiveStat = WIFI_RECEIVER_LENGTH;
	}
/*	else
	{
		os_printf("%u, %s\n\r", length, pusrdata);
		i = os_strlen(pusrdata);
	}*/

	return i;
}

LOCAL struct espconn *pespconn;
ICACHE_FLASH_ATTR sint8 tcp_send_data(char* data, unsigned int length)
{
	sint8 res = 0;
	uint8 Byte[2048];
	uint32 i = 0, send_length;

	if(NULL == pespconn)
		return -1;

	os_memset(Byte, 0, sizeof(Byte));
	os_memcpy(Byte + 4, data, length);

	Byte[length + 4] = 0;
	Byte[length + 5] = 0;

	send_length = length;
	for(i =0; i < 4; i++)
	{
		if(0 == send_length)
			break;
		else
			Byte[3 - i] = send_length % 0x100;
		send_length /= 0x100;
	}

	res = espconn_send(pespconn, Byte, length+4);
	//os_printf("Res %d, %s\n\r", res, data);
#if 0
	if(0 != res)
	{
		while(0 != buff_isBusy_flag);

		for(i = 0; i < length+4; i++)
			QueueWrite(ReSendBuff, Byte[i]);
	}
#else
	if(0 != res /*&& 0 == buff_isBusy_flag*/)
	{
		for(i = 0; i < length+4; i++)
			QueueWrite(ReSendBuff, Byte[i]);
	}

#endif

	return res;
}

/******************************************************************************
 * FunctionName : webserver_recv
 * Description  : Processing the received data from the server
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/
LOCAL ICACHE_FLASH_ATTR void webserver_recv(void *arg, char *pusrdata, unsigned short length)
{
	sint8  res;
	uint32 i = 0, send_length;
	static uint32 times = 0;
	static uint32 buffer_offset;
	uint8  upload_data[512];
	static uint32 electric_value = 0;
	uint8* pByte = NULL;
	struct espconn *pespconn0 = NULL;

	if(NULL == pusrdata)
		return ;

	pespconn0 = arg;
	//os_printf("pespconn0->link_cnt %u\n\r", pespconn0->link_cnt);
	if(0 != pespconn0->link_cnt)
		return ;

	pespconn = arg;

	WifiModule.DeviceFindFlag = 1;
	user_link_led_timer_rst();

	get_framer(pusrdata, length);
	if(0 != os_strlen(WifiReceiveData.framer))
	{
		//os_printf("%u, %s\n\r", WifiReceiveData.length, WifiReceiveData.framer);
		user_app_command_analyze(WifiReceiveData);
	}

	if (wifi_get_opmode() == SOFTAP_MODE)
	{
		//os_printf("ESP8266 Work Mode : SOFTAP_MODE\n\r");
	}
	else
	{
		//os_printf("ESP8266 Work Mode : STATION_MODE\n\r");
	}
}


/******************************************************************************
 * FunctionName : webserver_recon
 * Description  : the connection has been err, reconnection
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL ICACHE_FLASH_ATTR void webserver_recon(void *arg, sint8 err)
{
    struct espconn *pesp_conn = arg;

   // os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n", pesp_conn->proto.tcp->remote_ip[0],
   // 		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
   // 		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port, err);

	pespconn = NULL;
   	buff_isBusy_flag = 0;
	QueueCreate(ReSendBuff, sizeof(ReSendBuff), NULL, NULL);
	WifiModuleInit();
	user_link_led_timer_done();
	WifiModule.WifiStatus = wifi_conenct_err;
	wifi_module_set_wifi_status(wifi_conenct_err);
}

/******************************************************************************
 * FunctionName : webserver_recon
 * Description  : the connection has been err, reconnection
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL ICACHE_FLASH_ATTR void webserver_discon(void *arg)
{
    struct espconn *pesp_conn = arg;

    //os_printf("webserver's %d.%d.%d.%d:%d disconnect\n", pesp_conn->proto.tcp->remote_ip[0],
    //    		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
    //    		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port);

	pespconn = NULL;
	buff_isBusy_flag = 0;
	QueueCreate(ReSendBuff, sizeof(ReSendBuff), NULL, NULL);
	WifiModuleInit();
	user_link_led_timer_done();
	WifiModule.WifiStatus = wifi_conenct_err;
	wifi_module_set_wifi_status(wifi_conenct_err);
}

/******************************************************************************
 * FunctionName : user_accept_listen
 * Description  : server listened a connection successfully
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL ICACHE_FLASH_ATTR void webserver_listen(void *arg)
{
    struct espconn *pesp_conn = arg;

    espconn_regist_recvcb(pesp_conn, webserver_recv);
    espconn_regist_reconcb(pesp_conn, webserver_recon);
    espconn_regist_disconcb(pesp_conn, webserver_discon);
}

LOCAL ICACHE_FLASH_ATTR void user_espconn_sent_cb(void *arg)
{
	uint32 length;
	sint8  res;
	uint32 i;
	uint8 Byte[3072];

	buff_isBusy_flag = 1;
	if(0 == (length = QueueNData(ReSendBuff)))
	{
		buff_isBusy_flag = 0;
		return ;
	}

	os_memset(Byte, 0, sizeof(Byte));

	for(i = 0; i < length; i++)
	{
		QueueRead(Byte + i, ReSendBuff);
		//os_printf("0x%02x ", pByte[i]);
	}
	buff_isBusy_flag = 0;

	pespconn = arg;
	res = espconn_send(pespconn, Byte, length);
	//os_printf("\n\rReSend Data >> %d, %u\n\r", res, length);
}


/******************************************************************************
 * FunctionName : user_webserver_init
 * Description  : parameter initialize as a server
 * Parameters   : port -- server port
 * Returns      : none
*******************************************************************************/
ICACHE_FLASH_ATTR void user_webserver_init(uint32 port)
{
    LOCAL struct espconn esp_conn;
    LOCAL esp_tcp esptcp;

	buff_isBusy_flag = 0;
	QueueCreate(ReSendBuff, sizeof(ReSendBuff), NULL, NULL);

    esp_conn.type = ESPCONN_TCP;
    esp_conn.state = ESPCONN_NONE;
    esp_conn.proto.tcp = &esptcp;
    esp_conn.proto.tcp->local_port = port;
    espconn_regist_connectcb(&esp_conn, webserver_listen);
	espconn_regist_sentcb(&esp_conn, user_espconn_sent_cb);
    espconn_accept(&esp_conn);
}

