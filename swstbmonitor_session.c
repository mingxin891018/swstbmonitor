/***************************************************************************
* Copyright: sunniwell
* file     : nmpserver.c
* AUTHOR   : 吴荣富
* brief    : 华为统一管理工具服务器接口
****************************************************************************/
#include "swstbmonitor_priv.h"
#include "swstbmonitorserver.h"
#include "swthrd.h"
#include "netinet/tcp.h"
#include "swlog.h"
#define MAX_LOCK_IP 10
#define LOCK_TIME 300
#define RECV_BUF_SIZE 32*1024
#define DEC_BUF_SIZE 32*1024

typedef struct
{
	HANDLE thread;
	int type;//0为PC连接机顶盒,1为机顶盒主动连接PC
	int socket;
	char *buf;
	int buf_size;
	char *decbuf;
	int  decbuf_size;

	char *encbuf;
	int  encbuf_size;
	char *sendbuf;
	int  sendbuf_size;
}session_info_t;

typedef struct _packet_info
{
	int function_call;
	char call_value[32];
	char parameter[128];
	char data[1024];
}packet_info_t;

typedef struct _reply_info
{
	char *head;
	int head_size;
	char *data;
	int data_size;
}reply_info_t;

typedef struct _lock_ip_info
{
	char ip[16];
	int start_time;
}lock_ip_info_t;

static char m_first_connected_ip[32] = {'\0'};
static lock_ip_info_t lock_ip_info[MAX_LOCK_IP];
static char m_monitor_ip[32] = {0};
static int m_lock_ip_count = 0;
static int m_fail_times = 0;

static int m_file_size = 0;
static int m_current_size = 0;
static bool m_is_stb_sleep = false;
static bool m_session_is_create = false;
static session_info_t m_session;
static sw_stbmonitor_callback_funcs_t *m_cbfs = NULL;

static bool sw_stbmonitor_session_recv_proc( unsigned long wParam, unsigned long lParam );
static int sw_stbmonitor_on_initialize_request( char *buf, int size );
static int sw_stbmonitor_on_data( char *buf, int size );

static int sw_stbmonitor_send_reply_data( reply_info_t *info, int need_enc );
static int sw_stbmonitor_send_reply( int reply_code, char *function_call, char *call_value, char *data );

static int sw_stbmonitor_packet_parse( packet_info_t *info, char *buf, int size );
static int sw_stbmonitor_handle_function_read( packet_info_t *info );
static int sw_stbmonitor_handle_function_write( packet_info_t *info );
static int sw_stbmonitor_handle_function_ioctl( packet_info_t *info );
static int sw_stbmonitor_handle_function_inform( packet_info_t *info );
static int sw_stbmonitor_handle_function_connect( packet_info_t *info );

int sw_stbmonitor_sleep(bool sleep)
{
	m_is_stb_sleep = sleep;
	return 0;
}

//只支持一个session
int sw_stbmonitor_create_session(int socket, sw_stbmonitor_callback_funcs_t *cbfs, int type)
{
	if(m_session_is_create == true)
	{
#ifdef SUPPORT_OTT
		return -1;
#endif

#ifdef SUPPORT_C60
		return -1;
#endif

#ifndef SUPPORT_TM_8V8
		//如果不在升级，可以允许新连接强占
		if( sw_stbmonitor_is_upgrading() != true )
			sw_stbmonitor_destroy_session();
		else
#endif
			return -1;
	}
	memset( &m_session, 0, sizeof(m_session) );
	if(cbfs != NULL)
		m_cbfs = cbfs;
	m_session.type = type;
	m_session.socket = socket;
	m_session.thread = sw_thrd_open( "tStbMonitorSession", 80, 0, 192*1024, (threadhandler_t)sw_stbmonitor_session_recv_proc, socket, 0 ) ;
	if(m_session.thread)
		sw_thrd_resume(m_session.thread);
	m_session_is_create = true;
	return 0;
}

void sw_stbmonitor_destroy_session()
{
	m_session.socket = -1;
	if(m_session.thread)
		sw_thrd_close(m_session.thread,100);
	m_session.thread = NULL;

	if(m_session.socket>0)
		close(m_session.socket);
	m_session.socket = -1;

	if(m_session.buf)
		free(m_session.buf);
	m_session.buf = NULL;

	if(m_session.decbuf)
		free(m_session.decbuf);
	m_session.decbuf = NULL;

	if(m_session.sendbuf)
		free(m_session.sendbuf);
	m_session.sendbuf = NULL;

	memset( &m_session, 0, sizeof(m_session) );
	m_session_is_create = false;
}

