#include "swapi.h"
#include "swstbmonitorserver.h"
#include "hwnmpd_priv.h"
#include "swippkg.h"
#include "swupgrade2.h"
#include "swupgrade2_config.h"
#include "swthrd.h"
#include "swevtdispatcher.h"
#include "swapp.h"
#include "swchannels.h"
#include "swparameter.h"
#include "hwlog.h"
#include "swmedia.h"
#include "hwmedia.h"
#include "swplayer.h"
#include "swbrowser.h"
#include "hwstbmonitor.h"
#include "swstb.h"
#include "string_ext.h"
#include <paths.h>
//#include "call_swserice.h"

static sw_stbmonitor_callback_funcs_t m_scbfs;
static swupg_file_t m_upgrade_file;
static bool m_callback_function_is_add  = false;
static int updatefp = -1;
static int count = 0;
static bool m_is_log_on = true;
static char last_server_ip[32] = {0};

static int SW_Exec(const char *command);

extern char* sw_network_get_currentip();
extern int property_get(const char *key, char *value, const char *default_value);

static int cb_on_set_log_type(char *type)
{
	HWNMPD_LOG_DEBUG("===============,%s,%d,type=%s\n",__FUNCTION__,__LINE__,type);
	sw_parameter_set("LogType", type);
	int ret = 0;
	char value[32] = {0};
	sw_strlcpy(value, sizeof(value), type, sizeof(value));
	if( m_is_log_on )
		ret = sw_android_property_set("LogType", value);
	return ret;
}

static int cb_on_set_log_level(char *level)
{
	HWNMPD_LOG_DEBUG("===============,%s,%d,level=%s\n",__FUNCTION__,__LINE__,level);
	//int ret = 0;
	int loglevel = 0;
	char value[32] = {0};
	sw_strlcpy(value, sizeof(value), level, sizeof(value));
	sw_parameter_set("LogLevel", level);
	if( m_is_log_on )
	{
		switch( atoi(value) )
		{
			case 0:
				loglevel = LOG_LEVEL_ALL;
				break;
			case 3:
				loglevel = LOG_LEVEL_ERROR;
				break;
			case 6:
				loglevel = LOG_LEVEL_INFO;
				break;
			case 7:
				loglevel = LOG_LEVEL_DEBUG;
				break;
			default:
				break;
		}
		sw_android_property_set_int("LogLevel", loglevel);
//		ret = sw_parameter_set_int("log_level", loglevel);
//		sw_parameter_save();
//		hw_log_validate_loglevel(NULL);
	}
	return 1;
}

static int cb_on_set_log_out_type(char *out_type)
{
	HWNMPD_LOG_DEBUG("===============%d,out_tpye=%s\n",__LINE__,out_type);
	sw_parameter_set("LogOutPutType", out_type);
//	handle_LogOutPutType(NULL, out_type, 0,1 );
//    hw_syslog_send_command("start");
	int ret = 0;
	char value[32] = {0};
	char ftpserver[128] = {0};
	char logoutputtype[4] = {0};
	sw_strlcpy(value, sizeof(value), out_type, sizeof(value));
	if( atoi(value) == 0)
	{
		sw_parameter_set_int("log_level", LOG_LEVEL_OFF);
		sw_android_property_set_int("LogLevel", LOG_LEVEL_OFF);
//		hw_log_validate_loglevel(NULL);
		m_is_log_on = false;
	}
	else
		m_is_log_on = true;
	if(atoi(value) == 0)
		sw_strlcpy(value, sizeof(value), "00", sizeof(value));
	else if(atoi(value) == 1)
		sw_strlcpy(value, sizeof(value), "01", sizeof(value));
	else if(atoi(value) == 2)
		sw_strlcpy(value, sizeof(value), "02", sizeof(value));
	else if(atoi(value) == 3)
		sw_strlcpy(value, sizeof(value), "03", sizeof(value));
	ret = sw_parameter_set("logoutputtype", value);
	sw_parameter_save();
	sw_android_property_get("logoutputtype", logoutputtype, sizeof(logoutputtype));
	if ( strcmp(logoutputtype, "01") == 0 || strcmp(logoutputtype, "03") == 0)
	{
		sw_android_property_get("logftpserver", ftpserver, sizeof(ftpserver));
		HWNMPD_LOG_DEBUG("===============%d,%s\n",__LINE__,ftpserver);
		sw_log_add_target(ftpserver);
	}
	return ret;
}

static int cb_on_media_stop()
{
	HWNMPD_LOG_DEBUG("===============%d,%s\n",__LINE__,__FUNCTION__);
	sw_monitor_on_event( STB_EVENT_MEDIA, STB_MEDIAEVENT_STOP , (uint32_t)0, (uint32_t)0);
	sw_thrd_delay(10);
	return 0;
}

