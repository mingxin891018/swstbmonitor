#include "swapi.h"
#include "swhashmap.h"
#include "string_ext.h"
#include "swparameter.h"
#include "swparamsafe.h"
#include "swstbmonitorserver.h"
#include "hwnmpd_priv.h"
#include "hwpara.h"
#include "swplayerdef.h"
#include "swbrowser.h"
#include "swapp.h"
#include "swnetwork.h"
#include "timezone.h"
#include "hwtest.h"
#include "hwlog.h"
#include "swstbmonitorserver.h"
#include "hwnmpd_priv.h"
#include "swtxtparser.h"

#define STBMONITOR_HASH_NUM			(400)
#define STBMONITOR_READ				0
#define STBMONITOR_WRITE			1


//extern int handle_LogOutPutType(const char *name, char *value, int size, int act_type);
//extern int handle_LogServer(const char *name, char *value, int size, int act_type);
static int handle_LogFtpServer(const char *name, char *value, int size, int act_type);


static char old_netmode[65] = {0};
static char current_netmode[65] = {0};

static swhashmap_t* m_hashmap = NULL;

extern bool sw_get_android_way_to_set(char* name, char* value);
extern int property_get(const char *key, char *value, const char *default_value);

static int sw_stbmonitor_parameter_init();
static void sw_stbmonitor_parameter_hash_init();
static void sw_stbmonitor_add_parameter(char *name, stbmonitor_handle fun);
//static int stbmonitor_LogType(const char *name, char *value, int size, int act_type);
static int stbmonitor_dhcpaccount(const char *name, char *value, int size, int act_type);
static int stbmonitor_dhcppassword(const char *name, char *value, int size, int act_type);
static int handle_systemTypeLA_para(const char *name,char *value, int size, int act_type);
//static int stbmonitor_LogLevel(const char *name, char *value, int size, int act_type);
static int handle_common_para(const char *name, char *value, int size, int act_type );
static int handle_ntvuserpassword_para(const char *name, char *value, int size, int act_type ); 
static int handle_connecttype_para(const char *name, char *value, int size, int act_type ); 
static int handle_timeZone_para(const char *name, char *value, int size, int act_type ); 
static int stbmonitor_directplay(const char *name,char *value,int size,int act_type);
static int handle_localTime_para(const char *name, char *value, int size, int act_type ); 
static int handle_EPGRatio_para(const char *name, char *value, int size, int act_type );
static int handle_hw_op_CAregister_para(const char *name, char *value, int size, int act_type );
static int handle_CASType_para(const char *name, char *value, int size, int act_type );
static int handle_qosreg_para(const char *name,char *value,int size,int act_type);
static int handle_log_upload_interval(const char *name,char *value,int size,int act_type);
static int handle_SupportHD_para(const char *name, char *value, int size, int act_type ); 
static int stbmonitor_antiFlickerSwitch(const char *name,char *value,int size,int act_type);
static int handle_AspectRatio_para(const char *name, char *value, int size, int act_type ); 
static int stbmonitor_TMSHeartBit(const char *name,char *value,int size,int act_type);
static int stbmonitor_SDVideoStandard(const char *name, char *value, int size, int act_type ); 
static int stbmonitor_HDVideoStandard(const char *name, char *value, int size, int act_type ); 
static int handle_SoftwareVersion_para(const char *name, char *value, int size, int act_type ); 
static int handle_SoftwareHWversion_para(const char *name, char *value, int size, int act_type ); 
static int handle_CompTime_para(const char *name, char *value, int size, int act_type ); 
static int handle_BrowserVersion_para(const char *name, char *value, int size, int act_type );
static int handle_BrowserTime_para(const char *name, char *value, int size, int act_type );
static int handle_IPAddress_para(const char *name, char *value, int size, int act_type );
static int handle_workModel(const char *name, char *value, int size, int act_type ); 
static int handle_channelSwitchMode_para(const char *name, char *value, int size, int act_type ); 
static int handle_pcap_dat_para(const char *name, char *value, int size, int act_type ); 
static int handle_telnetServiceEnable_para(const char *name, char *value, int size, int act_type );
static int handle_allDebugInfo_para(const char *name, char *value, int size, int act_type );
static int handle_serialPortServiceEnable_para(const char *name, char *value, int size, int act_type ); 			
static int handle_stbIP_para(const char *name, char *value, int size, int act_type);
static int handle_LogServer(const char *name, char *value, int size, int act_type);
static int handle_transport_protocol(const char *name, char *value, int size, int act_type);
static int handle_log_server(const char *name, char *value, int size, int act_type);
static int stbmonitor_ChipId(const char *name,char *value, int size, int act_type);
static int handle_tvmsvodheartbiturl_para(const char *name,char *value, int size, int act_type);;
static int handle_tvmsheartbiturl_para(const char *name,char *value, int size, int act_type);;
static int handle_Stbidnum_para(const char *name,char *value, int size, int act_type);
static int handle_netuserpassword_para(const char *name,char *value, int size, int act_type);
static int handle_netuseraccount_para(const char *name,char *value, int size, int act_type);
static int handle_ParasListMain(const char *name,char *value, int size, int act_type);
static int handle_ParasListPip(const char *name,char *value, int size, int act_type);
static int handle_mainhomepageurl_para(const char *name, char *value, int size, int act_type );
static int handle_sechomepageurl_para(const char *name, char *value, int size, int act_type );
static int handle_ntvuseraccount_para(const char *name, char *value, int size, int act_type );
static int handle_epgurl_para(const char *name, char *value, int size, int act_type );

#ifdef SUPPORT_HWSTBMONITOR_VER4
static int handle_ramSize_para(const char *name,char *value, int size, int act_type);
static int handle_flashSize_para(const char *name,char *value, int size, int act_type);
static int handle_appVersion_para(const char *name,char *value, int size, int act_type);
static int handle_devicesModel_para(const char *name,char *value, int size, int act_type);
static int handle_casURL_para(const char *name,char *value, int size, int act_type);
static int handle_sqmURL_para(const char *name,char *value, int size, int act_type);
#endif

int sw_stbmonitor_hwparameter_size(void)
{
	return sw_hashmap_size( m_hashmap );
}

int sw_stbmonitor_hwparameter_read_byindex(int index, char **key, char *buf, int length)
{
	int hashsize = sw_hashmap_size(m_hashmap);
	int ret = -1;
	stbmonitor_handle fun;
	if ( index < hashsize )
	{
		if (sw_hashmap_get_byindex(m_hashmap, index, (void**)key, (void*) &fun) == 0 )
		{
			ret = fun(*key, buf, length, STBMONITOR_READ);
		}
	}
	return ret;
}