static int sw_stbmonitor_send_reply_data( reply_info_t *info, int need_enc)
{
	int send_size = 0;
	int enc_size = 0;
	int ret = -1;
	if( info==NULL )
		return -1;
	m_session.sendbuf_size = 0; 
	if(info->head_size > 0)
		m_session.sendbuf_size  += info->head_size; 
	if(info->data_size > 0)
		m_session.sendbuf_size  += info->data_size;
	m_session.sendbuf_size  += 16;//采用aes 128bit加密
	m_session.sendbuf = malloc(m_session.sendbuf_size);

	if( m_session.sendbuf==NULL )
	{
		SWSTBMONITOR_LOG_DEBUG("buf is NULL\n");
		goto EXIT;
	}
	memset(m_session.sendbuf,0,m_session.sendbuf_size);
	if( info->head && info->head_size > 0 )
	{
		memcpy(m_session.sendbuf,info->head,info->head_size);	
		send_size += info->head_size;
	}
	if( info->data && info->data_size > 0 )
	{
		memcpy(m_session.sendbuf+send_size,info->data,info->data_size);
		send_size += info->data_size;
	}
	if( need_enc == 1 )
	{
		m_session.encbuf_size = m_session.sendbuf_size;
		m_session.encbuf = malloc(m_session.encbuf_size);
		if( m_session.encbuf==NULL )
		{
			SWSTBMONITOR_LOG_DEBUG("buf is NULL\n");
			goto EXIT;
		}
		memset(m_session.encbuf,0,m_session.encbuf_size);

		enc_size = sw_stbmonitor_aes_enc(m_session.encbuf,m_session.sendbuf,send_size);
		if( enc_size <= 0 )
			goto EXIT;
		ret = send( m_session.socket, m_session.encbuf, enc_size, 0 );
	}
	else
		ret = send( m_session.socket, m_session.sendbuf, send_size, 0 );
	if( ret <= 0 )
		SWSTBMONITOR_LOG_DEBUG("send error\n");
	else
		ret = 0;
EXIT:
	if( m_session.sendbuf )
	{
		free(m_session.sendbuf);
		m_session.sendbuf=NULL;
	}
	if( m_session.encbuf )
	{
		free(m_session.encbuf);
		m_session.encbuf=NULL;
	}
	return ret;

}

static int sw_stbmonitor_send_reply( int reply_code, char *function_call, char *call_value,  char *data )
{
	char buf[2048];
	int len = 0;
	reply_info_t reply;
	if(function_call == NULL)
		return -1;
	memset(buf,0,sizeof(buf));
	memset(&reply,0,sizeof(reply_info_t));
	len = snprintf(buf,sizeof(buf),"%d%s",reply_code,function_call);
	if(call_value)
		len += snprintf(buf+len,sizeof(buf)-len,"^%s",call_value);
	if(data)
		len += snprintf(buf+len,sizeof(buf)-len,"^%s",data);
	reply.head = buf;
	reply.head_size = len;
	reply.data = NULL;
	reply.data_size = 0;
	sw_stbmonitor_send_reply_data( &reply, 1 );
	return 0;
}

static int sw_stbmonitor_send_reply_code(const char *buf, int code)
{
	if (NULL == buf)
	{
		return -1;
	}
	unsigned char rsa_buf[256] = {0};
	char reply_buf[512] = {0};
	int buf_size = 0;
	reply_info_t reply;
	memset(&reply, 0, sizeof(reply));
	if(strstr(buf,"initialize_sunniwell"))
		buf_size = sw_stbmonitor_aes_create_key(rsa_buf,SW_STBMONITOR_RSA_SUNNIWELL);
	else
		buf_size = sw_stbmonitor_aes_create_key(rsa_buf,SW_STBMONITOR_RSA_HUAWEI);
	if( buf_size <= 0 )
		return -1;
	memcpy(reply_buf,rsa_buf,buf_size);
	buf_size += snprintf( reply_buf+buf_size,sizeof(reply_buf)-buf_size,"%dinitialize", code);
	reply.head = reply_buf;
	reply.head_size = buf_size;
	reply.data = NULL;
	reply.data_size = 0;
	sw_stbmonitor_send_reply_data( &reply, 0);
	return 0;
}

