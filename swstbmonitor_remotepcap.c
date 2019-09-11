/***************************************************************************
* Copyright: sunniwell
* file     : nmpserver.c
* AUTHOR   : 吴荣富
* brief    : 华为统一管理工具服务器接口
****************************************************************************/
#include "swstbmonitor_priv.h"
#include "swstbmonitorserver.h"
#include "swthrd.h"
#include "swparameter.h"

static sw_stbmonitor_reply_callback m_cbfs_reply = NULL;
static sw_stbmonitor_callback_funcs_t *m_cbfs = NULL;
static int function_tcpdump(unsigned long wParam,unsigned long lParam);
static int function_tcpdumppro(unsigned long wParam,unsigned long lParam);
static int sw_stbmonitor_nmp_remotepcap();
static int sw_stbmonitor_get_filesize(char * strfilename);  
static void getlocaltime(char *buf, int size);

int sw_stbmonitor_set_remotepcap_reply_callback( sw_stbmonitor_reply_callback cbfs_reply )
{
	m_cbfs_reply = cbfs_reply;
	return 0;
}

int sw_stbmonitor_remotepcap_get_maxfilesize()
{
	char buf[32];
	memset(buf,0,sizeof(buf));
	snprintf(buf,sizeof(buf),"pcap_maxfilesize:%d",4*1024*1024);
	if( m_cbfs_reply != NULL )
		m_cbfs_reply( STBMONITOR_REPLY_OK, "inform", buf, "NULL" );
	return 0;
}

int sw_stbmonitor_remotepcap_start( sw_stbmonitor_callback_funcs_t *cbfs, char *parameter, char *data )
{
	SWSTBMONITOR_LOG_DEBUG("parameter = %s,data = %s\n", parameter,data );
	HANDLE m_pTrd = NULL;
	HANDLE m_pTrd1 = NULL;
	//int ret = 0;
	char *p;
	char *q;
	int buflen=0;
	char buf[128]={0};
	char ip[128]={0};
	char port[64]={0};
	char timelength[64]={0};
	char filesize[64]={0};
	
	if(cbfs != NULL)
		m_cbfs = cbfs;
	buflen = snprintf(buf,sizeof(buf),parameter);
	q = strstr(buf," ");
	snprintf(ip,q-buf-2,buf+3);
	SWSTBMONITOR_LOG_DEBUG("ip = %s size = %d\n",ip,strlen(ip));
	snprintf(port,sizeof(port),q+6);
	SWSTBMONITOR_LOG_DEBUG("port = %s size = %d\n",port,strlen(port));
	buflen = snprintf(buf,sizeof(buf),data);
	q = strstr(buf,"^");
	snprintf(timelength,q-buf-4,buf+5);
	SWSTBMONITOR_LOG_DEBUG("timelength = %s size = %d\n",timelength,strlen(timelength));
	q = strstr(buf,"^filesize:");
	p = strstr(buf,"^null");
	snprintf(filesize,p-(q+10)+1,q+10);
	SWSTBMONITOR_LOG_DEBUG("filesize = %s size = %d\n",filesize,strlen(filesize));
	
	m_pTrd = sw_thrd_open("thrdtcpdump",200,0,128*1024,(threadhandler_t)function_tcpdump,(uint32_t)ip,(uint32_t)port);
	if(m_pTrd)
		sw_thrd_resume( m_pTrd );
	sw_thrd_delay(500);
	m_pTrd1 = sw_thrd_open("thrdtcpdumppro",200,0,128*1024,(threadhandler_t)function_tcpdumppro, (uint32_t)timelength,(uint32_t)filesize);
	if( m_pTrd1)
		sw_thrd_resume( m_pTrd1 );
	
	if( m_cbfs_reply != NULL )
	{
		if(m_pTrd && m_pTrd1)
			m_cbfs_reply( STBMONITOR_REPLY_OK, "ioctl", "NULL", "NULL" );
		else
			m_cbfs_reply( STBMONITOR_REPLY_CMD_ILLEGAL, "ioctl", "NULL", "NULL" );
	}
	sw_thrd_delay(1000);
	return 0;
}

