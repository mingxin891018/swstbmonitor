#include "swapi.h"
#include "swapp.h"
#include "swparameter.h"
#include "hwiptv.h"
//#include "cwmp_db.h"
#include "hwnmpd_priv.h"
#include "swijavadepot.h"
#include "swstb.h"
//#所有参数名和参数值所占空间大小
#define MAX_PARA_SIZE (128*1024)

/*在java仓库里面存储的参数表和默认值*/
static java_map_t m_java_map[] =
{
    //系统或者设置参数
	{"qosreg", "0"},
	{"loguploadinterval", "xx"},
	{"logserverurl", "xx"},
//	{"inner_stop_holdpic", "1"},
//	{"video_render", "1"},
	{"ntvuseraccount", "xx"}, 
	{"ntvuserpassword", "xx"},
	{"home_page", "xx"},
	{"home_page2", "xx"},
	{"mac", "xx"},
	{"serial", "xx"},
	{"stbid", "xx"},
	{"timezone", "xx"},
    {"hardware_type", "xx"},
    {"hardware_version", "xx"},
    {"app_version", "xx"},
    {"hw_app_version", "xx"},
    {"app_release_time", "xx"},
	{"ntpserver", "xx"},
	{"ntpserver2", "xx"},
    {"hw_upgrade_server", "xx"},
    {"hw_upgrade_server2"},
    {"hd_standard", "1080i_50Hz"},
    {"tvms_heartbeat_url", "xx"},
    {"tvms_vod_url", "xx"},
    {"tvms_heartbeat_ period", "xx"},
    {"tvms_heartbeat_delay", "xx"},
    //hwstbmonitor参数
    {"stbmonitorable", "1"},
    {"manager_user", "xx"},
    {"manager_password", "xx"},
    {"new_manager_password", "xx"},
    //获取网络接口
    {"defaultnetmode", "dhcp"},
	{"pppoe_user" , "xx"},
	{"pppoe_pswd", "xx"},
    {"dhcp_user", "xx"},
    {"dhcp_pswd", "xx"},
    {"reboot", "xx"},

#if 0
	{"lan_ip", "4"},  
	{"lan_mask", "4"},
	{"lan_gateway", "4"},
	{"lan_dns", "4"},
	{"lan_dns2", "4"},
	{"pppoe_user" , "64"},
	{"pppoe_pswd", "64"},
	{"pppoe_sessionid", "4"},
	{"wifi_ip", "4"},
	{"wifi_dns", "4"},
	{"wifi_dns2", "4"},
	{"wifi_mask", "4"},
	{"wifi_gateway", "4"},
#endif
    {"HDCPEnableDefault", "xx"},
    {"MacrovisionEnableDefault", "xx"},
    {"CGMSAEnableDefault", "xx"},
    {"ijava_para_path", "xx"}, //获取java数据文件位置接口
#if 0 //暂时去掉不需要的参数
	{"wifi_ssid", "64"},
	{"wifi_authentication_type", "1"},
	{"wifi_password", "64"},
	{"wifi_password1", "64"},
	{"wifi_password2", "64"},
	{"wifi_password3", "64"},
	{"wifi_password4", "64"},
	{"wifi_password5" , "64"},
#endif
    //下面接口调用sw_android_property_set设置属性，当作参数可能会有缓存, 会有问题
/*  //控制音量接口
    {"maxvol", "xx"},
    {"minvol", "xx"},
    {"sys_volume", "xx"},
    {"mute", "xx"},
    //HDCP, MACROVISION, CGMSA 接口
    {"HDCPEnable", "xx"},    
    {"MacrovisionEnable", "xx"},
    {"CGMSAEnable", "xx"},
*/
	// tms 网管系统基本参数
	{"tmsreg", "xx"},
    {"acs_periodinform", "xx"},
    {"acs_piinterval", "xx"},
    {"acs_url", "xx"},
    {"acs_backup_url", "xx"},
    {"acs_username", "xx"},    
    {"acs_password", "xx"},
    {"acs_crusername", "xx"},
    {"acs_crpassword", "xx"},
};

#ifndef SUPPORT_LYCA
static void parse_parameter_from_cmdline(void);
#endif

static sw_idepot_t* m_javadepot = NULL;
static sw_idepot_t* m_ifiledepot[3]={NULL,NULL,NULL};

static wcallback_t wfun = NULL; /*java数据库读写指针*/
static rcallback_t rfun = NULL;