static bool sw_stbmonitor_session_recv_proc( unsigned long wParam, unsigned long lParam )
{
	int socket = (int)wParam;
	int recv_size = -1;
	int dec_size = -1;
	int ret = -1;
	struct timeval timeout = {3600,0};//recv接收超时事件，3600秒
	int keepalive = 1; // 开启keepalive属性
	int keepidle = 5; // 如该连接在5秒内没有任何数据往来,则进行探测
	int keepinterval = 1; // 探测时发包的时间间隔为5 秒
	int keepcount = 5; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	if(m_session.buf == NULL)
	{
		m_session.buf = malloc( RECV_BUF_SIZE );
		if( m_session.buf==NULL )
		{
			SWSTBMONITOR_LOG_DEBUG("buf is NULL\n");
			sw_thrd_delay(100);
			return true;
		}
	}
	if(m_session.decbuf == NULL)
	{
		m_session.decbuf = malloc( DEC_BUF_SIZE );
		if( m_session.decbuf==NULL )
		{
			SWSTBMONITOR_LOG_DEBUG("buf is NULL\n");
			sw_thrd_delay(100);
			return true;
		}
	}
	memset(m_session.buf,0,RECV_BUF_SIZE);
	memset(m_session.decbuf,0,DEC_BUF_SIZE);
	setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
	setsockopt(socket, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
	setsockopt(socket, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
	setsockopt(socket, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));
	setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
	recv_size = recv( socket, m_session.buf, RECV_BUF_SIZE, 0 );
	if (-1 == recv_size && errno == EAGAIN)
	{
		//连接一小时无数据，关闭远程连接开关
		SWSTBMONITOR_LOG_DEBUG("haven't received data for 1 hour ,close the remote connetion\n");
		if ( NULL != m_cbfs && NULL != m_cbfs->fcb_on_stb_close_remote_connect)
		{
			m_cbfs->fcb_on_stb_close_remote_connect();
		}
	}
	if(recv_size > 0)
	{
		if(strstr(m_session.buf,"initialize"))
		{
			ret = sw_stbmonitor_on_initialize_request(m_session.buf,recv_size);
		}
		else
		{
			dec_size = sw_stbmonitor_aes_dec( m_session.decbuf, m_session.buf,recv_size );
			if( dec_size>0 )
			{
				ret = sw_stbmonitor_on_data( m_session.decbuf,dec_size );
			}
		}
	}
	if(-1 == ret)
	{
		goto ERROR;	
	}
	else
	{
		return true;
	}
ERROR:
	sw_stbmonitor_destroy_session();
	return false;
}

static int sw_stbmonitor_add_lock_ip()
{
	time_t now = 0;
	time(&now);
	memset(lock_ip_info[m_lock_ip_count].ip, 0, sizeof(lock_ip_info[m_lock_ip_count].ip));
	strlcpy(lock_ip_info[m_lock_ip_count].ip, sw_stbmonitor_get_ip(), sizeof(lock_ip_info[0].ip));
	lock_ip_info[m_lock_ip_count].start_time = now;
	SWSTBMONITOR_LOG_DEBUG("lock_ip:%s start_time:%d count:%d...\n", lock_ip_info[m_lock_ip_count].ip, lock_ip_info[m_lock_ip_count].start_time, m_lock_ip_count);
	m_lock_ip_count++;

	if(m_lock_ip_count >= MAX_LOCK_IP - 1)
	{
		m_lock_ip_count = 0;
		//被锁定的ip达到最大数目，关闭远程连接开关
		if ( NULL != m_cbfs && NULL != m_cbfs->fcb_on_stb_close_remote_connect)
		{
			m_cbfs->fcb_on_stb_close_remote_connect();
		}
	}
	return 0;
}

static bool stbmonitor_check_lock_ip(const char* buf)
{
	if (NULL == buf)
	{
		return -1;
	}
	int i = 0;
	int count = 0;
	time_t now = 0;
	time(&now);
	for(i=0; i<m_lock_ip_count; i++)
	{
		if(lock_ip_info[i].ip && lock_ip_info[i].ip[0] != '\0' && lock_ip_info[i].start_time >= 0)
		{
			//达到锁定时间，解锁
			if(now - lock_ip_info[i].start_time >= LOCK_TIME)
			{
				SWSTBMONITOR_LOG_DEBUG("The %dth unlock ip:%s,now:%d, start_time:%d...\n", i, lock_ip_info[i].ip,now, lock_ip_info[i].start_time);
				memset(&lock_ip_info[i], 0, sizeof(lock_ip_info)/MAX_LOCK_IP);
				count++;
			}
		}
	}
	SWSTBMONITOR_LOG_DEBUG("Total lock:%d, unlock:%d\n", m_lock_ip_count, count);
	for(i=0; i<(m_lock_ip_count - count); i++)
	{
		if(lock_ip_info[i+count].ip && lock_ip_info[i+count].ip[0] != '\0' && count > 0)
		{
			memmove(&lock_ip_info[i], &lock_ip_info[i+count], sizeof(lock_ip_info)/MAX_LOCK_IP);
			memset(&lock_ip_info[i+count], 0, sizeof(lock_ip_info)/MAX_LOCK_IP);
			SWSTBMONITOR_LOG_DEBUG( "=======================[%d][%s]====================\n", i, lock_ip_info[i].ip);
		}
	}
	m_lock_ip_count = m_lock_ip_count - count;
	for(i=0; i<m_lock_ip_count; i++)
	{
		if(lock_ip_info[i].ip[0] != '\0' && strcmp(lock_ip_info[i].ip, sw_stbmonitor_get_ip()) == 0)
		{
			SWSTBMONITOR_LOG_DEBUG("This ip:[%s] is in locking!\n", sw_stbmonitor_get_ip());
			//非法操作，用户被锁定
			sw_stbmonitor_send_reply_code(buf, STBMONITOR_REPLY_LOCKED);
			return false;
		}
	}
	return true;
}

static int stbmonitor_lock_by_ip(const char * buf)
{
	if (NULL == buf)
	{
		return -1;
	}
	if(0 == m_fail_times)
	{
		m_fail_times++;
		memset(m_monitor_ip, 0, sizeof(m_monitor_ip));
		strlcpy(m_monitor_ip, sw_stbmonitor_get_ip(), sizeof(m_monitor_ip));
	}
	else
	{
		if( strncmp(m_monitor_ip, sw_stbmonitor_get_ip(), strlen(m_monitor_ip)) == 0 )
		{
			m_fail_times++;
			//同一ip连接失败达到3次锁定ip
			if( m_fail_times >= 3)
			{
				sw_stbmonitor_add_lock_ip();
				//非法操作，用户ip被锁定
				sw_stbmonitor_send_reply_code(buf, STBMONITOR_REPLY_LOCKED);
				m_fail_times = 0;
			}
		}
		else
		{
			m_fail_times = 1;
			memset(m_monitor_ip, 0, sizeof(m_monitor_ip));
			strlcpy(m_monitor_ip, sw_stbmonitor_get_ip(), sizeof(m_monitor_ip));
		}
	}
	SWSTBMONITOR_LOG_DEBUG("failed times : %d\n", m_fail_times);
	return 0;
}

static int sw_stbmonitor_on_initialize_request( char *buf, int size )
{	
	if (NULL == buf)
	{
		return -1;
	}
	if( false == stbmonitor_check_lock_ip(buf) )
	{
		return -1;
	}
	char value[64];
	char sessionID[32];
	char identify_code[16];
	unsigned char rsa_buf[256];
	char reply_buf[512];
	int buf_size = 0;

	reply_info_t reply;
	memset(value, 0 ,sizeof(value));
	memset(&reply,0,sizeof(reply_info_t));
	memset( identify_code,0,sizeof(identify_code) );
	memset( sessionID,0,sizeof(sessionID) );
	memset(rsa_buf,0,sizeof(rsa_buf));
	memset(reply_buf,0,sizeof(reply_buf));

	memcpy( identify_code, buf,8 );
	sw_stbmonitor_get_sessionid(sessionID,16);//sessionID:两端用户名和密码

	strlcpy(value, m_first_connected_ip, sizeof(m_first_connected_ip));
	if (0 == strlen(value) || '\0' == value[0])
	{
		SWSTBMONITOR_LOG_DEBUG("monitor first connect !\n");
	}
	else
	{
		//C30基线远程连接开启周期只允许首次连接上的ip进行连接
		if(strcmp(value, sw_stbmonitor_get_ip()) != 0)
		{
			SWSTBMONITOR_LOG_DEBUG("m_first_connected_ip:%s, monitor_ip:%s\n", value, sw_stbmonitor_get_ip());
			//开启周期内只允许一个ip连接，其他ip要被锁定，超过10个自动关闭远程连接
			sw_stbmonitor_add_lock_ip();
			sw_stbmonitor_send_reply_code(buf, STBMONITOR_REPLY_LOCKED);
			return -1;
		}
	}
	if(false == sw_stbmonitor_check_identify_code(identify_code, sessionID))
	{
		stbmonitor_lock_by_ip(buf);
		//非法连接，用户或密码错误
		sw_stbmonitor_send_reply_code(buf, STBMONITOR_REPLY_IDE_CODE_ILLEGAL);
		return -1;
	}
	else
	{
		m_fail_times = 0;
		//保存第一次连接的ip
		strlcpy(m_first_connected_ip, sw_stbmonitor_get_ip(), sizeof(m_first_connected_ip));
	}
	if(strstr(buf,"initialize_sunniwell"))
		buf_size = sw_stbmonitor_aes_create_key(rsa_buf,SW_STBMONITOR_RSA_SUNNIWELL);
	else
		buf_size = sw_stbmonitor_aes_create_key(rsa_buf,SW_STBMONITOR_RSA_HUAWEI);
	if( buf_size <= 0 )
	{
		return -1;
	}
	memcpy(reply_buf,rsa_buf,buf_size);
	buf_size += snprintf( reply_buf+buf_size,sizeof(reply_buf)-buf_size,"%dinitialize",STBMONITOR_REPLY_OK);
	reply.head = reply_buf;
	reply.head_size = buf_size;
	reply.data = NULL;
	reply.data_size = 0;
	sw_stbmonitor_send_reply_data( &reply, 0);
	CONNECT_LOG_DEBUG("stbmonitor connect!");
	return 0;
}

static int sw_stbmonitor_on_data( char *buf, int size )
{
	if (NULL == buf)
	{
		return -1;
	}
	char sessionID[32];
	char identify_code[16];
	char *p = NULL;
	packet_info_t info; 
	if( buf==NULL || size<16 )
		return -1;

	if( strstr(buf,"^") == NULL )
		return -1;
	memset( sessionID,0,sizeof(sessionID) );
	memset( identify_code,0,sizeof(identify_code) );
	memset( &info,0,sizeof(packet_info_t) );
	memcpy( identify_code, buf, 8 );
	memcpy( sessionID, buf+8, 16 );

	if(false == sw_stbmonitor_check_identify_code(identify_code, sessionID))
		return -1;

	p = buf+24;
	if( -1 == sw_stbmonitor_packet_parse(&info,p,size-24) )
		return -1;

	if( info.function_call == STBMONITOR_FUNCTION_CALL_READ )	
		sw_stbmonitor_handle_function_read( &info );
	else if( info.function_call == STBMONITOR_FUNCTION_CALL_WRITE )	
		sw_stbmonitor_handle_function_write( &info );
	else if( info.function_call == STBMONITOR_FUNCTION_CALL_IOCTL )	
		sw_stbmonitor_handle_function_ioctl( &info );
	else if( info.function_call == STBMONITOR_FUNCTION_CALL_INFORM )	
		sw_stbmonitor_handle_function_inform( &info );
	else if( info.function_call == STBMONITOR_FUNCTION_CALL_CONNECT )	
		sw_stbmonitor_handle_function_connect( &info );
	return 0;
}

static int sw_stbmonitor_packet_parse(packet_info_t *info, char *buf, int size)
{
	if( info == NULL || buf == NULL || size <=0 )	
		return -1;
	char* p = NULL;
	char* start = buf;
	char* end = buf+strlen(buf);

	memset(info, 0, sizeof(packet_info_t));
	//因为传入的buf已经将头24个字节过滤掉，所以直接解析function call
	if(strncmp(start, "read", strlen("read")) == 0)
		info->function_call = STBMONITOR_FUNCTION_CALL_READ;		
	else if(strncmp(start, "write", strlen("write")) == 0)
		info->function_call = STBMONITOR_FUNCTION_CALL_WRITE;		
	else if(strncmp(start, "ioctl", strlen("ioctl")) == 0)
		info->function_call = STBMONITOR_FUNCTION_CALL_IOCTL;		
	else if(strncmp(start, "inform", strlen("inform")) == 0)
		info->function_call = STBMONITOR_FUNCTION_CALL_INFORM;		
	else if(strncmp(start, "connect", strlen("connect")) == 0)
		info->function_call = STBMONITOR_FUNCTION_CALL_CONNECT;	

	p = strchr( buf,'^');//FunctionCall^
	if(!p)
		return -1;//TODO

	start = p+1;
	p = strchr( start, '^' );//CallValue^
	if(!p)
		return -1;//TODO
	memcpy(info->call_value,start,p-start);

	start = p+1;
	p = strchr( start, '^' );//Parameter^
	if(!p)
	{
		memcpy(info->parameter,start,end-start);
		return 0;
	}
	else
		memcpy(info->parameter,start,p-start);

	start = p+1;//Data\0
	if(end-start > 0)
		memcpy(info->data,start,end-start);

	return 0;
}

static int sw_stbmonitor_handle_function_read( packet_info_t *info )
{
	int read_size = 0;
	char *buf = NULL;
	int buf_size = 0;
	reply_info_t reply;

	char value[2048];

	if( info == NULL )
		return -1;
	if( strcmp(info->call_value, "Paralist") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_read_all_parameter != NULL )
		{
			buf_size = 16*1024;
			buf = malloc(buf_size);
			if(buf == NULL)
				return -1;
			memset(buf,0,buf_size);

			read_size = m_cbfs->fcb_on_read_all_parameter(buf,buf_size);
			if(read_size <= 0)
			{
				sw_stbmonitor_send_reply( STBMONITOR_REPLY_PARA_ILLEGAL, "read", info->call_value, "null" );
				if(buf)
					free(buf);
				return -1;
			}
			else
			{
				char head[] = "200read^Paralist^";
				int head_size = strlen(head);

				memset(&reply,0,sizeof(reply_info_t));
				reply.head = head;
				reply.head_size = head_size;
				reply.data = buf;
				reply.data_size = read_size;
				sw_stbmonitor_send_reply_data( &reply, 1 );
				if(buf)
					free(buf);
				return 0;
			}
		}
		else
			sw_stbmonitor_send_reply( STBMONITOR_REPLY_PARA_ILLEGAL, "read", info->call_value, "null" );
	}
	else if( strcmp(info->call_value, "Channellist") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_read_channellist != NULL )
		{
			//TODO
			buf_size = 128*1024;
			buf = malloc(buf_size);
			if(buf == NULL)
				return -1;
			memset(buf,0,buf_size);

			read_size = m_cbfs->fcb_on_read_channellist(buf,buf_size);
			if(read_size <= 0)
			{
				sw_stbmonitor_send_reply( STBMONITOR_REPLY_PARA_ILLEGAL, "read", info->call_value, "null" );
				if(buf)
					free(buf);
				return -1;
			}
			else
			{
				char head[] = "200read^Channellist^";
				int head_size = strlen(head);

				memset(&reply,0,sizeof(reply_info_t));

				reply.head = head;
				reply.head_size = head_size;
				reply.data = buf;
				reply.data_size = read_size;
				sw_stbmonitor_send_reply_data( &reply, 1 );
				if(buf)
					free(buf);
				return 0;
			}
		}
		else
			sw_stbmonitor_send_reply( STBMONITOR_REPLY_PARA_ILLEGAL, "read", info->call_value, "null" );
		
	}
	else
	{
		memset(value,0,sizeof(value));
		if( m_cbfs != NULL && m_cbfs->fcb_on_read_parameter != NULL )
		{
			read_size = m_cbfs->fcb_on_read_parameter(info->call_value, value,sizeof(value));
			if(read_size <= 0)
			{
				sw_stbmonitor_send_reply( STBMONITOR_REPLY_PARA_ILLEGAL, "read", info->call_value, "null" );
				return -1;
			}
			else
			{
				char head[64];
				int head_size = 0;
				memset(head,0,sizeof(head));
				memset(&reply,0,sizeof(reply_info_t));
				head_size = snprintf(head,sizeof(head),"200read^%s^",info->call_value);
				reply.head = head;
				reply.head_size = head_size;
				reply.data = value;
				reply.data_size = read_size;
				sw_stbmonitor_send_reply_data( &reply, 1 );
				return 0;	
			}
		}
		else
			sw_stbmonitor_send_reply( STBMONITOR_REPLY_PARA_ILLEGAL, "read", info->call_value, "null" );

	}
	return 0;
}