int sw_stbmonitor_hwparameter_read(const char *params, char *buf, int length)
{
	stbmonitor_handle fun;
	int ret = -1;
	if ( params == NULL || buf == NULL || length <= 0 )
	{
		if ( buf != NULL && length > 0 )
			*buf = '\0';
		return -1;
	} 
	sw_stbmonitor_parameter_init();
	if( sw_hashmap_get(m_hashmap, (char*)params, (void*) &fun) == 0 )
	{
		sw_memset(buf, length, 0, length);
		ret = fun(params, buf, length, STBMONITOR_READ);
		HWNMPD_LOG_DEBUG("params:%s\n", params);
	}
	else
	{
		*buf = '\0';
		/* 一般参数,parameter会清空数据的 */
		sw_memset(buf, length, 0, length);
		if(!strcmp(params, "LogType"))
			ret = sw_parameter_get((char *)params, buf,length);
		else if(!strcmp(params, "LogOutPutType"))
			ret = sw_parameter_get((char *)params, buf,length);
		else if(!strcmp(params, "LogLevel")) 
			ret = sw_parameter_get((char *)params, buf,length);
		else 
			ret = sw_android_property_get((char *)params, buf,length);

		if(ret)//上述接口执行成返回true，本接口执行成功需要返回0，否则，pc端认为执行失败
			ret = 0;
		HWNMPD_LOG_DEBUG("params:%s,buf:%s\n", params, buf);
	}
	return ret;
}

int sw_stbmonitor_hwparameter_write(const char *params, char *buf, int length)
{
	int ret = -2;
	stbmonitor_handle fun;
	if ( params == NULL || buf == NULL || length <= 0 )
		return -1;
	sw_stbmonitor_parameter_init();
	if( sw_hashmap_get(m_hashmap, (char*)params, (void*) &fun) == 0 )
	{
		HWNMPD_LOG_DEBUG("params:%s\n", params);
		ret = fun(params, buf, length, STBMONITOR_WRITE);
	}
	else
	{
		HWNMPD_LOG_DEBUG("Not support:%s\n", params);
		sw_parameter_set((char *)params, buf);
		sw_parameter_save();
		return 0;
	}
	return ret;
}


//当管理工具和stb长时间没有交互而断开连接的时候，把日志上传功能也停下来
void sw_close_sFtpUpload()
{
	sw_android_property_set("LogServer", "ClosesFtpUpload");
}

static void sw_stbmonitor_add_parameter(char *name, stbmonitor_handle fun)
{
	if( m_hashmap)
		sw_hashmap_put(m_hashmap,(void*)name,fun);
}

static int sw_stbmonitor_parameter_init()
{
	if (m_hashmap != NULL)
		return 0;
	/** create hashmap */
	m_hashmap =  sw_hashmap_create(STBMONITOR_HASH_NUM, KEY_STRING );
	if(m_hashmap)
	{
		HWNMPD_LOG_DEBUG("Creat hashmap %d members Succeed!---------\n",STBMONITOR_HASH_NUM);
		sw_stbmonitor_parameter_hash_init();
		return 0;
	}
	else
	{
		HWNMPD_LOG_FATAL("Creat hashmap Failed!-----------\n");
		return -1;
	}
	return -1;
}


static int handle_LogServer(const char *name, char *value, int size, int act_type)
{

    HWNMPD_LOG_DEBUG("#########################name:%s\n",name);
	if ( act_type == 0 )
	{
		//sw_android_property_get("LogServer", value, size);
		//sw_parameter_get("LogServer",  value, size);
		sw_parameter_safe_get("LogServer",  value, size);
		return 0;
	}
	else
	{
		if ( strlen(value) >= 128 )
			return -1;
		
		//sw_parameter_set("LogServer", value);
		sw_parameter_safe_set("LogServer", value);
		sw_android_property_set("LogServer", value);//把地址发给IPTV进程，让IPTV进程把日志通过UDP输出到PC
		return 0;
	}
	return -1;
}

static int handle_transport_protocol(const char *name, char *value, int size, int act_type)
{

    HWNMPD_LOG_DEBUG("#########################name:%s,value:%s\n",name,value);
	if ( act_type == 0 )
	{
		sw_parameter_get("transport_protocol", value, size);
		HWNMPD_LOG_DEBUG("--------transport_protocol=%s--------\n",value);
		return 0;
	}
	else
	{
		sw_parameter_set("transport_protocol", value);
		HWNMPD_LOG_DEBUG("-------transport_protocol=%s---------\n",value);
		sw_parameter_save();
		return 0;
	}
	return -1;
}

static int stbmonitor_dns(const char *name,char *value,int size,int act_type)
{
	char dnsmode[65] = {0};
	if ( act_type == STBMONITOR_READ )
	{
		sw_strlcpy( dnsmode, sizeof(dnsmode), sw_network_get_currentmode(), sizeof(dnsmode) );
		if( strcmp(dnsmode,"static") == 0 || strcmp(dnsmode,"dhcp")==0)
		{
			sw_android_property_get("lan_dns", value, size > 32 ? 32 : size);
		}
		else if( strcmp(dnsmode,"pppoe")==0)
		{
			sw_android_property_get("pppoe_dns", value, size > 32 ? 32 : size);
		}
		else if( strcmp(dnsmode,"wifi_static") == 0 || strcmp(dnsmode,"wifi_dhcp")==0)
		{
			sw_android_property_get("wifi_dns", value, size > 32 ? 32 : size);
		}
	}
	else
	{
		if ( sw_txtparser_is_address(value) )
		{
			if(strcasecmp(current_netmode, old_netmode))
				sw_get_android_way_to_set("lan_dns", value);
			else
				sw_android_property_set("lan_dns", value);
		}
	}
	return 0;
}

static int stbmonitor_gateway(const char *name,char *value,int size,int act_type)
{
	char gatewaymode[65] = {0};
	if ( act_type == STBMONITOR_READ )
	{	
		sw_strlcpy( gatewaymode, sizeof(gatewaymode), sw_network_get_currentmode(), sizeof(gatewaymode) );
		HWNMPD_LOG_DEBUG("#########################name:%s,gatewaymode:%s\n",name,gatewaymode);
		if( strcmp(gatewaymode,"static") == 0 || strcmp(gatewaymode,"dhcp")==0)
		{
			sw_android_property_get("lan_gateway", value, size > 32 ? 32 : size);
		}
		else if( strcmp(gatewaymode,"pppoe")==0)
		{
			sw_android_property_get("pppoe_gateway", value, size > 32 ? 32 : size);
		}
		else if( strcmp(gatewaymode,"wifi_static") == 0 || strcmp(gatewaymode,"wifi_dhcp")==0)
		{
			sw_android_property_get("wifi_gateway", value, size > 32 ? 32 : size);
		}
	}
	else
	{
		if ( sw_txtparser_is_address(value) )
		{
			if(strcasecmp(current_netmode, old_netmode))
				sw_get_android_way_to_set("lan_gateway", value);
			else
				sw_android_property_set("lan_gateway", value);
		}
	}
	return 0;
}

