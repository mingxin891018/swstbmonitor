#ifndef __SWSTBMONITOR_PRIV_H__
#define __SWSTBMONITOR_PRIV_H__

#include "swlog.h"
#include "swapi.h"
#include "swstbmonitorserver.h"

#define SWSTBMONITOR_LOG_DEBUG( format, ...) 	sw_log_syslog( LOG_LEVEL_DEBUG, LOG_TYPE_SECURITY, "stbstbmonitor", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define SWSTBMONITOR_LOG_INFO( format, ... ) 	sw_log_syslog( LOG_LEVEL_INFO, LOG_TYPE_SECURITY, "stbstbmonitor", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define SWSTBMONITOR_LOG_WARN( format, ... ) 	sw_log_syslog( LOG_LEVEL_WARN, LOG_TYPE_SECURITY, "stbstbmonitor", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define SWSTBMONITOR_LOG_ERROR( format, ... ) 	sw_log_syslog( LOG_LEVEL_ERROR, LOG_TYPE_SECURITY, "stbstbmonitor", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define SWSTBMONITOR_LOG_FATAL( format, ... ) 	sw_log_syslog( LOG_LEVEL_FATAL, LOG_TYPE_SECURITY, "stbstbmonitor", __FILE__, __LINE__, format, ##__VA_ARGS__  )

#define CONNECT_LOG_DEBUG( format, ...) 	sw_log_syslog( LOG_LEVEL_DEBUG, LOG_TYPE_USER, "stbstbmonitor", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define CONNECT_LOG_INFO( format, ... ) 	sw_log_syslog( LOG_LEVEL_INFO, LOG_TYPE_USER, "stbstbmonitor", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define CONNECT_LOG_WARN( format, ... ) 	sw_log_syslog( LOG_LEVEL_WARN, LOG_TYPE_USER, "stbstbmonitor", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define CONNECT_LOG_ERROR( format, ... ) 	sw_log_syslog( LOG_LEVEL_ERROR, LOG_TYPE_USER, "stbstbmonitor", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define CONNECT_LOG_FATAL( format, ... ) 	sw_log_syslog( LOG_LEVEL_FATAL, LOG_TYPE_USER, "stbstbmonitor", __FILE__, __LINE__, format, ##__VA_ARGS__  )

typedef int (*sw_stbmonitor_reply_callback)( int reply_code, char *function_call, char *call_value, char *data );
typedef int (*sw_stbmonitor_sftp_reply_callback)(int argc, char *ip, char *servername, char *serverpswd, char *localpath, char *remotepath, char *type, int *uploadsize);
typedef enum
{
	SW_STBMONITOR_RSA_SUNNIWELL,	
	SW_STBMONITOR_RSA_HUAWEI,	
}swstbmonitor_rsa_type_e;

int sw_stbmonitor_aes_create_key( unsigned char *rsabuf, swstbmonitor_rsa_type_e type );
int sw_stbmonitor_aes_dec( char *outbuf, char *inbuf, int inlen );
int sw_stbmonitor_aes_enc( char *outbuf, char *inbuf, int inlen );
int sw_stbmonitor_get_sessionid(char *session_id ,int cutsize);
bool sw_stbmonitor_check_identify_code(char *identify_code, char *session_id);
int sw_stbmonitor_create_session(int socket, sw_stbmonitor_callback_funcs_t *cbfs, int type);
void sw_stbmonitor_destroy_session();

int sw_stbmonitor_set_upgrade_reply_callback( sw_stbmonitor_reply_callback cbfs_reply );
int sw_stbmonitor_upgrade_server_create(sw_stbmonitor_callback_funcs_t *cbfs);
void sw_stbmonitor_upgrade_server_destroy();
int sw_stbmonitor_set_upgrade_file_size( int size );
int sw_stbmonitor_set_upgrade_force( bool force );
int sw_stbmonitor_create_upgrade_session(int socket, sw_stbmonitor_callback_funcs_t *cbfs);
void sw_stbmonitor_destroy_upgrade_session();

int sw_stbmonitor_set_ping_reply_callback( sw_stbmonitor_reply_callback cbfs_reply );
int sw_stbmonitor_ping_start( sw_stbmonitor_callback_funcs_t *cbfs, char* length,char* url );
int sw_stbmonitor_ping_stop();

int sw_stbmonitor_set_traceroute_reply_callback(sw_stbmonitor_reply_callback cbfs_reply);
int sw_stbmonitor_traceroute_start( sw_stbmonitor_callback_funcs_t *cbfs, char* url );
int sw_stbmonitor_traceroute_stop();
int sw_stbmonitor_set_remotepcap_reply_callback( sw_stbmonitor_reply_callback cbfs_reply );
int sw_stbmonitor_remotepcap_get_maxfilesize();
int sw_stbmonitor_remotepcap_start( sw_stbmonitor_callback_funcs_t *cbfs,  char *parameter, char *data );
char *xstrgetval( char *buf, char *name, char *value, int valuelen );

int sw_stbmonitor_upgrade_connect();
int sw_stbmonitor_filesend_connect(char *buffer);
#endif //__SWSTBMONITOR_PRIV_H__