int sw_app_parameter_init()
{
#ifndef SUPPORT_LYCA
    struct stat stat_info;
    char java_para_path[128] = {0};
    FILE *fp = NULL;
#endif

    /*初始化参数列表*/
    if( sw_parameter_init(MAX_PARA_SIZE) == false )
    {
        HWNMPD_LOG_DEBUG("parameter init fail\n");
        return false;
    }

    sw_monitor_get_javapara_callback(&wfun, &rfun);

    /*打开系统参数和设置页面参数仓库，存储在java数据库里面*/
    m_javadepot = sw_ijavadepot_open(m_java_map, sizeof(m_java_map) / sizeof(m_java_map[0]));
    sw_ijavadepot_set_wrcallback(wfun, rfun); //设置ijava对象的读/写参数方法
    if( sw_parameter_register_depot(m_javadepot, false) == false )
    {
        HWNMPD_LOG_DEBUG("parameter register fail,func:%s,:line:%d\n",__FUNCTION__,__LINE__);
    }

#ifndef SUPPORT_LYCA
    sw_strlcpy(java_para_path, sizeof(java_para_path), "/data/data/net.sunniwell.app.ott.huawei/", sizeof(java_para_path)); /*在编译测试程序的时候需要手动修改这个路径，否则测试程序跑不起来*/
    HWNMPD_LOG_DEBUG("%s:%d, ijava_para_path = %s\n", __FUNCTION__, __LINE__,  java_para_path);

    /*打开often文件参数仓库*/
    sw_strlcat(java_para_path, sizeof(java_para_path), ".iptv", sizeof(java_para_path) );
    if ( lstat( java_para_path,  &stat_info ) != 0 )
	{
        if ( mkdir(java_para_path, S_IRWXU | S_IRWXG | S_IROTH) < 0 )
		{
    		HWNMPD_LOG_DEBUG("%s:%d, mkdir error %s\n", __FUNCTION__, __LINE__, strerror(errno));
		}
	}

    sw_strlcat(java_para_path, sizeof(java_para_path), "/often", sizeof(java_para_path) );
    if ( lstat( java_para_path,  &stat_info ) != 0 ) /*判断文件是否存储，不存在就创建*/
    {
        if((fp = fopen(java_para_path, "w+")) != NULL)
            fclose(fp);
    }
    m_ifiledepot[0] = sw_ifiledepot_open(java_para_path);
    if( m_ifiledepot[0] == NULL )
    {
        HWNMPD_LOG_DEBUG("open %s fail\n",java_para_path);
        return false;
    }
    m_ifiledepot[0]->autosave = true; 
    if( sw_parameter_register_depot(m_ifiledepot[0],false) == false )
    {
        HWNMPD_LOG_DEBUG("parameter register fail,func:%s,:line:%d\n",__FUNCTION__,__LINE__);
    }
    /*把频繁读取的参数，默认设置到这个仓库里面去*/
    sw_parameter_set_default("lastchannelid","0",m_ifiledepot[0]);
    sw_parameter_set_default("last_pip_channel","0",m_ifiledepot[0]);
    sw_parameter_set_default("last_multicast_addr","0",m_ifiledepot[0]);
    sw_parameter_set_default("local_port","0",m_ifiledepot[0]);
    sw_parameter_set_default("pppoe_sessionid","0",m_ifiledepot[0]);
    //sw_parameter_set_default("volume","12",m_ifiledepot[0]);  //每个据点使用的参数是不一样的,这里不能加，否则回复出厂有问题
    sw_parameter_set_default("muteflag","0",m_ifiledepot[0]);
//    sw_parameter_set_default("epgurl","www.baidu.com",m_ifiledepot[0]);

    java_para_path[strlen(java_para_path) - strlen("/often")] = '\0';

    /*打开iptv正常业务使用的参数*/
    sw_strlcat(java_para_path, sizeof(java_para_path), "/para", sizeof(java_para_path) );
    HWNMPD_LOG_DEBUG("%s:%d, ijava_para_path = %s\n", __FUNCTION__, __LINE__,  java_para_path);
    if ( lstat( java_para_path,  &stat_info ) != 0 ) /*判断文件是否存储，不存在就创建*/
    {
        if((fp = fopen(java_para_path, "w+")) != NULL)
            fclose(fp);
    }
    m_ifiledepot[1] = sw_ifiledepot_open(java_para_path);
    if( sw_parameter_register_depot(m_ifiledepot[1],true) == false)
    {
        HWNMPD_LOG_DEBUG("parameter register fail,func:%s,:line:%d\n",__FUNCTION__,__LINE__);
        return false;
    }
    java_para_path[strlen(java_para_path) - strlen("/.iptv/para")] = '\0';

    if (lstat( "/swdb/upgrade/upgradeparam.txt",  &stat_info ) == 0 )
	{//更新升级的参数文件到parameter里,在删除文件前需要保存所有参数
		m_ifiledepot[2] = sw_ifiledepot_open("/swdb/upgrade/upgradeparam.txt");	
		sw_parameter_updatefrom_depot(m_ifiledepot[2]);
		sw_ifiledepot_close(m_ifiledepot[2]);
		m_ifiledepot[2] = NULL;
		sw_parameter_save();
		if ( remove("/swdb/upgrade/upgradeparam.txt") == -1 )
		{
        	HWNMPD_LOG_DEBUG("remove failed ,func:%s,:line:%d\n",__FUNCTION__,__LINE__);
		}
	}

    sw_strlcat(java_para_path, sizeof(java_para_path), "/defaultparam.txt", sizeof(java_para_path));
    HWNMPD_LOG_DEBUG("%s:%d, ijava_para_path = %s\n", __FUNCTION__, __LINE__,  java_para_path);
	/* 预先加载默认的参数,防止删除参数后参数丢失导致运行不正常,参数表里面如果存在参数，参数值是不会被修改的 */
    if ( lstat( java_para_path,  &stat_info ) == 0 ) /*判断文件是否存储，不存在就创建*/
		sw_app_parameter_set_group_default(java_para_path);

	/* 此处解析cmdline判断dram_size,kernel_dram_size,sdk的内存大小 */
	parse_parameter_from_cmdline();

	//sw_parameter_set("huawei_version",HUAWEI_VERSION);
	sw_parameter_set_readonly("flash_size",true);
	sw_parameter_set_readonly("dram_size",true);
	sw_parameter_set_readonly("kernel_dram_size",true);
	sw_parameter_set_readonly("app_mode_reg", true);
	sw_parameter_set_readonly("boot_mode_reg", true);
	sw_parameter_set_readonly("cpu_freq",true);
	sw_parameter_set_readonly("loader_version",true);
	sw_parameter_set_readonly("boot_version",true);
	sw_parameter_set_readonly("backupboot_version",true);
	sw_parameter_set_readonly("app_version",true);
	sw_parameter_set_readonly("logo_version",true);
	sw_parameter_set_readonly("param_version",true);
	sw_parameter_set_readonly("huawei_version",true);
	sw_parameter_set_readonly("playlist_version",true);
	sw_parameter_set_readonly("font_version",true);
	sw_parameter_set_readonly("setting_version",true);
	sw_parameter_set_readonly("mac",true);
	sw_parameter_set_readonly("serial",true);
	sw_parameter_set_readonly("support_ac3wms", true);
	sw_parameter_set_readonly("upgrade_signature", true);
	//hw_iptv_parameter_validate(NULL, NULL);

#ifdef SUPPORT_CTC_TMS30
	sw_parameter_set_observer(cwmp_data_modified_func,0xFFFF);
#endif
#endif
    /*STBID和MAC地址都在盒子生产的时候被写入，这里重新设置STBID是不对的*/
    //sw_modify_stbid();
	return 0;
}