static int stbmonitor_netmask(const char *name,char *value,int size,int act_type)
{
	char netmaskmode[65] = {0};
	if ( act_type == STBMONITOR_READ )
	{		
		sw_strlcpy( netmaskmode, sizeof(netmaskmode), sw_network_get_currentmode(), sizeof(netmaskmode) );
		if( strcmp(netmaskmode,"static") == 0 || strcmp(netmaskmode,"dhcp")==0)
		{
			sw_android_property_get("lan_mask", value, size > 32 ? 32 : size);
		}
		else if( strcmp(netmaskmode,"pppoe")==0)
		{
			sw_android_property_get("pppoe_mask", value, size > 32 ? 32 : size);
		}
		else if( strcmp(netmaskmode,"wifi_static") == 0 || strcmp(netmaskmode,"wifi_dhcp")==0)
		{
			sw_android_property_get("wifi_mask", value, size > 32 ? 32 : size);
		}
	}
	else
	{
		if ( sw_txtparser_is_address(value) )
		{
			if(strcasecmp(current_netmode, old_netmode))
				sw_get_android_way_to_set("lan_mask", value);
			else
				sw_android_property_set("lan_mask", value);
		}
	}
	return 0;	
}



static void sw_stbmonitor_parameter_hash_init()
{
	sw_stbmonitor_add_parameter("Main_HomepageUrl",			handle_mainhomepageurl_para);
	sw_stbmonitor_add_parameter("Secondary_HomepageUrl",	handle_sechomepageurl_para);
	sw_stbmonitor_add_parameter("NTPDomain",				handle_common_para);
#ifdef SUPPORT_HWSTBMONITOR_VER4
	sw_stbmonitor_add_parameter("bakNTPDomain",			    handle_common_para);
#else
	sw_stbmonitor_add_parameter("NTPDomainBackup",			handle_common_para);
#endif
	sw_stbmonitor_add_parameter("netuseraccount",			handle_netuseraccount_para);
	sw_stbmonitor_add_parameter("netuserpassword",			handle_netuserpassword_para);
	sw_stbmonitor_add_parameter("ntvuseraccount",			handle_ntvuseraccount_para);
	sw_stbmonitor_add_parameter("ntvuserpassword",			handle_ntvuserpassword_para);
	sw_stbmonitor_add_parameter("connecttype",				handle_connecttype_para);
	sw_stbmonitor_add_parameter("stbIP",					handle_stbIP_para);
	sw_stbmonitor_add_parameter("netmask",					stbmonitor_netmask);
	sw_stbmonitor_add_parameter("gateway",					stbmonitor_gateway);
	sw_stbmonitor_add_parameter("dns",						stbmonitor_dns);
	sw_stbmonitor_add_parameter("dns2",						handle_common_para);
	sw_stbmonitor_add_parameter("defContAcc",				handle_common_para);
	sw_stbmonitor_add_parameter("defNetUserPassword",		handle_common_para);
	sw_stbmonitor_add_parameter("directplay",				stbmonitor_directplay);
	sw_stbmonitor_add_parameter("epgurl",					handle_epgurl_para);
	sw_stbmonitor_add_parameter("timeZone",					handle_timeZone_para);
	sw_stbmonitor_add_parameter("localTime",				handle_localTime_para);
	sw_stbmonitor_add_parameter("EPGRatio",					handle_EPGRatio_para);
	sw_stbmonitor_add_parameter("hw_op_CAregister",			handle_hw_op_CAregister_para);
	sw_stbmonitor_add_parameter("CASType",					handle_CASType_para);
	sw_stbmonitor_add_parameter("QoSLogSwitch",				handle_qosreg_para);
	sw_stbmonitor_add_parameter("LogUploadInterval",		handle_log_upload_interval);
	sw_stbmonitor_add_parameter("LogServerUrl",				handle_log_server);
	sw_stbmonitor_add_parameter("LogRecordInterval",		handle_common_para);
	sw_stbmonitor_add_parameter("TVMSGWIP",					handle_common_para);
	sw_stbmonitor_add_parameter("TVMSHeartbitInterval",		handle_common_para);
	sw_stbmonitor_add_parameter("TVMSDelayLength",			handle_common_para);
	sw_stbmonitor_add_parameter("TVMSHeartbitUrl",			handle_tvmsheartbiturl_para);
	sw_stbmonitor_add_parameter("TVMSVODHeartbitUrl",		handle_tvmsvodheartbiturl_para);
	sw_stbmonitor_add_parameter("templateName",				handle_common_para);
	sw_stbmonitor_add_parameter("areaid",					handle_common_para);
	sw_stbmonitor_add_parameter("ManagementDomain",			handle_common_para);
	sw_stbmonitor_add_parameter("TMSEnable",				handle_common_para);
	sw_stbmonitor_add_parameter("TMSUsername",				handle_common_para);
	sw_stbmonitor_add_parameter("TMSPassword",				handle_common_para);
	sw_stbmonitor_add_parameter("TMSHeartBit",				stbmonitor_TMSHeartBit);
	sw_stbmonitor_add_parameter("TMSHeartBitInterval",		handle_common_para);
	sw_stbmonitor_add_parameter("SupportHD",				handle_SupportHD_para);
	sw_stbmonitor_add_parameter("AspectRatio",				handle_AspectRatio_para);
	sw_stbmonitor_add_parameter("SDVideoStandard",			stbmonitor_SDVideoStandard);
	sw_stbmonitor_add_parameter("HDVideoStandard",			stbmonitor_HDVideoStandard);
	sw_stbmonitor_add_parameter("STB_model",				handle_devicesModel_para);
	sw_stbmonitor_add_parameter("SoftwareVersion",			handle_SoftwareVersion_para);
	sw_stbmonitor_add_parameter("SoftwareHWversion",		handle_SoftwareHWversion_para);
	sw_stbmonitor_add_parameter("CompTime",					handle_CompTime_para);
	sw_stbmonitor_add_parameter("HardWareVersion",          handle_common_para);
	sw_stbmonitor_add_parameter("BrowserVersion",			handle_BrowserVersion_para);
	sw_stbmonitor_add_parameter("BrowserTime",				handle_BrowserTime_para);
	sw_stbmonitor_add_parameter("STBIDNUM",			handle_Stbidnum_para);
	sw_stbmonitor_add_parameter("MACAddress",				handle_common_para);
	sw_stbmonitor_add_parameter("IPAddress",				handle_IPAddress_para);
	sw_stbmonitor_add_parameter("UpgradeServer",			handle_common_para);
	sw_stbmonitor_add_parameter("bakUpgradeServer",			handle_common_para);
//	sw_stbmonitor_add_parameter("LogLevel",					stbmonitor_LogLevel);
//	sw_stbmonitor_add_parameter("LogType",					stbmonitor_LogType);
	sw_stbmonitor_add_parameter("LogFtpServer",				handle_LogFtpServer);
	sw_stbmonitor_add_parameter("LogServer",				handle_LogServer);
//	sw_stbmonitor_add_parameter("LogOutPutType",			handle_LogOutPutType);
	sw_stbmonitor_add_parameter("workModel",				handle_workModel);
	sw_stbmonitor_add_parameter("channelSwitchMode",		handle_channelSwitchMode_para);
	sw_stbmonitor_add_parameter("watchDogSwitch",			handle_common_para);
	sw_stbmonitor_add_parameter("ErrorLogToFlashEnable",	handle_common_para);
	sw_stbmonitor_add_parameter("ANRLogToFlashEnable",		handle_common_para);
	sw_stbmonitor_add_parameter("PerfKPILogToFlashEnable",  handle_common_para);
	sw_stbmonitor_add_parameter("ErrorCodeLogToFlashEnable",handle_common_para);
	sw_stbmonitor_add_parameter("StartupErrorLogToFlash",   handle_common_para);
	sw_stbmonitor_add_parameter("TimeIntervalForLogToFlash",handle_common_para);
	sw_stbmonitor_add_parameter("antiFlickerSwitch",		stbmonitor_antiFlickerSwitch);
	sw_stbmonitor_add_parameter("TransportProtocol",		handle_transport_protocol);
	sw_stbmonitor_add_parameter("pcap.dat",					handle_pcap_dat_para);
	sw_stbmonitor_add_parameter("telnetServiceEnable",		handle_telnetServiceEnable_para);
	sw_stbmonitor_add_parameter("allDebugInfo",				handle_allDebugInfo_para);
	sw_stbmonitor_add_parameter("serialPortServiceEnable",	handle_serialPortServiceEnable_para);
#ifdef SUPPORT_HWSTBMONITOR_VER4
	sw_stbmonitor_add_parameter("socType", stbmonitor_ChipId);
#else
	sw_stbmonitor_add_parameter("ChipId", stbmonitor_ChipId);
#endif
	sw_stbmonitor_add_parameter("dhcpaccount", stbmonitor_dhcpaccount);
	sw_stbmonitor_add_parameter("dhcppassword", stbmonitor_dhcppassword);
	sw_stbmonitor_add_parameter("systemTypeLA",	handle_systemTypeLA_para);
#ifdef SUPPORT_HWSTBMONITOR_VER4
	sw_stbmonitor_add_parameter("flashSize",	handle_flashSize_para);
	sw_stbmonitor_add_parameter("ramSize",	    handle_ramSize_para);
	sw_stbmonitor_add_parameter("appVersion",	handle_appVersion_para);
	sw_stbmonitor_add_parameter("sqmURL",	    handle_sqmURL_para);
	sw_stbmonitor_add_parameter("casURL",	    handle_casURL_para);
#endif
	sw_stbmonitor_add_parameter("ParasListMain",    handle_ParasListMain);
	sw_stbmonitor_add_parameter("ParasListPip", handle_ParasListPip);
}