static int sw_stbmonitor_handle_function_write( packet_info_t *info )
{
	int ret = -1;
	if( info == NULL )
		goto EXIT;			
	//如果需要可以解析info->parameter 为/s是否立即保存
	if( m_cbfs != NULL && m_cbfs->fcb_on_write_parameter != NULL )
		ret = m_cbfs->fcb_on_write_parameter(info->call_value,info->data,1);
EXIT:
	if(ret == 0)
		sw_stbmonitor_send_reply( STBMONITOR_REPLY_OK,"write", NULL, NULL );
	else
		sw_stbmonitor_send_reply( STBMONITOR_REPLY_PARA_ILLEGAL, "write", info->call_value, NULL );
	return 0;
}

static int sw_stbmonitor_handle_function_ioctl( packet_info_t *info )
{
	int ret = -1;
	char line_buf[16];
	if( info == NULL )
	{
		goto EXIT;
	}
	if( strcasecmp(info->call_value, "upgrade") == 0 )
	{
		if( m_is_stb_sleep == true )
		{
			sw_stbmonitor_send_reply( STBMONITOR_REPLY_CMD_ILLEGAL, "ioctl", NULL, NULL );
			return 0;
		}
		int upgrade_port = 0;
		char portbuf[32];
		memset(portbuf,0,sizeof(portbuf));
		if( strncmp(info->parameter, "/f", 2)==0 )
			sw_stbmonitor_set_upgrade_force( true );
		sw_stbmonitor_set_upgrade_reply_callback( sw_stbmonitor_send_reply );
		if(m_session.type == 1)
		{
			snprintf(portbuf,sizeof(portbuf),"%d",9004);
			sw_stbmonitor_send_reply( 200, "ioctl", "upgrade", portbuf );
			sw_thrd_delay(3500);
			upgrade_port = sw_stbmonitor_upgrade_connect();
			if(upgrade_port > 0)
				return 0;
		}
		else
		{
			upgrade_port = sw_stbmonitor_upgrade_server_create(m_cbfs);
			if(upgrade_port > 0)
			{
				snprintf(portbuf,sizeof(portbuf),"%d",upgrade_port);
				sw_stbmonitor_send_reply( 200, "ioctl", "upgrade", portbuf );
				return 0;
			}
		}
	}
	else if( strcasecmp(info->call_value, "reboot") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_stb_reboot != NULL )
			ret = m_cbfs->fcb_on_stb_reboot();
	}
	else if( strcasecmp(info->call_value, "upgrade_online") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_upgrade_online != NULL)
			ret = m_cbfs->fcb_on_upgrade_online();
	}
	else if( strcasecmp(info->call_value, "restore_setting") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_restore_setting != NULL )
			ret = m_cbfs->fcb_on_restore_setting();
	}
	else if( strcasecmp(info->call_value, "set_test_mode") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_set_test_mode != NULL )
			ret = m_cbfs->fcb_on_set_test_mode();
	}
	else if( strcasecmp(info->call_value, "set_autotest_mode") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_set_autotest_mode != NULL )
			ret = m_cbfs->fcb_on_set_autotest_mode();
	}
	else if( strcasecmp(info->call_value, "set_work_mode") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_set_work_mode != NULL )
			ret = m_cbfs->fcb_on_set_work_mode();
	}
	else if( strcasecmp(info->call_value, "set_scriptrecord_mode") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_set_scriptrecord_mode != NULL )
			ret = m_cbfs->fcb_on_set_scriptrecord_mode();
	}
	else if( strcasecmp(info->call_value, "set_log_out_type") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_set_log_out_type != NULL )
			ret = m_cbfs->fcb_on_set_log_out_type( info->data );
	}
	else if( strcasecmp(info->call_value, "set_log_level") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_set_log_level!= NULL )
			ret = m_cbfs->fcb_on_set_log_level( info->data );
	}
	else if( strcasecmp(info->call_value, "set_log_type") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_set_log_type!= NULL )
			ret = m_cbfs->fcb_on_set_log_type( info->data );
	}
	else if( strcasecmp(info->call_value, "remotepcap") == 0 )
	{
		sw_stbmonitor_remotepcap_start( m_cbfs, info->parameter, info->data );
		ret = 0;
	}

	else if( strcasecmp(info->call_value, "startDebugInfo") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_startDebugInfo != NULL )
			ret = m_cbfs->fcb_on_startDebugInfo(info->data);
	}
	else if( strcasecmp(info->call_value, "stopDebugInfo") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_stopDebugInfo != NULL )
			ret = m_cbfs->fcb_on_stopDebugInfo(info->data);
	}
	else if( strcasecmp(info->call_value, "UploadDebugInfo") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_uploadDebugInfo != NULL )
			ret = m_cbfs->fcb_on_uploadDebugInfo(info->data);
	}
	else if( strcasecmp(info->call_value, "starStartupInfo") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_startBootDebugInfo != NULL )
			ret = m_cbfs->fcb_on_startBootDebugInfo(info->data);
	}
	else if( strcasecmp(info->call_value, "stopStartupInfo") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_stopBootDebugInfo != NULL )
			ret = m_cbfs->fcb_on_stopBootDebugInfo(info->data);
	}
	else if( strcasecmp(info->call_value, "UploadStartupInfo") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_uploadBootDebugInfo != NULL )
			ret = m_cbfs->fcb_on_uploadBootDebugInfo(info->data);
	}