static int function_tcpdumppro(unsigned long wParam,unsigned long lParam)
{
	SWSTBMONITOR_LOG_DEBUG("function_tcpdumppro begin\n");
	time_t begin = 0;
	time_t last = 0;
	time_t current = 0;
	char timelength[64] = {0};
	char size[64] = {0};
	strlcpy(timelength,(char *)wParam,sizeof(timelength));
	strlcpy(size,(char *)lParam,sizeof(size));
	time( &begin );
	current = begin;
	last = begin;
	int filesize = 0;
	char buf[64];
	if(strlen(size) != 0)
	{
		while( (sw_stbmonitor_get_filesize("/tmp/localtcpdump.cap") < atoi(size)) )
		{
			filesize = sw_stbmonitor_get_filesize("/tmp/localtcpdump.cap");
			snprintf(buf,sizeof(buf),"get_pcapfilesize:%d",filesize);
			time( &current );
			if((current -last) >= 5)
			{
				last = current;
				if( m_cbfs_reply != NULL )
					m_cbfs_reply( STBMONITOR_REPLY_OK, "ioctl", buf, "NULL" );
			}
			sw_thrd_delay(500);
			if((strlen(timelength) != 0) && (current - begin) > atoi(timelength))
			{
				SWSTBMONITOR_LOG_DEBUG("end of the time,break exit\n");
				break;
			}
		}
	}
	else
	{
		while(1)
		{
			filesize = sw_stbmonitor_get_filesize("/tmp/localtcpdump.cap");
			snprintf(buf,sizeof(buf),"get_pcapfilesize:%d",filesize);
			time( &current );
			if((current -last) >= 5)
			{
				last = current;
				if( m_cbfs_reply != NULL )
					m_cbfs_reply( STBMONITOR_REPLY_OK, "ioctl", buf, "NULL" );
			}
			sw_thrd_delay(500);
			if( (current - begin) > atoi(timelength) )
			{
				SWSTBMONITOR_LOG_DEBUG("end of the time,break exit2\n");
				break;
			}
		}
	}
	if( m_cbfs != NULL && m_cbfs->fcb_on_syscmd != NULL )
		m_cbfs->fcb_on_syscmd("killall -9 tcpdump");
	else
		system("killall -9 tcpdump");
	sw_stbmonitor_nmp_remotepcap();
	if( m_cbfs != NULL && m_cbfs->fcb_on_syscmd != NULL )
		m_cbfs->fcb_on_syscmd("rm /tmp/localtcpdump.cap");
	else
		system("rm /tmp/localtcpdump.cap");
	return false;
}

static int function_tcpdump(unsigned long wParam,unsigned long lParam)
{
	SWSTBMONITOR_LOG_DEBUG("wParam = %s lParam = %s\n",(char *)wParam,(char *)lParam);
	char buf[128] = {0};
	char ip[64] = {0};
	char port[32] = {0};
	char device_name[16] = {0};
	strlcpy(ip,(char *)wParam,sizeof(ip));
	strlcpy(port,(char *)lParam,sizeof(port));
	//TODO 增加wifi网卡的支持
	snprintf(device_name,sizeof(device_name),"eth0");
	if(strlen(ip) == 0 && strlen(port) != 0)
		snprintf(buf,sizeof(buf),"tcpdump -i %s -s0 port %s -w /tmp/localtcpdump.cap",device_name,port);
	else if(strlen(ip) != 0 && strlen(port) == 0)
		snprintf(buf,sizeof(buf),"tcpdump -i %s -s0 host %s -w /tmp/localtcpdump.cap",device_name,ip);
	else if(strlen(ip) == 0 && strlen(port) == 0)
		snprintf(buf,sizeof(buf),"tcpdump -i %s -s0 -w /tmp/localtcpdump.cap",device_name);
	else
		snprintf(buf,sizeof(buf),"tcpdump -i %s -s0 host %s and port %s -w /tmp/localtcpdump.cap",device_name,ip,port);
	SWSTBMONITOR_LOG_DEBUG("%s\n",buf);
	if( m_cbfs != NULL && m_cbfs->fcb_on_syscmd != NULL )
		m_cbfs->fcb_on_syscmd(buf);
	else
		system(buf);
	return false;

}