static int handle_ParasListMain(const char *name,char *value, int size, int act_type)
{
	if(act_type == STBMONITOR_READ)
	{
		sw_android_property_get("IPTV_ParasListMain", value, size);
		return 0;
	}
	return -1;
}

static int handle_ParasListPip(const char *name,char *value, int size, int act_type)
{
	if(act_type == STBMONITOR_READ)
	{
		sw_android_property_get("IPTV_ParasListPip", value, size);
		return 0;
	}
	return -1;
}

#ifdef SUPPORT_HWSTBMONITOR_VER4
static int handle_sqmURL_para(const char *name,char *value, int size, int act_type)
{
	if( act_type == STBMONITOR_READ )
	{
		sw_android_property_get("sqm_server_ip",value,size);
		return 0;
	}
	else
		sw_android_property_set("sqm_server_ip",value);
	return -1;
	
}

static int handle_casURL_para(const char *name,char *value, int size, int act_type)
{
	char default_mode[10];
	sw_android_property_get("default_iptv_ott",default_mode,10);
	if( act_type == STBMONITOR_READ )
	{
		if(strcmp(default_mode,"iptv") == 0)
			sw_parameter_get("drm_serverip_verimatrix",value,size);
		else if(strcmp(default_mode,"ott") == 0)
			sw_parameter_get("ott_verimatrix_serverip",value,size);
		else
			return -1;
		return 0;
	}
	return -1;
	
}

static int handle_appVersion_para(const char *name,char *value, int size, int act_type)
{
	if( act_type == STBMONITOR_READ )
	{
		property_get("ro.build.version.incremental",value,"");
		return 0;
	}
	return -1;
	
}

static int handle_flashSize_para(const char *name,char *value, int size, int act_type)
{
	if( act_type == STBMONITOR_READ )
	{
		property_get("ro.flash.size",value,"4GB");
		return 0;
	}
	return -1;
	
}
static int handle_ramSize_para(const char *name,char *value, int size, int act_type)
{
	if( act_type == STBMONITOR_READ )
	{
		property_get("ro.memory.size",value,"1GB");
		return 0;
	}
	return -1;

}
#endif


static int handle_devicesModel_para(const char *name,char *value, int size, int act_type)
{
	if( act_type == STBMONITOR_READ )
	{
		property_get("ro.build.devicemodel",value,"");
		return 0;
	}
	return -1;

}
static int handle_Stbidnum_para(const char *name,char *value, int size, int act_type)
{
	if( act_type == STBMONITOR_READ )
	{
		sw_android_property_get("serial", value, size);
		return 0;
	}
	return -1;

}

static int handle_tvmsvodheartbiturl_para(const char *name,char *value, int size, int act_type)
{
	if( act_type == STBMONITOR_READ )
	{
		sw_android_property_get("tvmsg_vodmsg_url",value,size);
	}
	else
		sw_android_property_set("tvmsg_vodurl_url",value);
	return 0;

}

static int handle_tvmsheartbiturl_para(const char *name,char *value, int size, int act_type)
{
	if( act_type == STBMONITOR_READ )
	{
		sw_android_property_get("tvmsg_heartbeat_url",value,size);
	}
	else
		sw_android_property_set("tvmsg_heartbeat_url",value);
	return 0;

}

static int handle_netuseraccount_para(const char *name,char *value, int size, int act_type)
{
	if(act_type == STBMONITOR_READ )
		sw_android_property_get("pppoe_user_name",value,size);
	else
		sw_android_property_set("pppoe_user_name",value);
	return 0;
}

static int handle_netuserpassword_para(const char *name,char *value, int size, int act_type)
{
	if(act_type == STBMONITOR_READ )
		return -1;
	else
		sw_android_property_set("pppoe_user_pass",value);
	return 0;
}

static int handle_systemTypeLA_para(const char *name,char *value, int size, int act_type)
{
	if ( act_type == STBMONITOR_READ )
	{
		sw_strlcpy(value, size, "1", size);
	}	
	else
	{
		return -1;
	}
	return 0;
	
}

static int stbmonitor_dhcpaccount(const char *name, char *value, int size, int act_type)
{
	if(act_type == 0)
	{
		sw_android_property_get("dhcpaccount", value, size);
	}
	else
	{
		sw_android_property_set("dhcpaccount", value);
	}
	return 0;
}