#ifndef SUPPORT_TM_8V8 
	else if( strcasecmp(info->call_value, "startChannelZapping") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_startChannelZapping != NULL )
			ret = m_cbfs->fcb_on_startChannelZapping(info->parameter);
	}
	else if( strcasecmp(info->call_value, "stopChannelZapping") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_stopChannelZapping != NULL )
			ret = m_cbfs->fcb_on_stopChannelZapping(info->parameter);
	}
	else if( strcasecmp(info->call_value, "startPipSwapping") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_startPipSwapping != NULL )
			ret = m_cbfs->fcb_on_startPipSwapping(info->parameter);
	}
	else if( strcasecmp(info->call_value, "stopPipSwapping") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_stopPipSwapping != NULL )
			ret = m_cbfs->fcb_on_stopPipSwapping(info->parameter);
	}
	else if( strcasecmp(info->call_value, "startStability") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_startStability != NULL )
			ret = m_cbfs->fcb_on_startStability(info->parameter);
	}
	else if( strcasecmp(info->call_value, "stopStability") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_stopStability != NULL )
			ret = m_cbfs->fcb_on_stopStability(info->parameter);
	}
	else if( strcasecmp(info->call_value, "fileSend") == 0 )
	{
		if( strstr(info->parameter,"numberOfLine") )
		{
			sscanf(info->parameter, "%*[^:]:%[^'NULL']", line_buf);
			m_file_size = atoi(line_buf);
			m_current_size = 0;
			ret = 1;
		}
		else
		{
			if(info->parameter)
			{
				FILE *file_fp = fopen("/tmp/channellist.txt","ab"); 
				if( file_fp )
				{
					fprintf(file_fp,"%s\n",info->parameter);
					fclose(file_fp);
				}
				else
					ret = -1;
			}
			m_current_size++;
			ret = 1;
		}
		if(m_current_size >= m_file_size)
		{
			m_file_size = 0;
			m_current_size = 0;
			ret = m_cbfs->fcb_on_fileSend();
		}
	}
	else if( strcasecmp(info->call_value, "remoteControl") == 0 )
	{
		int keycode = 0;
		char *key_code = NULL;

		key_code = strtok(info->data, ":");
		keycode = atoi(key_code);
		printf("========[%s;%d keycode = %d]=============\n", __func__, __LINE__, keycode);

		if( m_cbfs != NULL && m_cbfs->fcb_on_remote_control != NULL)
		{
			ret = m_cbfs->fcb_on_remote_control(keycode);
		}
	}