static int cb_on_media_ffwd()
{
	sw_monitor_on_event( STB_EVENT_MEDIA, STB_MEDIAEVENT_FFWD , (uint32_t)0, (uint32_t)0);
	sw_thrd_delay(10);
	return 0;
}

static int cb_on_media_fbwd()
{
	sw_monitor_on_event( STB_EVENT_MEDIA, STB_MEDIAEVENT_FBWD , (uint32_t)0, (uint32_t)0);
	sw_thrd_delay(10);
	return 0;
}

static int cb_on_media_pause()
{
	HWNMPD_LOG_DEBUG("===============%d,%s\n",__LINE__,__FUNCTION__);
	sw_monitor_on_event( STB_EVENT_MEDIA, STB_MEDIAEVENT_PAUSE , (uint32_t)0, (uint32_t)0);
	sw_thrd_delay(10);
	return 0;
}

static int cb_on_media_play(char* url)
{
#ifndef SUPPORT_MDZZ
	char channelnum[16];
	char channelurl[1024];
	sw_memset(channelnum, sizeof(channelnum), 0, sizeof(channelnum));
	sw_memset(channelurl, sizeof(channelurl), 0, sizeof(channelurl));
	if( url == NULL || url[0] == '\0' )
	{
		return -1;
	}
	xstrgetval( url, "Channel:", channelnum, sizeof(channelnum));
	
	HWNMPD_LOG_DEBUG("===============%d,channelnum=%s\n",__LINE__,channelnum);
	if ( channelnum[0] != '\0' )//play by channel num
	{
		HWNMPD_LOG_DEBUG("===============%d,channelnum=%s\n",__LINE__,channelnum);
		sw_monitor_on_event( STB_EVENT_MEDIA, STB_MEDIAEVENT_PLAYBYCHANNELNUM , (uint32_t)0 , (uint32_t)channelnum);
		sw_thrd_delay(10);
		return 0;
	}
	else  //play by url
	{
		HWNMPD_LOG_DEBUG("===============%d,channelurl=%s\n",__LINE__,url);
		sw_monitor_on_event( STB_EVENT_MEDIA, STB_MEDIAEVENT_PLAYBYURL , (uint32_t)0 ,(uint32_t)url);
		sw_thrd_delay(10);
		return 0;
	
	}
#else
	HWNMPD_LOG_DEBUG("===============%d,define MDZZ can not playurl,url=%s\n",__LINE__,url);
#endif
	return -1;

}

static int cb_on_upgrade_online()
{
	return -1;
}

static int cb_on_set_test_mode()
{
	return -1;
}

static int cb_on_set_autotest_mode()
{
	return -1;
}

static int cb_on_set_work_mode()
{
	return -1;
}

static int cb_on_set_scriptrecord_mode()
{
	return -1;
}

static int cb_on_restore_setting()
{
	sw_monitor_on_event(STB_EVENT_RPC, STB_RPC_RESTORE,(uint32_t)0, (uint32_t)0 );
	return 0;
}

static int cb_on_read_all_parameter( char *buf, int size )
{
	int ret = 0;
	int buflen = 0;
	char val_buf[2048];

	/* û??ָ???????????????????в???  HW_PARAM_DESC m_Hwpara_Table*/
	if( buf != NULL )
	{
		int tsize = sw_stbmonitor_hwparameter_size();
		int index = 0;
		char *paramsname = NULL;
		do {
			val_buf[0] = '\0';
			paramsname = NULL;
			ret = sw_stbmonitor_hwparameter_read_byindex(index, &paramsname, val_buf, sizeof(val_buf)-1);
			if ( ret == 0 && paramsname != NULL)
				buflen += sw_snprintf( buf+buflen, size-buflen, 0, size-buflen, "%s = %s\r\n", paramsname, val_buf );
		} while(++index < tsize);
	}
	return buflen;
}

static int cb_on_read_channellist( char *buf, int size )
{
	HWNMPD_LOG_DEBUG("===============read_channellist,size=%d\n",size);
	sw_android_property_get("ChannelList", buf, size);
	return strlen(buf);
}

static int cb_on_stb_reboot()
{
	sw_monitor_on_event(STB_EVENT_RPC, STB_RPC_REBOOT,(uint32_t)0, (uint32_t)0 );
	return 0;
}

static int cb_on_stb_close_remote_connect()
{
	sw_monitor_on_event(STB_EVENT_RPC, STB_RPC_TURNOFFTOOLPORT, (uint32_t)0, (uint32_t)0 );
	return 0;
}

