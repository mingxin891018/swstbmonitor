#ifndef __SW_IPTVMW_H__
#define __SW_IPTVMW_H__
#ifdef __cplusplus
extern "C"
{
#endif
//输入事件类型定义
typedef enum 
{
	IPTVMW_INPUTEVNET_RC, //
	IPTVMW_INPUTEVNET_KB,
	IPTVMW_INPUTEVNET_MOUSE,
	IPTVMW_INPUTEVNET_MAX
}IPTVMW_INPUTEVENT_E;

//红外遥控器事件类型定义
typedef struct _iptvmw_rcevent
{
        uint32_t keycode;
        bool	isdown;
        bool    isrepeat;
}iptvmw_rcevent_t;
 

#define STB_EVENT_PAGE			100L 	//页面事件
#define STB_EVENT_RPC            200L  	//命令事件
#define STB_EVENT_APP_MANAGER    300L  	//应用管理事件
#define STB_EVENT_MEDIA			1000L 	//媒体事件
#define STB_EVENT_LOCALUI        2000L 	//UI更新事件

//一键信息收集OTT事件定义
#define STB_DFXEVENT_START		3001L   //启动
#define STB_DFXEVENT_STOP		3002L   //停止
#define STB_DFXEVENT_UPLOAD		3003L   //上传

#define STB_BOOTEVENT_START		4001L	//start
#define STB_BOOTEVENT_STOP		4002L	//stop
#define STB_BOOTEVENT_UPLOAD	4003L	//upload



//应用管理事件
#define STB_APP_ISINSTALLED      301L//应用是否安装
#define STB_APP_START            302L//启动应用
#define STB_APP_RESTART          303L//重启应用
#define STB_APP_STARTAPPBYINTENT 304L//带参数启动应用
#define STB_APP_INSTALLBYURL     305L//通过URL安装apk

//页面事件定义
#define STB_PAGE_LOAD_STARTED	101L
#define STB_PAGE_LOAD_FINISHED	102L
#define STB_PAGE_LOAD_ERROR		103L
#define STB_PAGE_SIZECHANGE		104L
#define STB_PAGE_AUTHING		    105L  //开始认证事件
#define STB_PAGE_AUTHED	        106L  //认真结束
#define STB_PAGE_AUTH_FAIL	    107L  //认证失败
#define STB_PAGE_LOADING_EBASE	108L  //ebase开始加载
#define STB_PAGE_LOADEND_EBASE	109L  //ebase加载失败
#define STB_PAGE_AUTH_LOGO	110L  //展示认证logo

//RPC事件定义
#define STB_RPC_MUTE				201L  //静音
#define STB_RPC_OPENSETTING		202L  //打开设置页面
#define STB_RPC_UPGRADE			203L  //启动升级
#define STB_RPC_SETVOLUME     	204L  //设置音量
#define STB_RPC_EXITIPTV         205L  //退出tvos
#define STB_RPC_REBOOT           206L  //重启
#define STB_RPC_RESTORE          207L  //恢复出厂设置
#define STB_RPC_OPENADB          208L  //打开adb服务
#define STB_RPC_CLOSEADB         209L  //关闭adb服务
#define STB_RPC_UPGRADEING		 210L  //启动升级
#define STB_RPC_UPGRADEFAIL    	 211L  //启动升级
#define STB_RPC_STARTDEBUGINFO       212L  //开始一键信息收集
#define STB_RPC_STOPDEBUGINFO        213L  //停止一键信息收集
#define STB_RPC_UPLOADDEBUGINFO      214L  //信息上报
#define STB_RPC_TURNOFFTOOLPORT      216L  //关闭管理工具

//媒体事件定义
#define STB_MEDIAEVENT_BEGIN			1001L	//开始
#define STB_MEDIAEVENT_BUFFER		1002L	//缓冲
#define STB_MEDIAEVENT_PLAY			1002L	//播放
#define STB_MEDIAEVENT_END			1004L	//播放结束
#define STB_MEDIAEVENT_PAUSE			1005L	//暂停
#define STB_MEDIAEVENT_STOP      1006L   //停止播放
#define STB_MEDIAEVENT_SEEK			1007L	//定位操作完成
#define STB_MEDIAEVENT_TOTIMESHIFT	1008L	//直播转时移
#define STB_MEDIAEVENT_TOLIVE		1009L	//时移转直播
#define STB_MEDIAEVENT_RECVNEWDATA	1010L	//接收到新的数据
#define STB_MEDIAEVENT_ERROR			1011L	//媒体错误
#define STB_MEDIAEVENT_VIDEOSIZE     1012L   //媒体尺寸
#define STB_MEDIAEVENT_RATESIZE      1013L   //媒体码率大小
#define STB_MEDIAEVENT_PLAYBYCHANNELNUM      1014L   //根据频道号播放
#define STB_MEDIAEVENT_FFWD      1015L   //快进播放
#define STB_MEDIAEVENT_FBWD      1016L   //快退播放
#define STB_MEDIAEVENT_PLAYBYURL      1017L   //根据频道号播放


//媒体错误事件
#define STB_MEDIAERROR_CONNECTFAIL	2001L	//连接媒体服务器失败
#define STB_MEDIAERROR_RCVTIMEOUT	2002L	//数据接收超时
#define STB_MEDIAERROR_NOTFOUND		2003L	//无法找到目标文件
#define STB_MEDIAERROR_DNSFAIL		2004L	//DNS查询失败
#define STB_MEDIAERROR_UNKNOWNFMT	2005L	//未知的流格式
#define STB_MEDIAERROR_UNSUPPORTURL	2006L	//不支持的url
#define STB_MEDIAERROR_NETWORKEXP	2007L	//网络异常
#define STB_MEDIAERROR_SYSTEMEXP		2008L	//系统错误

//LocalUI事件定义
#define STB_LOCALUI_MUTE 		 	2001L
#define STB_LOCALUI_TIMESHIFT		2002L
#define STB_LOCALUI_TOLIVE			2003L
#define STB_LOCALUI_TVLOGO			2004L
#define STB_LOCALUI_TVNO				2005L
#define STB_LOCALUI_AUDIOTRACK		2006L


#define STB_LOCALUI_HIDDEN			0L
#define STB_LOCALUI_SHOW				1L

//声道类型定义
#define STB_AUDIOTRACKTYPE_LEFT		1L
#define STB_AUDIOTRACKTYPE_STEREO_LR 2L
#define STB_AUDIOTRACKTYPE_STEREO_RL 3L
#define STB_AUDIOTRACKTYPE_RIGHT 	4L

//Surface类型定义
#define STB_SURFACE_EPG 				0L
#define STB_SURFACE_VIDEO 			1L
#define STB_SURFACE_SUBTITLE 		2L
#define STB_SURFACE_JVM 				3L

/**
 *@brief 	事件回调函数指针定义
 *@param 	type 事件类型(PAGE,MEDIAEVENT,MEDIAERROR)
 *@param 	wparam 事件对应值
 *@param 	lparam 事件扩展值
 *@param 	usrparam 用户参数
 *@return 	0 成功,其它错误号
 */
typedef int (*event_callback)( int type, uint32_t wparam, uint32_t lparam, uint32_t usrparam);

/**
 *@brief 	视频位置变化回调函数指针定义
 *@param 	x
 *@param 	y
 *@param 	w
 *@param 	h
 *@return 	0 成功,其它错误号
 */
typedef int (*video_callback)( int x, int y, int w, int h );

/**
 *@brief 	写参数回调函数指针
 *@param 	name
 *@param 	value
 *@return 	true=成功，false失败 
 */
typedef bool (*wcallback_t)(char* name ,char* value );

/**
 *@brief 	读参数回调函数指针
 *@param 	name
 *@param 	value
 *@return 	true=成功，false失败 
 */
typedef bool (*rcallback_t)(char *name, char* value, int size);

/**
 *@brief 	IPTV中间件模块初始化
  *@param 	egpsurface  	EPG显示图层
 *@param 	videosurface   视频显示图层
 *@return 	0 成功,其它错误号
 */
int sw_iptvmw_init( void* epgsurface, void* videosurface );

/**
 *@brief 	IPTV中间件模块退出
 *@param 	无
 *@return 	无
 */
void sw_iptvmw_deinit();

/**
 *@brief 	打开主页(异步打开)
 *@param 	无
 *@return	0 成功,其它错误号
 */
int sw_iptvmw_openhomepage();


/**
 *@brief 	打开指定的Url地址
 *@param 	url 要打开的网页地址
 *@return 	0 成功,其它错误号
 */
int sw_iptvmw_openurl( char* url );

/**
 *@brief 	取消当前打开的页面
 *@param 	无
 *@return 	无
 */
void sw_iptvmw_cancelload( );

/**
 *@brief 	暂停当前业务(停止流媒体的播放,释放相关资源)
 *@param 	无
 *@return 	0 成功,其它错误号
 */
int sw_iptvmw_pause( );

/**
 *@brief 	恢复IPTV业务
 *@param 	无
 *@return 	0 成功,其它错误号
 */
int sw_iptvmw_resume( void* epgsurface, void* videosurface );

/**
 *@brief 	发送遥控器键值消息到IPTV中间件模块
 *@param 	type 	输入事件类
 *@param 	event	事件
 *@param 	size	event 大小
 *@return 	0 成功,其它错误号
 */
int sw_iptvmw_onmessage( IPTVMW_INPUTEVENT_E type, void* event, int size);

/**
 *@brief 	设置媒体事件回调函数
 *@param 	type 	输入事件类
 *@param 	event	事件
 *@param 	size	event 大小
 *@return 	0 成功,其它错误号
 */
int sw_iptvmw_set_eventcallback( event_callback callback, uint32_t usrparam );

/**
 *@brief 	获取媒体事件回调函数指针
 *@param 	callback[output] 
 *@param 	usrparam[output]
 *@return 	0 成功,其它错误号
 */
int sw_iptvmw_get_eventcallback( event_callback *callback, uint32_t *usrparam );

/**
 *@brief    设置是否安装apk的回调函数指针
 *@param  callback[input]
 *
 */ 
void sw_iptvmv_set_isinstallcallback(wcallback_t callback);

/**
 *@brief    是否安装apk
 *@param    appname
 */
BOOL sw_iptvmv_isinstall(char *appname);

/**
 *@brief 	设置视频位置变化回调函数
 *@param 	callback 
 *@return 	0 成功,其它错误号
 */
int sw_iptvmw_set_videocallback( video_callback callback );

/**
 *@brief 	获取视频位置变化回调函数指针
 *@param 	无 
 *@return 	函数指针
 */
video_callback sw_iptvmw_get_videocallback();

/**
 *@brief 	设置参数参数的读写接口，主要功能是iptvmw模块可以读取和设置存储在java数据库或者flash里面的参数
 *@param 	无 
 *@return 	0 成功，其他错误码
 */
int sw_iptvmw_set_javapara_callback(wcallback_t wfun,  rcallback_t rfun);

/**
 *@brief 	获取参数的读写接
 *@param 	无 
 *@return 	函数指针
 */
int sw_iptvmw_get_javapara_callback(wcallback_t *wfun, rcallback_t *rfun);

/**
 *@brief 	封装给anroid抛事件接口
 *@param 	type 事件类型(PAGE, MEDIAEVENT, MEDIAERROR)
 *@param 	wparam 事件对应值
 *@param 	lparam 事件扩展值
 *@param 	usrparam 用户参数
 *@return 	0＝成功，-1＝失败
 */
int sw_monitor_on_event(int type, uint32_t wparam, uint32_t lparam, uint32_t usrparam);

/**
 *@brief 参数更新后，通知接口
 *@param param 更新的参数名称
 *@return 0=成功，-1=失败
 */
int sw_iptvmw_update_parameter_notify(char *param);

int sw_monitor_get_javapara_callback(wcallback_t *wfun, rcallback_t *rfun);

int sw_monitor_update_parameter_notify(char *param);
#ifdef __cplusplus
}
#endif

#endif //__SW_IPTVMW_H__