#endif
	if(ret >= 0)
	{
		sw_stbmonitor_send_reply( STBMONITOR_REPLY_OK,"ioctl", NULL, NULL );
	}
	else
	{
		sw_stbmonitor_send_reply( STBMONITOR_REPLY_PARA_ILLEGAL, "ioctl", info->call_value, NULL );
	}
	return 0;

EXIT:
	sw_stbmonitor_send_reply( STBMONITOR_REPLY_CMD_ILLEGAL, "ioctl", NULL, NULL );
	return ret;
}

static int sw_stbmonitor_handle_function_inform( packet_info_t *info )
{
	int ret = -1;
	if( info == NULL )
		return -1;
	if( strcasecmp(info->call_value, "set_upgradelength") == 0 )
	{
		sw_stbmonitor_set_upgrade_file_size( atoi(info->data) );
		ret = 200;
	}
	else if( strcasecmp(info->call_value, "get_upgradeprecent") == 0 )
	{
		
	}
	else if( strcasecmp(info->call_value, "pcap_maxfilesize") == 0 )
	{
		sw_stbmonitor_set_remotepcap_reply_callback( sw_stbmonitor_send_reply );
		sw_stbmonitor_remotepcap_get_maxfilesize();
	}
	else if( strcasecmp(info->call_value, "debugInfoStatus") == 0 )
	{
		
	}

	//要求上层返回定义的错误码，执行成功ret返回200
	if(ret > 0)
		sw_stbmonitor_send_reply( ret, "inform", NULL, NULL );
	else
		sw_stbmonitor_send_reply( STBMONITOR_REPLY_CMD_ILLEGAL, "inform", NULL, NULL );
	return ret;
}