static int cb_on_read_parameter( char *name, char *value, int size)
{
	int ret = sw_stbmonitor_hwparameter_read(name, value, size);
	if(ret == 0)
		return strlen(value);
	else
		return -1;
}

static int cb_on_write_parameter( char *name, char *value, int save)
{
	return sw_stbmonitor_hwparameter_write(name, value, strlen(value));
}

static int percent_size_flag = 0;
static int now_size = 0;
static int cb_on_upgrade_data( unsigned char *buf, int size, int total_size, bool force )
{
	now_size = now_size + size;
//	HWNMPD_LOG_DEBUG("========now_size=%d=========cb_on_upgrade_data,buf=%d,size=%d,total_size=%d\n",now_size,sizeof(buf),size,total_size);
	if(count == 0)
	{
		HWNMPD_LOG_DEBUG("----------------upgrade start----------------\n");
		char buf[32];
		buf[0] = '\0';
		sw_android_property_set("IPTV_monitor_upgrade", buf);
		sw_monitor_on_event(STB_EVENT_RPC, STB_RPC_UPGRADEING,(uint32_t)0, (uint32_t)0 );//开始升级
		property_get("sys.upgrade.cache.path", buf, "0");
		SW_Exec("/system/bin/rm -rf /pltv/movie/*");
		HWNMPD_LOG_DEBUG("------------------------------->sys.upgrade.cache.path = %s\n", buf);
		if( strcmp(buf, "pltv") == 0 )
		{
			if ( remove("pltv/update/update.zip") == -1 )
			{
				HWNMPD_LOG_DEBUG("remove /cache/update/update.zip failed \n");
			}
			updatefp = open("/pltv/update/update.zip", O_WRONLY | O_CREAT | O_TRUNC,777 );
			if ( updatefp == -1 )
			{
				HWNMPD_LOG_DEBUG("open /cache/update/update.zip failed \n");
			}
		}
		else
		{
			if ( remove("cache/update/update.zip") == -1 )
			{
				HWNMPD_LOG_DEBUG("remove /cache/update/update.zip failed \n");
			}
			updatefp = open("/cache/update/update.zip", O_WRONLY | O_CREAT | O_TRUNC,777 );
			if ( updatefp == -1 )
			{
				HWNMPD_LOG_DEBUG("open /cache/update/update.zip failed \n");
			}
		}
		count = 1;
	}
	int ret = -1;
	int i = 0;
	if( now_size == total_size )
	{
		percent_size_flag = 100;
		sw_monitor_on_event(STB_EVENT_RPC, STB_RPC_UPGRADEING, (uint32_t)percent_size_flag, (uint32_t)0);//升级中
	}
	while(i < size)
	{
	ret =  write(updatefp, buf+i, size - i);
	if(ret < 0)
		break;
	i+=ret;
	}
	return 0;
}

static int cb_on_upgrade_end( )
{
	count = 0;
	now_size = 0;
	close(updatefp);
	if(percent_size_flag == 100)
	{	
		sw_monitor_on_event(STB_EVENT_RPC, STB_RPC_UPGRADE,(uint32_t)0, (uint32_t)0 );
	}
	percent_size_flag = 0;
	return -1;
}


static int cb_on_upgrade_error( )
{
	HWNMPD_LOG_DEBUG("========upgrade failed=======\n");
	count = 0;
	now_size = 0;
	percent_size_flag = 0;
	sw_monitor_on_event(STB_EVENT_RPC, STB_RPC_UPGRADEFAIL,(uint32_t)0, (uint32_t)0 );
	return -1;
}

static int cb_on_ping(char* result,char* srmping )
{
	return 0;
}

static int cb_on_traceroute(char* result,char*hostip )
{
	return 0;	
}

static int cb_on_startDebugInfo(char* buf)
{
	sw_android_property_set("IPTV_set_StartDebugInfo", buf);
#ifdef SUPPORT_IPTVMW_DFX
	sw_android_property_get_int("IPTV_get_StartDebugInfo");
#else
	sw_monitor_on_event(STB_EVENT_MEDIA, STB_DFXEVENT_START, 0, 0);
#endif
	return 1;
}

static int cb_on_stopDebugInfo()
{
#ifdef SUPPORT_IPTVMW_DFX
	sw_android_property_get_int("IPTV_get_StopDebugInfo");
#else
	sw_monitor_on_event(STB_EVENT_MEDIA, STB_DFXEVENT_STOP, 0, 0);
#endif
	return 1;
}