static int stbmonitor_dhcppassword(const char *name, char *value, int size, int act_type)
{
	if(act_type == 0)
	{
		sw_android_property_get("dhcppassword", value, size);
	}
	else
	{
		sw_android_property_set("dhcppassword", value);
	}
	return 0;
}

#if 0
static int stbmonitor_LogLevel(const char *name, char *value, int size, int act_type)
{
//	return handle_LogLevel(name, value, size, act_type == STBMONITOR_READ ? 0 : 1);
}

static int stbmonitor_LogType(const char *name, char *value, int size, int act_type)
{
//	return handle_LogType(name, value, size, act_type == STBMONITOR_READ ? 0 : 1);
}


static int handle_LogOutPutType(const char *name, char *value, int size, int act_type)
{
	HWNMPD_LOG_DEBUG("#########################name:%s,value:%s\n",name,value);
	if ( act_type == 0 )
	{
		if ( !sw_android_property_get("LogOutPutType", value, size) )
		{
			strcpy(value, "00");
		}
	}
	else
	{
        if ( strlen(value) > 12 )
            return 0;
        sw_parameter_set("LogOutPutType", value);

        int logtype = atoi(value);
		HWNMPD_LOG_DEBUG("bianzhonglin +++ %s:%d  logtype = %d \n", __FUNCTION__, __LINE__, logtype);
        if(logtype == 0 || logtype == 2) //关闭ftp日记
  //          hw_log_validate_logftpsever(NULL);
        if(logtype == 0 || logtype == 1)
//            hw_log_validate_logudpserver(NULL); //关闭udp日记

        if(logtype == 0)  //设置logcat服务是否发送日记
            hw_syslog_send_command("stop");
        else
            hw_syslog_send_command("start");
    }
    return 0;
}

#endif
int handle_LogFtpServer(const char *name, char *value, int size, int act_type)
{
	if ( act_type == 0 )
	{
		sw_parameter_get("LogFtpServer", value, size);
	}
	else
	{
		if ( strlen( value ) >= 236 )
			return 0;
//		hw_log_validate_logftpsever(value);
		sw_android_property_set("LogServer", value);
		sw_parameter_set("LogFtpServer", value);
	}
	return 0;
}

static int stbmonitor_TMSHeartBit(const char *name,char *value,int size,int act_type)
{
	if ( act_type == STBMONITOR_READ )
	{
		sw_snprintf(value, size, 0, size, "%d", sw_android_property_get_int("acs_periodinform"));
	}
	else
	{
		sw_parameter_set_int("acs_periodinform", atoi(value));
	}
	return 0;
}

static int handle_workModel(const char *name,char *value,int size,int act_type)
{/* set_test_mode, set_autotest_mode, set_work_mode, set_scriptrecord_mode */
	if ( act_type == 0 )
	{
		//strcpy(value, hw_test_get_workModel());
        sw_strlcpy(value, size, "set_work_mode", size);
	}
	else
	{
		//hw_test_set_workModel( value, false );
	}
	return 0;
}

static int stbmonitor_directplay(const char *name,char *value,int size,int act_type)
{
	if ( act_type == STBMONITOR_READ )
	{
#ifdef SUPPORT_TELKOM_8V9
		sw_snprintf( value, size, 0, size, "%d", 0);
#else
		sw_snprintf( value, size, 0, size, "%d", sw_android_property_get_int("directplay"));
#endif
	}	
	else
	{
		return -1;
	}
	return 0;
}


static int stbmonitor_antiFlickerSwitch(const char *name,char *value,int size,int act_type)
{
	if ( act_type == STBMONITOR_READ )
	{
		int i =  sw_android_property_get_int("anti_flicker");
        i = i < 0 ? 1 : i;
        sw_snprintf(value, size, 0, size, "%d", i);
	}
	else
	{
		sw_parameter_set_int("anti_flicker", atoi(value));
	}
	return 0;
}



static int stbmonitor_SDVideoStandard(const char *name,char *value,int size,int act_type)
{
	//注：依据Q22规格和与SE确定，对于管理工具标清制式不读取不写入，故删除此处代码
	return 0;
}


static int stbmonitor_ChipId(const char *name,char *value,int size,int act_type)
{
	if ( act_type == STBMONITOR_READ )
	{
		property_get("ro.chip.type", value, "");
		return 0;
	}
	return -1;
}


static int handle_stbIP_para(const char *name, char *value, int size, int act_type)
{  
	char netmode[65] = {0};
	if ( act_type == STBMONITOR_READ )
	{
		sw_strlcpy( netmode, sizeof(netmode), sw_network_get_currentmode(), sizeof(netmode) );
		if( strcmp(netmode,"static") == 0 || strcmp(netmode,"dhcp")==0)
		{
			sw_android_property_get("lan_ip", value, size > 32 ? 32 : size);
			HWNMPD_LOG_DEBUG("#########################name:%s,value:%s\n",name,!SENSITIVE_PRINT ? "..." : value);
		}
		else if( strcmp(netmode,"pppoe") == 0 )
		{
			sw_android_property_get("pppoe_ip", value, size > 32 ? 32 : size);
			HWNMPD_LOG_DEBUG("#########################name:%s,value:%s\n",name,!SENSITIVE_PRINT ? "..." : value);
		}
		else if( strcmp(netmode,"wifi_static") == 0 || strcmp(netmode,"wifi_dhcp")==0)
		{
			sw_android_property_get("wifi_ip", value, size > 32 ? 32 : size);
		}
	}
	else
	{
		if ( sw_txtparser_is_address(value) )
		{
			if(strcasecmp(current_netmode, old_netmode))
				sw_get_android_way_to_set("lan_ip", value);
			else
			sw_android_property_set("lan_ip", value);
		}
	}
	return 0;
}

static int handle_common_para(const char *name, char *value, int size, int act_type )
{
	HWNMPD_LOG_DEBUG("#########################name:%s\n",name);
	int pos = -1;
	int i = 0;
	for(i=0; i<sizeof(m_Hwpara_Table)/sizeof(HW_PARAM_DESC); i++)
	{
		if( !xstrcasecmp(m_Hwpara_Table[i].opt, name) )
		{
			pos = i;
			break;
		}
	}
	if(pos == -1)
		return -1;

	if( act_type == STBMONITOR_READ )
	{
		if( (m_Hwpara_Table[pos].bhr != PA_WRITEONLY) )
		{
			sw_android_property_get(m_Hwpara_Table[pos].name, value, size);
			HWNMPD_LOG_DEBUG("#########################name:%s,name2=%s\n",name,m_Hwpara_Table[pos].name);
			return 0;
		}
	}
	else if( act_type == STBMONITOR_WRITE )
	{
		if( (m_Hwpara_Table[pos].bhr != PA_READONLY) )
		{
			HWNMPD_LOG_DEBUG("#########################name:%s,name2=%s\n",name,m_Hwpara_Table[pos].name);
			sw_android_property_set(m_Hwpara_Table[pos].name, value);
			sw_parameter_set(m_Hwpara_Table[pos].name, value);
			sw_parameter_save();
			return 0;
		}
	}
	return -1;
}

