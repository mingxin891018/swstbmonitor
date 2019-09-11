/***************************************************************************
* Copyright: sunniwell
* file     : nmpserver.c
* AUTHOR   : 吴荣富
* brief    : 华为统一管理工具服务器接口
****************************************************************************/
#include "swstbmonitor_priv.h"
#include "swstbmonitorserver.h"
#include "swthrd.h"
/* 单播接收端口 */
#define SWSTBMONITOR_UPGRADE_TCP_PORT	9004
/*最多的socket数*/
#define MAXSOCKFD 10

static HANDLE m_server_thread = NULL;
static int m_server_socket = -1;
static bool sw_stbmonitor_upgrade_server_proc( unsigned long wParam, unsigned long lParam );

static const char* ip2str(uint32_t ip,char* buf,int size)
{
	memset(buf,0,size);
	//return inet_ntoa(*((struct in_addr*)(&ip)));
	inet_ntop( AF_INET, &ip, buf, size);
	return buf;
}
/* 启动TCP网管服务<UNICAST> */
int sw_stbmonitor_upgrade_server_create(sw_stbmonitor_callback_funcs_t *cbfs)
{
	int reuse = 1;
	struct sockaddr_in addr;
	//已经创建了，返回端口号
	if(m_server_socket > 0)
		return SWSTBMONITOR_UPGRADE_TCP_PORT;

	m_server_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );;
	if( m_server_socket < 0 )
	{
		SWSTBMONITOR_LOG_DEBUG("create tcp socket failed!\n");
		goto ERROR_EXIT;
	}
	/* 配置为非阻塞 */
	if ( setsockopt(m_server_socket , SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0 )
	{
		SWSTBMONITOR_LOG_DEBUG("unix socket setsockopt failed\n");
		goto ERROR_EXIT;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(SWSTBMONITOR_UPGRADE_TCP_PORT);
	if( bind( m_server_socket, (struct sockaddr*)&addr, sizeof(addr) ) < 0 )
	{
		SWSTBMONITOR_LOG_DEBUG("bind socket to local address failed!\n");
		goto ERROR_EXIT;
	}

	listen( m_server_socket, 1 );

	m_server_thread = sw_thrd_open( "tStbMonitorUpgradeServer", 10, 0, 128*1024, (threadhandler_t)sw_stbmonitor_upgrade_server_proc, (unsigned long)m_server_socket, (unsigned long)cbfs);
	if( m_server_thread == NULL )
	{
		SWSTBMONITOR_LOG_DEBUG("cannot create tcp recv task!\n");
		goto ERROR_EXIT;
	}
	sw_thrd_resume(m_server_thread);
	return SWSTBMONITOR_UPGRADE_TCP_PORT;

ERROR_EXIT:
	sw_stbmonitor_upgrade_server_destroy();
	return -1;
}

/* 关闭TCP管理工具服务 */
void sw_stbmonitor_upgrade_server_destroy()
{
	if(m_server_thread)
	{
		sw_thrd_close( m_server_thread, 30 );
		m_server_thread = NULL;
	}
	if( 0 < m_server_socket )
	{
		close(m_server_socket);
		m_server_socket = -1;
	}
}

static bool sw_stbmonitor_upgrade_server_proc( unsigned long wParam, unsigned long lParam )
{
	int server_accept_socket = -1;
	struct sockaddr_in from;
	char buf[64];
	unsigned int slen = sizeof(from);
	unsigned long accept_ip=0;
	unsigned short accept_port=0;
	sw_stbmonitor_callback_funcs_t *cbfs = (sw_stbmonitor_callback_funcs_t*)lParam;
	server_accept_socket = accept( m_server_socket, (struct sockaddr *)&from, &slen );
	if(server_accept_socket >= 0 )
	{
		accept_ip = from.sin_addr.s_addr;
		accept_port = from.sin_port;
		SWSTBMONITOR_LOG_DEBUG( "accept with %s:%d\n",ip2str(accept_ip,buf,sizeof(buf)),htons(accept_port) );
		sw_stbmonitor_create_upgrade_session(server_accept_socket,cbfs);
	}
	sw_thrd_delay(100);
	return true;	
}