static int cb_on_uploadDebugInfo(char* buf)
{
	sw_android_property_set("IPTV_set_UploadDebugInfo", buf);
#ifdef SUPPORT_IPTVMW_DFX
	sw_android_property_get_int("IPTV_get_UploadDebugInfo");
#else
	sw_monitor_on_event(STB_EVENT_MEDIA, STB_DFXEVENT_UPLOAD, 0, 0);
#endif
	return 1;
}

static int cb_on_startBootDebugInfo(char *buf)
{
#ifdef SUPPORT_IPTVMW_DFX
	HWNMPD_LOG_DEBUG("----------fun:%s----buf:%s----\n", __FUNCTION__,buf);
	sw_android_property_set("StartupCapturedSize", buf);
	sw_android_property_set("StartupCaptured", "3");
#else
	sw_android_property_set("IPTV_set_StartDebugInfo", buf);
	sw_monitor_on_event(STB_EVENT_MEDIA, STB_BOOTEVENT_START, 0, 0);
#endif
	return 1;
}

static int cb_on_stopBootDebugInfo()
{
#ifdef SUPPORT_IPTVMW_DFX
	HWNMPD_LOG_DEBUG("-------fun:%s--------line:%d---\n", __FUNCTION__,__LINE__);
	sw_android_property_get_int("IPTV_get_StopBootInfo");
#else
	sw_monitor_on_event(STB_EVENT_MEDIA, STB_BOOTEVENT_STOP, 0, 0);
#endif
	return 1;
}
static void  cb_on_closesFtpUpload()
{
	sw_close_sFtpUpload();
}
static int cb_on_uploadBootDebugInfo(char* buf)
{
	sw_android_property_set("IPTV_set_UploadDebugInfo", buf);
#ifdef SUPPORT_IPTVMW_DFX
	HWNMPD_LOG_DEBUG("------------fun:%s------buf:%s----\n", __FUNCTION__,"sensitive print");
	sw_android_property_get_int("IPTV_get_UploadBootInfo");
#else
	sw_monitor_on_event(STB_EVENT_MEDIA, STB_BOOTEVENT_UPLOAD, 0, 0);
#endif
	return 1;
}

static void hw_stbmonitor_add_callback_function()
{
	if( m_callback_function_is_add == true )
		return;
	sw_memset( &m_scbfs, sizeof(m_scbfs), 0, sizeof(m_scbfs) );
	sw_memset( &m_upgrade_file, sizeof(m_upgrade_file), 0, sizeof(m_upgrade_file) );
	m_scbfs.fcb_on_read_parameter = cb_on_read_parameter;
	m_scbfs.fcb_on_write_parameter = cb_on_write_parameter;
	m_scbfs.fcb_on_read_channellist = cb_on_read_channellist;
	m_scbfs.fcb_on_read_all_parameter = cb_on_read_all_parameter;
	m_scbfs.fcb_on_restore_setting = cb_on_restore_setting;
	
	m_scbfs.fcb_on_upgrade_data = cb_on_upgrade_data;
	m_scbfs.fcb_on_upgrade_error= cb_on_upgrade_error;
	m_scbfs.fcb_on_upgrade_end = cb_on_upgrade_end;
	m_scbfs.fcb_on_upgrade_online = cb_on_upgrade_online;
	
	m_scbfs.fcb_on_stb_reboot = cb_on_stb_reboot;
	m_scbfs.fcb_on_stb_close_remote_connect = cb_on_stb_close_remote_connect;

	m_scbfs.fcb_on_set_test_mode = cb_on_set_test_mode;
	m_scbfs.fcb_on_set_autotest_mode = cb_on_set_autotest_mode;
	m_scbfs.fcb_on_set_work_mode = cb_on_set_work_mode;
	m_scbfs.fcb_on_set_scriptrecord_mode = cb_on_set_scriptrecord_mode;
	
	m_scbfs.fcb_on_media_play = cb_on_media_play;
	m_scbfs.fcb_on_media_stop = cb_on_media_stop;
	m_scbfs.fcb_on_media_ffwd = cb_on_media_ffwd;
	m_scbfs.fcb_on_media_fbwd = cb_on_media_fbwd;
	m_scbfs.fcb_on_media_pause = cb_on_media_pause;
	
	m_scbfs.fcb_on_set_log_out_type = cb_on_set_log_out_type;
	m_scbfs.fcb_on_set_log_level = cb_on_set_log_level;
	m_scbfs.fcb_on_set_log_type = cb_on_set_log_type;

	m_scbfs.fcb_on_ping = cb_on_ping;
	m_scbfs.fcb_on_traceroute = cb_on_traceroute;
	m_scbfs.fcb_on_startDebugInfo = cb_on_startDebugInfo;
	m_scbfs.fcb_on_stopDebugInfo = cb_on_stopDebugInfo;
	m_scbfs.fcb_on_uploadDebugInfo = cb_on_uploadDebugInfo;

	m_scbfs.fcb_on_startBootDebugInfo = cb_on_startBootDebugInfo;
	m_scbfs.fcb_on_stopBootDebugInfo = cb_on_stopBootDebugInfo;
	m_scbfs.fcb_on_uploadBootDebugInfo = cb_on_uploadBootDebugInfo;
	m_scbfs.fcb_on_closesFtpUpload    = cb_on_closesFtpUpload;

	m_callback_function_is_add = true; 
}