static int handle_ntvuserpassword_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_WRITE )
	{
		sw_android_property_set("ntvuserpassword", value);
		return 0;
	}
	else
		return -1;

}

static int handle_ntvuseraccount_para(const char *name, char *value, int size, int act_type )
{

	if ( act_type == STBMONITOR_WRITE )
	{
		sw_android_property_set("ntvuseraccount", value);
		return 0;
	}
	else
	{
		sw_android_property_get("ntvuseraccount",value,size);
		return 0;
	}

	return -1;	
}

static int handle_epgurl_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_WRITE )
	{
		sw_android_property_set("epgurl", value);
		return 0;
	}
	else
	{
		sw_android_property_get("epgurl",value,size);
		return 0;
	}

	return -1;	
}



static int handle_mainhomepageurl_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_WRITE )
	{
		sw_android_property_set("home_page", value);
		return 0;
	}
	else
	{
		sw_android_property_get("home_page",value,size);
		return 0;
	}

	return -1;	
}

static int handle_sechomepageurl_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_WRITE )
	{
		sw_android_property_set("home_page2", value);
		return 0;
	}
	else
	{
		sw_android_property_get("home_page2",value,size);
		return 0;
	}

	return -1;	
}

static void connecttype_read(char *value, int size)
{
	int iConnType = 0;
	char netmode[65] = {0};

	sw_strlcpy( netmode, sizeof(netmode), sw_network_get_currentmode(), sizeof(netmode) );

	if( strcmp(netmode,"static") == 0 || strcmp(netmode,"wifi_static")==0)
	{
		iConnType = 3;
	}
	else if( strcmp(netmode,"dhcp") == 0 || strcmp(netmode,"wifi_dhcp")==0)
	{
		iConnType = 2;
	}
	else if( strcmp(netmode,"pppoe") == 0 || strcmp(netmode,"wifi_pppoe")==0 )
	{
		iConnType = 1;
	}	
	sw_snprintf(value, size, 0, size,"%d", iConnType);
}

static void connecttype_write(char *value, int size)
{
	char netmode[65];
	char sznetmode[65];
	int iConnType = atoi(value);
	sw_memset( netmode, sizeof(netmode), 0, sizeof(netmode) );
	sw_memset( sznetmode, sizeof(sznetmode), 0, sizeof(sznetmode) );
	sw_strlcpy( sznetmode, sizeof(sznetmode), sw_network_get_currentmode(), sizeof(sznetmode));

	switch(iConnType)
	{
		case 1:
			{
				if(strncasecmp(sznetmode, "wifi", strlen("wifi")) == 0)
					sw_strlcpy(netmode, sizeof(netmode), "wifipppoe", sizeof(netmode));
				else
					sw_strlcpy(netmode, sizeof(netmode), "pppoe", sizeof(netmode));
				break;
			}
		case 2:
			{
				if(strncasecmp(sznetmode, "wifi", strlen("wifi")) == 0)
					sw_strlcpy(netmode, sizeof(netmode), "wifidhcp", sizeof(netmode));
				else
					sw_strlcpy(netmode, sizeof(netmode), "dhcp", sizeof(netmode));
				break;
			}
		case 3:
			{
				if(strncasecmp(sznetmode, "wifi", strlen("wifi")) == 0)
					sw_strlcpy(netmode, sizeof(netmode), "wifistatic", sizeof(netmode));
				else
					sw_strlcpy(netmode, sizeof(netmode), "static", sizeof(netmode));
				break;
			}
		default:
			{
				//HWNETWORK_LOG_DEBUG("not support this kind of connect type now! \n ");
			}
	}
	if ( netmode[0] != '\0' )
	{
		sw_android_property_set("defaultnetmode",netmode);
		sw_strlcpy(current_netmode, sizeof(current_netmode), netmode, sizeof(current_netmode));
		sw_strlcpy(old_netmode, sizeof(old_netmode), sznetmode, sizeof(old_netmode));
		//sw_parameter_set("defaultnetmode",netmode);
	}
}

static int handle_connecttype_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_READ )
	{
		connecttype_read(value, size);
		return 0;
	}
	else
	{
		connecttype_write(value, size);
		return 0;
	}
	return -1;	
}

static void timeZone_read(char *value, int size)
{
	char buf[12] = {0};
	int hour = 0, minute = 0;
	sw_android_property_get("timezone", buf, sizeof(buf));
	sw_strlcpy(value, size, "GMT ", size);
	value += 4;
	if ( buf[0] != '+' && buf[0] != '-')
		*value++ = '+';
	else
		*value++ = buf[0];
	if ( strchr(buf, ':') == NULL )
	{
		int zone = atoi(buf);
		zone = abs(zone);
		if ( zone <= 12 )
			sw_snprintf(value, size, 0, size, "%02d:00", abs(zone));
		else if ( zone < 780 )
		{
			timezone = abs(zone);
			hour = zone / 60;
			minute = zone %60;
			sw_snprintf(value, size, 0, size, "%02d:%02d", hour, minute);
		}
		else
		{
			zone -= 780;
			hour = 0;
			minute = zone % 60;
			sw_snprintf(value, size, 0, size, "%02d:%02d", hour, minute);
		}
	}
	else
		sw_strlcpy(value, size, buf, size);
}

static void timeZone_write(char *value, int size)
{
	char *p = NULL;
	if(strstr(value, "UTC") || strstr(value, "GMT") )
		p = value + 4;	
	if ( p == NULL )
		return ;
	int timezone = sw_timezone2minute(p);
	int tzone = 0;
	char buf[32] = {0};
	if ( timezone % 60 == 0 )
	{
		tzone = timezone / 60;
		tzone %= 13;
		if ( timezone < 0 )
			sw_snprintf(buf, sizeof(buf), 0, sizeof(buf), "-%d", abs(tzone));
		else
			sw_snprintf(buf, sizeof(buf), 0, sizeof(buf), "%d", tzone);
	}
	else 
	{
		tzone = abs(timezone);
		if ( tzone >= 780 )
			return;
		if ( tzone < 60 )
			tzone = tzone + 13*60;
		if ( timezone < 0 )
			sw_snprintf(buf, sizeof(buf), 0, sizeof(buf), "-%d", tzone);
		else
			sw_snprintf(buf, sizeof(buf), 0, sizeof(buf), "%d", tzone);
	}
	sw_android_property_set("timezone", buf);
	sw_parameter_save();
}