//恢复出厂参数
void sw_app_parameter_restore( )
{
    return ;
}

//向参数中追加默认的参数值
void sw_app_parameter_set_group_default(char *path )
{	char *buf = NULL;
	int fd = open(path,  O_RDONLY);
	if ( fd >= 0 )
	{
		int fsize = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
		if ( fsize > 0 )
		{
			buf = malloc( fsize + 2);
			if ( buf )
			{
				int size = read(fd, buf, fsize);
				size = size > 0 ? size : 0;
				buf[size] = '\0';
				if ( size > 0 )
					sw_parameter_set_group_default(buf, size);
			}
		}
		close( fd );
		if ( buf )
			free(buf);
	}
    return ;
}

void sw_app_parameter_exit()
{	
	int i =0;

	sw_parameter_exit();
	if( m_javadepot)
	{
//		sw_ijavadepot_close(m_javadepot);
		m_javadepot = NULL;
	}

	for( i=0 ;i<(int)(sizeof(m_ifiledepot)/sizeof(sw_idepot_t*)); i++)
	{
		if( m_ifiledepot[i] != NULL )
		{
			sw_ifiledepot_close(m_ifiledepot[i]);
			m_ifiledepot[i] = NULL;
		}
	}
}
/*  
int sw_app_parameter_updatefrom_depot(void)
{
	sw_app_parameter_exit();
	sw_app_parameter_init();
	return 0;
}
*/
bool sw_app_update_ijavadepot(void)
{
    if( m_javadepot == NULL)
        return false;
    return sw_parameter_updatefrom_depot(m_javadepot);
}