static bool m_telnet_server_enable = false;
static bool m_stbmonitor_server_init = false;

bool hw_telnet_is_enable()
{
	return m_telnet_server_enable;
}

void hw_telnet_enable(bool val)
{
	m_telnet_server_enable = val;
	
	if(val == true)
		sw_monitor_on_event(STB_EVENT_RPC, STB_RPC_OPENADB, 0, 0);
	else
		sw_monitor_on_event(STB_EVENT_RPC, STB_RPC_CLOSEADB, 0, 0);
}


void hw_stbmonitor_enable(bool val)
{
	m_stbmonitor_server_init = val;
	
	if(val == true)
		hw_nmp_init();
	else
		hw_nmp_exit();
}



void hw_nmp_init()
{
	char manager_pswd[32];

#ifndef SUPPORT_MONITOR_CHECKCODE
	sw_android_property_get("manager_password",manager_pswd,sizeof(manager_pswd));
	if(strcmp(manager_pswd,"28780808") == 0)
	{
		sw_parameter_set("manager_password",".287aW");
		sw_parameter_save();
	}
#endif
	hw_stbmonitor_add_callback_function();
	sw_stbmonitor_server_init(&m_scbfs);
}

/* ?ر????ܷ??? */
void hw_nmp_exit()
{
	sw_stbmonitor_server_exit();
}

/*当需要机顶盒主动连接pc时，调用此函数,需要pc工具支持监听9003端口*/
int  hw_nmp_connect( char *server_ip )
{
	int connectret = -1;
	if(server_ip)
	{
		if(sw_stbmonitor_session_is_create())
		{
			HWNMPD_LOG_DEBUG("The session had been created, please use STBMonitor to connect to STB...\n");
			//PC端和盒子之前已经建立了tcp连接，再次点击设置页面的connect，如果PC端ip没变，返回0连接成功，否则返回-1连接失败
			if( strcmp(server_ip, last_server_ip) == 0 )
				return 0;
			else
				return -1;
		}
		sw_strlcpy(last_server_ip, sizeof(last_server_ip), server_ip, sizeof(last_server_ip));
	}
	hw_stbmonitor_add_callback_function();
	connectret = sw_stbmonitor_connect( &m_scbfs, server_ip, (char *)sw_network_get_currentip());
	HWNMPD_LOG_DEBUG("===============%d,%d\n",__LINE__,connectret);
	return connectret;
}

extern char **environ;
static int SW_Exec(const char *command)
{
	/* 从SDK中copy过来，禁止多条命令一起调用管道调用，非系统命令调用 */
	pid_t pid;
	sig_t intsave, quitsave;
	sigset_t mask, omask;
	int pstat;
	char *argp[] = {"sh", "-c", NULL, NULL};

	if (!command || strchr(command, ';') != NULL || strstr(command, "&&") != NULL || strchr(command, '|') != NULL)		/* just checking... */
		return(1);
	if (strncmp(command, "busybox pkill -9", 16) != 0 && strncmp(command, "/system/bin/", 12) != 0 && strncmp(command, "/usr/bin/", 9) != 0)
		return (1);
	argp[2] = (char *)command;

	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &mask, &omask);
	switch (pid = vfork()) {
		case -1:			/* error */
			sigprocmask(SIG_SETMASK, &omask, NULL);
			return(-1);
		case 0:				/* child */
			sigprocmask(SIG_SETMASK, &omask, NULL);
			execve(_PATH_BSHELL, argp, environ);
			_exit(127);
	}

	intsave = (sig_t)  bsd_signal(SIGINT, SIG_IGN);
	quitsave = (sig_t) bsd_signal(SIGQUIT, SIG_IGN);
	pid = waitpid(pid, (int *)&pstat, 0);
	sigprocmask(SIG_SETMASK, &omask, NULL);
	(void)bsd_signal(SIGINT, intsave);
	(void)bsd_signal(SIGQUIT, quitsave);
	return (pid == -1 ? -1 : pstat);
}
