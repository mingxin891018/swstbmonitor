#include "swapi.h"
#include "swos.h"
#include "swlog.h"
#include "swthrd.h"
#include "swevtdispatcher.h"
#include "swapp.h"
#include "swparameter.h"
#include "swshelld.h"
#include "swstb.h"
#include "swnetwork.h"
#include "hwstbmonitor.h"
#include "hwnmpd_priv.h"

//参数初始化和退出[swapp_parameter.c中定义]
extern int sw_app_parameter_init();
extern int sw_app_parameter_exit();
extern int sw_androidporting_init();
extern void hw_stbmonitor_set_enable(bool able);
//运行时上下文
static  swcontext_t* m_app = NULL;
//static	HANDLE m_thrd = NULL;
static 	HANDLE m_timer_thrd = NULL;

//事件回调处理函数指针
static event_callback m_eventcallback = NULL;
//扩展参数
static uint32_t m_usrparam = 0;
//写java数据参数回调函数
static wcallback_t m_wcallback = NULL;
static rcallback_t m_rcallback = NULL;
//读java数据库参数回调函数
//强制停媒体线程
//static  HANDLE m_mediastop_thrd = NULL; 

static int m_iptv_status = 0; //标志iptv是在运行(0)或者sleep(1)

static int m_anew_authentcation = 0; /* 判断唤醒后是否需要重新认证*/

//app线程处理
//static bool on_app_proc( uint32_t wparam, uint32_t lparam );
//Timer
//static bool on_timer_proc(uint32_t wparam,uint32_t lparam);

/**
 *@brief 	IPTV中间件模块退出
 *@param 	无
 *@return 	无
 */
void sw_monitor_deinit()
{
   HWNMPD_LOG_DEBUG( "[%s] -----------------------\n", __FUNCTION__ );
	sw_app_exit();
}


/**
 *@brief 	发送遥控器键值消息到IPTV中间件模块
 *@param 	type 	输入事件类
 *@param 	event	事件
 *@param 	size	event 大小
 *@return 	0 成功,其它错误号
 */
int sw_monitor_onmessage( IPTVMW_INPUTEVENT_E type, void* event, int size)
{

    HWNMPD_LOG_DEBUG( "[%s] +++++++++++++++++++++++++++++++++++++\n", __FUNCTION__ );

 //   bool result = false;
	if(type == 0 && event)
	{

	}
	return 0;
}

/**
 *@brief 	设置媒体事件回调函数
 *@param 	type 	输入事件类
 *@param 	event	事件
 *@param 	size	event 大小
 *@return 	0 成功,其它错误号
 */
int sw_monitor_set_eventcallback( event_callback callback, uint32_t usrparam )
{
    HWNMPD_LOG_DEBUG( "[%s] -----------------------\n", __FUNCTION__ );
	m_eventcallback = callback;
	m_usrparam = usrparam;
    return 0;
}

/**
 *@brief 	获取媒体事件回调函数指针
 *@param 	callback[output] 
 *@param 	usrparam[output]
 *@return 	0 成功,其它错误号
 */
int sw_monitor_get_eventcallback( event_callback *callback, uint32_t *usrparam )
{
	if( callback )
		*callback = m_eventcallback;
	if( usrparam )
		*usrparam = m_usrparam;
    return 0;
}

//当需要盒子主动链接工具时的接口
int sw_nmp_connect(char *server_ip)
{
	HWNMPD_LOG_DEBUG("#########################connect, server_ip:%s\n",server_ip);
	if(server_ip != NULL)
		return hw_nmp_connect( server_ip );
	return -1;
}