static int handle_timeZone_para(const char *name, char *value, int size, int act_type )
{
	HWNMPD_LOG_DEBUG("handle_timeZone_para,name=%s,value=%s\n", name,value);
	if ( act_type == STBMONITOR_READ )
	{
		timeZone_read(value, size);
		return 0;
	}
	else
	{
		timeZone_write(value, size);
		return 0;
	}
	return -1;	
}

static int handle_localTime_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_READ )
	{
		time_t now;
		time(&now);
//		sw_time2clock((time_t)now, value,size);

		if ( value == NULL )
			return -1;
		struct tm tm;
		struct timeval tv;
		struct timezone tz;
		sw_memset(&tm, sizeof(tm), 0, sizeof(tm));
		sw_memset(&tv, sizeof(tv), 0, sizeof(tv));
		sw_memset(&tz, sizeof(tz), 0, sizeof(tz));

		localtime_r(&now, &tm);
		gettimeofday(&tv, &tz);
		sw_snprintf(value, size, 0, size, "%04d%02d%02d%02d%02d%02d.%03ldZ",
				tm.tm_year+1900,
				tm.tm_mon+1,
				tm.tm_mday,
				tm.tm_hour,
				tm.tm_min,
                tm.tm_sec,
                tv.tv_usec/1000);			
		return 0;
	}
	return -1;	
}

static void epgratio_read(char *value, int size)
{
	int x = 0, y = 0, w = 1280, h = 720;
	char browser_region[64];
	if ( sw_android_property_get("browser_region", browser_region, sizeof(browser_region)) )
		sw_sscanf(browser_region, "%d,%d,%d,%d", &x, &y, &w, &h);
	sw_snprintf(value, size, 0, size, "%d,%d:%d,%d", w, h, x, y);
}

static void epgratio_write(char *value, int size)
{
	int i, x = 0, y = 0, w = 1280, h = 720;
	i = sw_sscanf(value, "%d,%d:%d,%d", &w,&h,&x,&y);
	if (i >= 3 && 640 <= w && w <= 1280 && 480 <= h && h <= 720)
	{
		char browser_region[64] = {0};
		sw_snprintf(browser_region, sizeof(browser_region), 0, size, "%d,%d,%d,%d", x, y, w, h);
		sw_parameter_set("browser_region", browser_region);
	}
}

static int handle_EPGRatio_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_READ )
	{
		epgratio_read(value, size);
		return 0;
	}
	else
	{
		epgratio_write(value, size);
		return 0;
	}
	return -1;	
}

static int handle_hw_op_CAregister_para(const char *name, char *value, int size, int act_type )
{
	char default_mode[10] = {0};
	sw_android_property_get("default_iptv_ott",default_mode,10);
	
	int ca_type = sw_android_property_get_int("ca_type"), ca_type2 = 0;
	int ca_mode = sw_android_property_get_int("ca_mode"), ca_mode2 = 0;
	if( act_type==0 )
	{
		if (ca_mode == 0)
			sw_strlcpy(value, size, "0", size);
		else
		{
			if (ca_type == 4)
				sw_strlcpy(value, size, "4", size);
			else if (ca_type == 3)
				sw_strlcpy(value, size, "3", size);
			else if ( ca_type == 2)
				sw_strlcpy(value, size, "2", size);
			else
				sw_strlcpy(value, size, "0", size);
		}
	}
	else
	{
		if (atoi(value) != 4 && atoi(value) != 3 && atoi(value) != 0 )
			return 0;
		
		if((strcmp(default_mode,"iptv") == 0) && (atoi(value) == 4))
		{
			ca_mode2 = 1;
			ca_type2 = 4;
		}
		else if((strcmp(default_mode,"ott") == 0) && (atoi(value) == 3))
		{
			ca_mode2 = 1;
			ca_type2 = 3;
		}
		else if ( atoi(value) == 2 )
		{
			ca_mode2 = 1;
			ca_type2 = 2;
		}
		HWNMPD_LOG_DEBUG("ca_mode%d, ca_mode2 = %d\n", ca_mode, ca_mode2);
		if ( ca_type != ca_type2 )
			sw_parameter_set_int( "ca_type", ca_type2);
	}
	return 0;
}

static int handle_CASType_para(const char *name, char *value, int size, int act_type )
{  
	char default_mode[10] = {0};
	sw_android_property_get("default_iptv_ott",default_mode,10);
	if ( act_type == STBMONITOR_READ )
	{
		if(strcmp(default_mode,"iptv") == 0)
			sw_strlcpy(value, size, "Verimatrix", size);
		else if ( strcmp(default_mode,"ott") == 0 )
			sw_strlcpy(value, size, "Verimatrix", size);
		else
			sw_strlcpy(value, size, "", size);
		return 0;
	}

	return -1;	
}

/*qosreg 日志上报开关 act_type操作类型：0时为读操作，1时为写操作 */
static int handle_qosreg_para(const char *name,char *value,int size,int act_type)
{
	if(act_type == 0)
	{
		int i;
		i = sw_android_property_get_int("qosreg");
		if( i != 0 && i != 1)
		{
			i = 0;
			sw_android_property_set_int("loguploadinterval", 0);
		}
		sw_snprintf(value, size, 0, size, "%d", i);
	}
	else
	{
		HWNMPD_LOG_DEBUG("++++++++++qosreg:%s++++++++\n", value);
		sw_android_property_set_int("qosreg", atoi(value));
		if(atoi(value) == 1)
			sw_android_property_set_int("StbQosStart", 1);
		else
			sw_android_property_set_int("StbQosStop", 1);
	}
	return 0;
}

static int handle_log_upload_interval(const char *name,char *value,int size,int act_type)
{
	if(act_type == 0)
	{
		int i;
		i = sw_android_property_get_int("loguploadinterval");
		if( i < 0 )
		{
			if(sw_android_property_get_int("qosreg") == 0)
				i = 0;
			else
				i = 3600;
		}	
		sw_snprintf(value, size, 0, size, "%d", i);
	}
	else
	{
		/* 调整qos上报周期 */
		if(atoi(value)< 60)
			sw_android_property_set_int("loguploadinterval", 60);//新规格要求qos上报周期最小为60s
		else
			sw_android_property_set_int("loguploadinterval", atoi(value));
	}
	return 0;
}

static int handle_log_server(const char *name, char *value, int size, int act_type)
{

	if ( act_type == 0 )
	{
		sw_android_property_get("logserverurl", value, size);
		return 0;
	}
	else
	{
		HWNMPD_LOG_DEBUG("++++++++++set logserverurl+++++++\n");
		sw_android_property_set("logserverurl", value);
		if(sw_android_property_get_int("qosreg") == 1)
			sw_android_property_set_int("StbQosStart", 1);
	}
	return -1;
}

static void supporthd_read(char *value, int size)
{
	char hardware[16];
	sw_memset(hardware, sizeof(hardware), 0, sizeof(hardware));
	sw_android_property_get("hardware_version",hardware,sizeof(hardware));
	if(strncmp("SBox71",hardware,6) == 0 || strncmp("SBox75",hardware,6) == 0 
			|| strncmp("SBox88",hardware,6) == 0 || strncmp("SBox86",hardware,6) == 0 )
		sw_strlcpy(value, size, "1", size);
	else
		sw_strlcpy(value, size, "0", size);//??支?指???
}

