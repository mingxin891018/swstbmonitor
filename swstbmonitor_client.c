/***************************************************************************
* Copyright: sunniwell
* file     : nmpserver.c
* AUTHOR   : 吴荣富
* brief    : 华为统一管理工具服务器接口
****************************************************************************/
#include "swstbmonitor_priv.h"
#include "swstbmonitorserver.h"
#include "swnetwork.h"
#include "swthrd.h"

/* 单播接收端口 */
#define SWSTBMONITOR_TCP_PORT	9003
/* 单播升级接收端口 */
#define SWSTBMONITOR_UPGRADE_TCP_PORT	9004

static sw_stbmonitor_callback_funcs_t *m_cbfs = NULL;
static 	uint32_t m_server_ip;
static 	uint32_t m_local_ip;
/* 分析是不是一个有效的点分十进制IP地址 */
static bool IsAddress(char* buf)
{
	int i, j, k;
	int sip;
	j = i = 0 ;
	k = 0;
	//to do 
	//IPv6合法性如何判断需要研究，这里如果检查buf有:认为是ipv6的地址，而且暂且当其是合法的
	if( strchr(buf,':') != 0 )
		return true;
	while (k < 4)
	{
		sip = 0;
		while (buf[i] != '.' && buf[i] != '\0')
		{
			if ( 2 < (i-j) || buf[i] < '0' || '9' < buf[i] )
				return false;
			sip = sip * 10 + buf[i] - '0';
			i++;
		}
		if ( j == i || sip > 255 )/* 没有数据.9..0或者大于255 */
			return false;
		i++;
		j = i;
		++k;
	}
	return (k == 4 && buf[i-1] == '\0' && buf[i-2] != '.') ? true : false;
}

/* 启动机顶盒主动连接流程<UNICAST> */
int sw_stbmonitor_connect(sw_stbmonitor_callback_funcs_t *cbfs, char *server_ip, char *local_ip)
{
	int local_socket = -1;
	int reuse = 1;
	struct sockaddr_in addr;
	if( (server_ip == NULL || !IsAddress(server_ip)) 
		|| (local_ip == NULL || !IsAddress(local_ip)) ) 
	{
		SWSTBMONITOR_LOG_ERROR("Please input correct ip !\n");
		goto ERROR_EXIT;
	}
	m_server_ip = inet_addr(server_ip);
	m_local_ip = inet_addr(local_ip);
	if(cbfs != NULL)
		m_cbfs = cbfs;
	local_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );;
	//允许端口复用
	if(setsockopt( local_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse) ) < 0)
	{	
		SWSTBMONITOR_LOG_DEBUG("setsockopt reuse failed...\n");
		goto ERROR_EXIT;
	}
	if( local_socket < 0 )
	{
		SWSTBMONITOR_LOG_DEBUG("create tcp socket failed!\n");
		goto ERROR_EXIT;
	}
	int flags = fcntl(local_socket, F_GETFL, 0);
	fcntl(local_socket, F_SETFL, flags | O_NONBLOCK);
	memset( &addr, 0, sizeof(addr) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = m_server_ip;
	addr.sin_port = htons(SWSTBMONITOR_TCP_PORT);
	if( connect( local_socket, (struct sockaddr *)&addr, sizeof(addr) ) < 0 )
	{
		fd_set rset;
		struct timeval timeout={5,0};

		FD_ZERO(&rset);
		FD_SET(local_socket, &rset); 
		int iRet1 = select(local_socket+1, NULL, &rset, NULL, &timeout);  
		if (iRet1 < 0 || !FD_ISSET(local_socket, &rset))
		{
			SWSTBMONITOR_LOG_DEBUG("connect socket to server address %s failed! ret=%d\n",server_ip,iRet1);
			goto ERROR_EXIT;
		}
	}
	int error=-1;
	int len=sizeof(int);
	//当ip地址可达的时候，但是端口不可以用的情况

	if(getsockopt(local_socket, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len)!=0)
	{
		SWSTBMONITOR_LOG_DEBUG("getsockopt failed\n");
		//goto ERROR_EXIT;
	}
	if(error)
	{
		SWSTBMONITOR_LOG_DEBUG("connect socket to server address %s failed! error=%d\n",server_ip,error);
		goto ERROR_EXIT;
	}

	if(fcntl(local_socket, F_SETFL, flags & ~O_NONBLOCK)<0)
			SWSTBMONITOR_LOG_DEBUG("fcntl error\n");
	sw_stbmonitor_create_session( local_socket,m_cbfs,1 );

	return 0;

ERROR_EXIT:
	if( 0 <= local_socket )
	{
		close(local_socket);
		local_socket = -1;
	}
	return -1;
}

/* 启动机顶盒主动连接流程<UNICAST> */
int sw_stbmonitor_upgrade_connect()
{
	int reuse = 1;
	int local_socket = -1;
	struct sockaddr_in addr;
	local_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );;
	if( local_socket < 0 )
	{
		SWSTBMONITOR_LOG_DEBUG("create tcp socket failed!\n");
		goto ERROR_EXIT;
	}
	/* 配置为非阻塞 */
	if ( setsockopt(local_socket , SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0 )
	{
		SWSTBMONITOR_LOG_DEBUG("unix socket setsockopt failed\n");
		goto ERROR_EXIT;
	}
	memset( &addr, 0, sizeof(addr) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = m_local_ip;
	addr.sin_port = htons(SWSTBMONITOR_UPGRADE_TCP_PORT);
	if( bind( local_socket, (struct sockaddr*)&addr, sizeof(addr) ) < 0 )
	{
		SWSTBMONITOR_LOG_DEBUG("bind socket to local address failed!\n");
		goto ERROR_EXIT;
	}
	memset( &addr, 0, sizeof(addr) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = m_server_ip;
	addr.sin_port = htons(SWSTBMONITOR_UPGRADE_TCP_PORT);
	if( connect( local_socket, (struct sockaddr *)&addr, sizeof(addr) ) < 0 )
	{
		SWSTBMONITOR_LOG_DEBUG("connect socket to server address failed!\n");
		goto ERROR_EXIT;
	}

	sw_stbmonitor_create_upgrade_session( local_socket,m_cbfs );

	return SWSTBMONITOR_UPGRADE_TCP_PORT;

ERROR_EXIT:
	if( 0 <= local_socket )
	{
		close(local_socket);
		local_socket = -1;
	}
	return -1;
}