#ifndef SUPPORT_LYCA
static void parse_parameter_from_cmdline(void)
{
	int physize = -1;
	int kernel_size = -1;
	int sdksize = -1;
	char buf[512];
	
	FILE* fp = fopen( "/proc/cmdline", "rb" );
	if( fp )
	{
		int rsize = fread(buf, 1, sizeof(buf)-1, fp);
		fclose(fp);
		if ( rsize > 0 )
		{
			buf[rsize] = '\0';
			char *p = buf;
			while ( p )
			{/* 解析tmem */
				p = strstr(p, "tmem=");
				if ( p && (p == (char*)buf || !isalpha(*(p-1))) )
				{
					p += 5;/* strlen("tmem=") */
					physize = atoi(p);
					break;
				}
				else if ( p )
					p += 5;
			}
			p = buf;
			while ( p )
			{/* 解析kernel size */
				p = strstr(p, "mem=");
				if ( p && (p == (char*)buf || !isalpha(*(p-1))) )
				{
					p += 4;/* strlen("mem=") */
					kernel_size = atoi(p);
					break;
				}
				else if ( p )
					p += 4;
			}	
			/* 判断是否存在mmz= */
			p = buf;
			while ( p )
			{/* 解析kernel size */
				p = strstr(p, "mmz=");
				if ( p && (p == (char*)buf || !isalpha(*(p-1))) )
				{
					p += 4;/* strlen("mmz=") */
					/*mmz=ddr,0,0xxxxx,96M*/
					/* 定位到第4个参数 */
					while (*p != '\0' && *p != ',')
						p++;
					if (*p != ',')
						break;
					p++;
					while (*p != '\0' && *p != ',')
						p++;
					if (*p != ',')
						break;
					p++;
					while (*p != '\0' && *p != ',')
						p++;
					if (*p != ',')
						break;
					p++;
					sdksize = atoi(p);
				}
				else if ( p )
					p += 4;
			}
		}
	}
	if ( physize < 128 )
	{
		if ( 0 < sdksize && 0 < kernel_size)
			physize = sdksize + kernel_size;
		else
			physize = 256;
	}
	if (kernel_size < 0)
		kernel_size = 96;
	if (physize != sw_parameter_get_int("dram_size"))
		sw_parameter_set_int("dram_size", physize);
	if ( kernel_size != sw_parameter_get_int("kernel_dram_size") )
		sw_parameter_set_int("kernel_dram_size", kernel_size);
}
#endif

//直接调用android的接口，有些属性需要直接直接设置的
bool sw_android_property_get(char *name, char *value, int size)
{
	if(rfun)
    {
		rfun(name, value, size);
        return true;
    }
    return false;
}

int sw_android_property_get_int(char *name)
{
    char text[16]={0};
    sw_android_property_get(name, text, sizeof(text));
    if(strlen(text) > 0)
        return atoi(text);
    return -1;
}

bool sw_android_property_set(char *name, char *value)
{
	if(wfun)
	{
		if(rfun)
		{
			char buf[1024] = {0};
			rfun(name, buf, sizeof(buf));
			if(strlen(buf) > 0 && strcmp(buf, value) == 0)
			{
				HWNMPD_LOG_DEBUG("name:%s,don't channge, don't need set\n", name);
				return true;
			}
		}
		return wfun(name, value);
    }
    return false;
}

/* 
 使用管理工具将网络模式从hdcp换成static的时候，
 需要将ip，gateway, mask, dns设置下去，否则apk会使用默认参数
 所以不用去判断将要设置的参数和盒子里面当前参数是否相同
 （hdcp->static如果不去修改管理工具上的ip,mask,dns，默认就和hdcp一样）
 */
bool sw_get_android_way_to_set(char* name, char* value)
{
	if(wfun)
		return wfun(name, value);
	return false;
}


bool sw_android_property_set_int(char *name, int value)
{

    char text[16] = {0};
    sw_snprintf( text, sizeof(text), 0, sizeof(text), "%d", value );

    return sw_android_property_set(name, text);
}