static int handle_SupportHD_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_READ )
	{
		supporthd_read(value, size);
		return 0;
	}
	return -1;	
}

static int handle_AspectRatio_para(const char *name, char *value, int size, int act_type )
{
/* 0－原始显示，1－全屏显示,实际上是contentmode */
	swcontentmode_e content_mode;
	sw_memset(&content_mode, sizeof(content_mode), 0, sizeof(content_mode));
	if ( act_type == 0 )
	{
		content_mode = (swcontentmode_e)sw_android_property_get_int("content_mode");
		HWNMPD_LOG_DEBUG("----------------------mode:%d----\n", content_mode);
		if ( content_mode == 2)
			sw_strlcpy(value, size, "0", size);
		else if ( content_mode == 1)
			sw_strlcpy(value, size, "1", size);
		else
			sw_strlcpy(value, size, "null", size);
	}
	else
	{
		int mode = atoi(value);
		if ( mode == 1 )
			content_mode = SW_CONTENTMODE_FULL;
		else if ( mode == 0 )
			content_mode = SW_CONTENTMODE_LETTERBOX;
		else
			return -1;
		sw_parameter_set_int("content_mode", content_mode);
		sw_parameter_set_int("aspect_mode", -1);
	}
	return 0;
}

static int stbmonitor_HDVideoStandard(const char *name,char *value,int size,int act_type)
{
	static struct {
		char *jsstr;
		char *hdstandard;
	} m_map[] = {
		{"1080p60Hz", "0"},
		{"1080p50Hz", "1"},
		{"1080i60Hz", "5"},
		{"1080i50Hz", "6"},
		{"720p60Hz",  "7"},
		{"720p50Hz",  "8"},
		{"576p50Hz",  "9"},//576i60Hz??????
		{"480p60Hz",  "10"},
		{"PAL",  "11"},
		{"NTSC",  "12"},
	};
	unsigned int i = 0;
	if ( act_type == 0 )
	{
		char hdstandard[12] = {0};
		sw_android_property_get("hd_standard", hdstandard, sizeof(hdstandard));
		for (i = 0 ; i < sizeof(m_map)/sizeof(m_map[0]); i++)
		{
			HWNMPD_LOG_DEBUG("=====================hdstandard:%s,\n",hdstandard);
			if ( strcasecmp(m_map[i].hdstandard, hdstandard) == 0 )
			{
				sw_strlcpy(value, size, m_map[i].jsstr, size);
				return 0;
			}
		}
		sw_strlcpy(value, size, hdstandard, size);
	}
	else
	{
		for (i = 0 ; i < sizeof(m_map)/sizeof(m_map[0]); i++)
		{
			if ( strcasecmp(m_map[i].jsstr, value) == 0 )
			{
				sw_parameter_set("hd_standard", m_map[i].hdstandard);
				return 0;
			}
		}
	}
	return 0;

}


static int handle_SoftwareVersion_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_READ )
	{
		property_get("ro.build.version.hwincremental",value,"");
		return 0;
	}
	return -1;	
}


static int handle_SoftwareHWversion_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_READ )
	{
//		sw_strlcpy(value, size, HUAWEI_VERSION, size);
		return 0;
	}
	return -1;	
}

static int handle_CompTime_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_READ )
	{
		sw_android_property_get("app_release_time", value, size);
		return 0;
	}
	return -1;	
}


static int handle_BrowserVersion_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_READ )
	{
		sw_android_property_get("BrowserVersion", value, size);
		return 0;
	}
	return -1;	
}

static int handle_BrowserTime_para(const char *name, char *value, int size, int act_type )
{
	if ( act_type == STBMONITOR_READ )
	{
//		sw_browser_get_release_time(value, size);
		return 0;
	}
	return -1;	
}

static int handle_IPAddress_para(const char *name, char *value, int size, int act_type )
{
    HWNMPD_LOG_DEBUG("#########################name:%s\n",name);
	if(act_type == STBMONITOR_READ)
	{
		sw_strlcpy(value, size, sw_network_get_currentip(), size);
		return 0;
	}
	return -1;	
}

static int handle_channelSwitchMode(const char *name,char *value,int size,int act_type)
{
	if ( act_type == 0 )
	{
		int innermode = sw_android_property_get_int("inner_stop_holdpic");
		int videomode = sw_android_property_get_int("video_render");
		if ( innermode == 1 && videomode == SW_VIDEORENDER_SLOWVIEW )
			sw_strlcpy(value, size, "smooth switch", size);
		else if ( innermode == 1 && videomode == SW_VIDEORENDER_NORMAL )
			sw_strlcpy(value, size, "last picture", size);
		else if ( innermode == 0 && videomode == SW_VIDEORENDER_NORMAL )
			sw_strlcpy(value, size, "normal", size);
		else
			sw_strlcpy(value, size, "normal", size);
	}
	else
	{
		int innermode;
		int videomode;
		if ( strcasecmp(value, "normal") == 0 )
		{
			innermode = 0;
			videomode = SW_VIDEORENDER_NORMAL;
		}
		else if ( strcasecmp(value, "smooth switch") == 0 )
		{
			innermode = 1;
			videomode = SW_VIDEORENDER_SLOWVIEW;
		}
		else if ( strcasecmp(value, "last picture") == 0 )
		{
			innermode = 1;
			videomode = SW_VIDEORENDER_NORMAL;
		}
		else
			return 0;
		sw_parameter_set_int("inner_stop_holdpic", innermode);
		sw_parameter_set_int("video_render", videomode);
	}
	return 0;
}

static int handle_channelSwitchMode_para(const char *name, char *value, int size, int act_type )
{
	return handle_channelSwitchMode(name, value, size, act_type == STBMONITOR_READ ? 0 : 1);
}

static int handle_pcap_dat_para(const char *name, char *value, int size, int act_type )
{
	return -1;	
}

static int handle_telnetServiceEnable_para(const char *name, char *value, int size, int act_type )
{
    if(act_type == 1)
    {
		int val = atoi(value);
		hw_telnet_enable((bool)val);
        return 0;
    }
	else
	{
        int m = hw_telnet_is_enable();
		sw_snprintf(value, size, 0, size, "%d", m);
        return 0;
    }
    return -1;
}

static int handle_allDebugInfo_para(const char *name, char *value, int size, int act_type )
{
	return -1;	
}

static int handle_serialPortServiceEnable_para(const char *name, char *value, int size, int act_type )
{
	if(act_type == 1)
    {
		int val = atoi(value);
		hw_telnet_enable((bool)val);
        return 0;
    }
	else
	{
        int m = hw_telnet_is_enable();
		sw_snprintf(value, size, 0, size, "%d", m);
        return 0;
	}	
}
