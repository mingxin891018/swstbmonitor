/***************************************************************************
* Copyright: sunniwell
* file     : nmpserver.c
* AUTHOR   : 吴荣富
* brief    : 华为统一管理工具服务器接口
****************************************************************************/
#include "swstbmonitor_priv.h"
#include "swstbmonitorserver.h"
#include "swthrd.h"
#define RECV_BUF_SIZE 32*1024
typedef struct
{
	HANDLE thread;
	int socket;
	unsigned char *buf;
	int buf_size;
}session_info_t;

typedef struct
{
	int file_size;
	int recv_total_size;
	bool force;
}upgrade_info_t;

static bool m_session_is_create = false;
static upgrade_info_t m_upgrade;
static session_info_t m_session;
static sw_stbmonitor_callback_funcs_t *m_cbfs = NULL;
static sw_stbmonitor_reply_callback m_cbfs_reply = NULL;

static bool sw_stbmonitor_upgrade_session_recv_proc( unsigned long wParam, unsigned long lParam );

int sw_stbmonitor_set_upgrade_file_size( int size )
{
	//升级都是先设置长度，所以可以在这里初始化
	memset(&m_upgrade,0,sizeof(upgrade_info_t));
	m_upgrade.file_size = size;
	return 0;
}

int sw_stbmonitor_set_upgrade_force( bool force )
{
	m_upgrade.force = force;
	return 0;
}

int sw_stbmonitor_set_upgrade_reply_callback( sw_stbmonitor_reply_callback cbfs_reply )
{
	m_cbfs_reply = cbfs_reply;
	return 0;
}

//只支持一个session
int sw_stbmonitor_create_upgrade_session(int sockfd, sw_stbmonitor_callback_funcs_t *cbfs)
{
	if(m_session_is_create == true)
		return -1;
	memset( &m_session, 0, sizeof(m_session) );
	if(cbfs != NULL)
		m_cbfs = cbfs;
	m_session.socket = sockfd;
	m_session.thread = sw_thrd_open( "tStbMonitorUpgSession", 80, 0, 16384, (threadhandler_t)sw_stbmonitor_upgrade_session_recv_proc, sockfd, 0 ) ;
	if(m_session.thread)
		sw_thrd_resume(m_session.thread);
	m_session_is_create = true;
	return 0;
}

void sw_stbmonitor_destroy_upgrade_session()
{
	if(m_session.thread)
		sw_thrd_close(m_session.thread,100);
	m_session.thread = NULL;

	if(m_session.socket>0)
		close(m_session.socket);
	m_session.socket = -1;

	if(m_session.buf)
		free(m_session.buf);
	m_session.buf = NULL;

	memset( &m_session, 0, sizeof(m_session) );
	m_session_is_create = false;
}

static int m_upgrade_flag = 0;
static bool sw_stbmonitor_upgrade_session_recv_proc( unsigned long wParam, unsigned long lParam )
{
	int sktfd = (int)wParam;
	int recv_size = -1;
	int ret = -1;
	if(m_session.buf == NULL)
	{
		m_session.buf = (unsigned char*)malloc( RECV_BUF_SIZE );
		if( m_session.buf==NULL )
		{
			SWSTBMONITOR_LOG_DEBUG("buf is NULL\n");
			sw_thrd_delay(100);
			return true;
		}
	}
	memset(m_session.buf,0,RECV_BUF_SIZE);

    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(sktfd, &rfds);

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    retval = select(sktfd+1, &rfds, NULL, NULL, &tv);
    if (retval == -1)
    {
        perror("select()");
		SWSTBMONITOR_LOG_DEBUG("-------select failed--\n");
        //system error onevent();
		m_upgrade_flag = 1;
        goto EXIT;
    }
    else if(retval)
    {
        recv_size = recv( sktfd, m_session.buf, RECV_BUF_SIZE, 0 );
        if(recv_size > 0)
        {
            m_upgrade.recv_total_size += recv_size;
            if( m_cbfs != NULL && m_cbfs->fcb_on_upgrade_data != NULL )
                ret = m_cbfs->fcb_on_upgrade_data( m_session.buf, recv_size, m_upgrade.file_size, m_upgrade.force );
            if( ret != 0 )
                goto EXIT;
            return true;
        }
    }
    else
    {
		SWSTBMONITOR_LOG_DEBUG("-------timeout--\n");
        //timeout onevent();
		m_upgrade_flag = 1;
        goto EXIT;
    }


	if(m_upgrade.recv_total_size == m_upgrade.file_size)
		ret =  STBMONITOR_REPLY_UPGRADE_FINISH;
	else
		ret = STBMONITOR_REPLY_CHECK_VERSION_ERR;
EXIT:

	if( m_cbfs_reply != NULL )
		m_cbfs_reply( ret, "upgrade", NULL, NULL );

	if( m_cbfs != NULL && m_cbfs->fcb_on_upgrade_data != NULL )
	{
		if(m_upgrade_flag == 1)
			m_cbfs->fcb_on_upgrade_error( );
		else
			m_cbfs->fcb_on_upgrade_end( );
	}
	m_upgrade_flag =  0;
	if(m_session.socket>0)
		close(m_session.socket);
	m_session.socket = -1;

	if(m_session.buf)
		free(m_session.buf);
	m_session.buf = NULL;

	m_session.thread = NULL;
	m_session_is_create = false;
	return false;
}