int sw_monitor_init(wcallback_t wfun,  rcallback_t rfun)
{
#ifdef SUPPORT_IPTVMW_EXE
	sw_androidporting_init();
#endif
    m_wcallback = wfun;
    m_rcallback = rfun;
	//sw_app_handle_signal_init();
	m_app = (swcontext_t*)malloc(sizeof(swcontext_t));
	if( m_app == NULL)
		return -1;
	sw_log_init( LOG_LEVEL_ALL, "logcat", "all" );
//	sw_log_add_target( "logcat" );
//	hw_log_init();
	//初始化消息分发系统
 	//m_app->event_dispatcher = sw_evtdispatcher_instance();
	sw_app_parameter_init();
    HWNMPD_LOG_DEBUG( "[%s]:[%d] -----------------------\n", __FUNCTION__, __LINE__ );
	//初始化中间件
	/* 需要在ip连接成功之前初始化media否则无法启动irdeto的services */
#if 0
	m_thrd = sw_thrd_open( "tMsgLoopProc", 10,SW_SCHED_NORMAL,512 << 10, on_app_proc,0,0);
	if( NULL == m_thrd )
	{
		HWNMPD_LOG_DEBUG("%s Created main thread failed\n", __FUNCTION__);
		return -3;
	}
	sw_thrd_resume( m_thrd );
	m_timer_thrd = sw_thrd_open( "tTimerProc", 100,SW_SCHED_NORMAL,8192, on_timer_proc,0,0);
	if( NULL == m_timer_thrd )
	{
		HWNMPD_LOG_DEBUG("%s Created timer thread failed\n", __FUNCTION__);
		return -3;
	}
    HWNMPD_LOG_DEBUG( "[%s]:[%d] -----------------------\n", __FUNCTION__, __LINE__ );
	sw_thrd_resume( m_timer_thrd );
#endif
	hw_nmp_init();

    char macbuffer[32];
    char param_buffer[1024];
    char epgurl[1024];
    char hv_buf[64];
    char hf_buf[64];

    sw_memset(macbuffer, sizeof(macbuffer), 0, sizeof(macbuffer));
    sw_memset(param_buffer, sizeof(param_buffer), 0, sizeof(param_buffer));
    sw_memset(epgurl, sizeof(epgurl), 0, sizeof(epgurl));
    sw_memset(hv_buf, sizeof(hv_buf), 0, sizeof(hv_buf));
    sw_memset(hf_buf, sizeof(hf_buf), 0, sizeof(hf_buf));

    sw_android_property_get("mac",macbuffer,32);
    sw_android_property_get("hardware_type",param_buffer,1024);
    sw_android_property_get("epgurl",epgurl,1024);
    sw_android_property_get("hardware_version",hv_buf,sizeof(hv_buf));
    sw_android_property_get("hardware_type",hf_buf,sizeof(hf_buf));

    HWNMPD_LOG_DEBUG("mac:%s\n",!SENSITIVE_PRINT ? "..." : macbuffer);
    HWNMPD_LOG_DEBUG("________param_buffer:%s\n",param_buffer);
    HWNMPD_LOG_DEBUG("epgurl:%s\n",epgurl);
    HWNMPD_LOG_DEBUG("hardware_version:%s\n",hv_buf);
    HWNMPD_LOG_DEBUG("hardware_type:%s\n",hf_buf);
    HWNMPD_LOG_DEBUG("----------sw_parameter_get_num():%d\n",sw_parameter_get_num());
    char logourl[1024];

    sw_memset(logourl, sizeof(logourl), 0, sizeof(logourl));
    sw_android_property_get("pad_boot_logo_pic_url",logourl,sizeof(logourl));
    HWNMPD_LOG_DEBUG("logourl:%s\n",logourl);
    HWNMPD_LOG_DEBUG("**************sw_monitor_init end\n");

	return 0;
}

void sw_app_exit()
{
	HWNMPD_LOG_DEBUG( "-------------------[%s] -----------------\n", __FUNCTION__ );
    //sw_evtdispatcher_enable(m_app->event_dispatcher, false);
    if(m_timer_thrd)
        sw_thrd_close(m_timer_thrd, 100);
	//sw_app_handle_signal_quit();
	sw_app_parameter_exit();
    //if( m_app )
    //    free(m_app);
}

#if 0
static bool on_app_proc( uint32_t wparam, uint32_t lparam )
{
//	sw_evtdispatcher_do( m_app->event_dispatcher, -1);
	sw_thrd_delay(1000);
	return true;
}

static bool on_timer_proc( uint32_t wparam, uint32_t lparam )
{
//	static sw_event_t event;
//	event.type = SW_HEARTBEAT_EVENT;
//	sw_evtdispatcher_on_event( m_app->event_dispatcher, &event);
	sw_thrd_delay(1000);
	return true;
}
#endif

int sw_monitor_set_javapara_callback(wcallback_t wfun,  rcallback_t rfun)
{
    m_wcallback = wfun;
    m_rcallback = rfun;

    return 0;
}

int sw_monitor_get_javapara_callback(wcallback_t *wfun, rcallback_t *rfun)
{
    if(wfun)
        *wfun = m_wcallback;
    if(rfun)
        *rfun = m_rcallback;
    return 0;
}


int sw_monitor_on_event(int type, uint32_t wparam, uint32_t lparam, uint32_t usrparam)
{
    if(m_eventcallback)
    {
        return m_eventcallback(type, wparam, lparam,  usrparam == 0 ? m_usrparam : usrparam);
    }
    return -1;
}

/**
 *@brief: 关注一些参数的更新，使参数更新了实时有效
 */
int sw_monitor_update_parameter_notify(char *param)
{
    HWNMPD_LOG_DEBUG("[%s] param = %s\n", __FUNCTION__, param);

    if(param == NULL)
        return -1;

    char buf[128];
    sw_memset(buf, sizeof(buf), 0, sizeof(buf));
//	sw_app_parameter_updatefrom_depot(); 不能调用此接口，否则会出现释放参数模块后，其它线程读写参数导致应用崩溃
    sw_thrd_delay(100);
	if(strncmp(param, "stbmonitorable", strlen("stbmonitorable")) == 0)
    {
        if(atoi(buf) > 0)
        {
            hw_stbmonitor_enable(true);
            sw_parameter_set_int("stbmonitorable", 1); //刷新内存中保持的参数
        }
        else
        {
            hw_stbmonitor_enable(false);
            sw_parameter_set_int("stbmonitorable", 0);
        }
        return 0;
    }
    if(strncmp(param, "home_page", strlen("home_page")) == 0)
    {
        m_anew_authentcation = 1;
        if(buf[0] !=  0)
            sw_parameter_set("home_page", buf); //刷新内存中保持的参数
        return 0;
    }
    if(strncmp(param, "home_page2", strlen("home_page2")) == 0)
    {
        m_anew_authentcation = 1;
        if(buf[0] !=  0)
            sw_parameter_set("home_page2", buf); //刷新内存中保持的参数
        return 0;
    }
    if(strncmp(param, "default_iptv_ott", strlen("default_iptv_ott")) == 0)  //ott和iptv切换后需要重新认证
    {
        m_anew_authentcation = 1;
    }
    return 0;
}

int sw_monitor_get_status(void)
{
    return m_iptv_status;
}