static int uploadsize = 0;
static int function_sftpuploadpro(unsigned long wParam,unsigned long lParam)
{
	char buf[64] = {0};
	time_t begin;
	time_t current;
	time( &begin );
	current = begin;
	while( (current - begin)%5 == 0 )
	{
		time( &current );
		snprintf(buf,sizeof(buf),"get_capfileuploadszie:%d",uploadsize);
		if( m_cbfs_reply != NULL )
			m_cbfs_reply( STBMONITOR_REPLY_OK, "ioctl", buf, "NULL" );
		sw_thrd_delay(500);
		if(sw_stbmonitor_get_filesize("/tmp/localtcpdump.cap") == uploadsize)
		{
			SWSTBMONITOR_LOG_DEBUG("function_sftpuploadpro end\n");
			break;
		}
	}
	return false;
}

static int function_sftpupload(unsigned long wParam,unsigned long lParam)
{
	char sftpserver[128] = {0};
	char time[64] = {0};
	char buf[128] = {0};
	//strlcpy(sftpserver,sw_stbmonitor_get_ip(),sizeof(sftpserver));
	//SWSTBMONITOR_LOG_DEBUG("sftpserver = %s\n",sftpserver); //get stbmonitor host ip
	getlocaltime(time, sizeof(time));
	SWSTBMONITOR_LOG_DEBUG("time = %s\n",time);
	snprintf(buf,sizeof(buf),"remotefile_%s.cap",time);
	sw_parameter_get("logserver", sftpserver, sizeof(sftpserver));//get log server ip address
	if( m_cbfs != NULL && m_cbfs->fcb_on_sftp_write_noblock != NULL )
		m_cbfs->fcb_on_sftp_write_noblock(0, sftpserver, "hwtcpdump", "HwdumpIsGood", "/tmp/localtcpdump.cap", buf, "1", &uploadsize);
	SWSTBMONITOR_LOG_DEBUG("sftpserver:%s\n",sftpserver);
	return false;
}

static int sw_stbmonitor_nmp_remotepcap()
{
	HANDLE m_pTrd = NULL;
	HANDLE m_pTrd1 = NULL;
	m_pTrd = sw_thrd_open("thrdsftpupload",200,0,128*1024,(threadhandler_t)function_sftpupload, 0,0);
	if( m_pTrd )
		sw_thrd_resume( m_pTrd );
	sw_thrd_delay(500);
	m_pTrd1 = sw_thrd_open("thrdsftpuploadpro",200,0,128*1024,(threadhandler_t)function_sftpuploadpro, 0,0);
	if( m_pTrd1 )
		sw_thrd_resume( m_pTrd1 );
	return false;
}

static int sw_stbmonitor_get_filesize(char * strfilename)    
{   
    struct stat temp;   
    if( stat(strfilename, &temp) )
	{
		SWSTBMONITOR_LOG_ERROR(" stat '%s' failed\n", strfilename);
		return -1;
	}
    return temp.st_size;   
} 

static void getlocaltime(char *buf, int size)
{
    time_t thistime;
    struct tm *ptime;
    memset(&thistime, 0, sizeof(time_t));
    time(&thistime);
#ifdef SUPPORT_UTC
    ptime = gmtime(&thistime);
#else
    ptime = localtime(&thistime);
#endif
    memset(buf, 0,size);
    snprintf(buf, size - 1, "%04d%02d%02d%02d%02d%02d", ptime->tm_year + 1900,  ptime->tm_mon + 1,
            ptime->tm_mday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
}