static int sw_stbmonitor_handle_function_connect( packet_info_t *info )
{
	int ret = -1;
	if( info == NULL )
		return -1;			
	if( strcasecmp(info->call_value, "play") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_media_play != NULL )
			ret = m_cbfs->fcb_on_media_play(info->data);
		if(ret < 0)
		{
			sw_stbmonitor_send_reply( STBMONITOR_REPLY_CHANNEL_NOT_EXIT, "connect", NULL, NULL );
			return ret;
		}
	}
	else if( strcasecmp(info->call_value, "pause") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_media_pause != NULL )
			ret = m_cbfs->fcb_on_media_pause();
	}
	else if( strcasecmp(info->call_value, "stop") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_media_stop != NULL )
			ret = m_cbfs->fcb_on_media_stop();
	}
	else if( strcasecmp(info->call_value, "fast_forward") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_media_ffwd != NULL )
			ret = m_cbfs->fcb_on_media_ffwd();
	}
	else if( strcasecmp(info->call_value, "fast_backward") == 0 )
	{
		if( m_cbfs != NULL && m_cbfs->fcb_on_media_fbwd != NULL )
			ret = m_cbfs->fcb_on_media_fbwd();
	}
	else if( strcasecmp(info->call_value, "ping") == 0 )
	{
		sw_stbmonitor_set_ping_reply_callback( sw_stbmonitor_send_reply );
		if( strcasecmp(info->parameter, "/stop") == 0 )
			sw_stbmonitor_ping_stop();
		else 
			sw_stbmonitor_ping_start( m_cbfs,info->parameter,info->data );
		ret = 200;	
	}
	else if( strcasecmp(info->call_value, "traceroute") == 0 )
	{
		sw_stbmonitor_set_traceroute_reply_callback( sw_stbmonitor_send_reply );
		if (strcasecmp(info->parameter, "/stop") == 0)
			sw_stbmonitor_traceroute_stop();
		else
			sw_stbmonitor_traceroute_start( m_cbfs, info->data );
		ret = 200;	
	}

	//要求上层返回定义的错误码，执行成功ret返回200
	if(ret >= 0)
		sw_stbmonitor_send_reply( STBMONITOR_REPLY_OK, "connect", NULL, NULL );
	else
		sw_stbmonitor_send_reply( STBMONITOR_REPLY_CMD_ILLEGAL, "connect", NULL, NULL );
	return ret;
}

