#ifndef __SW_STBMONITOR_SERVER_H__
#define __SW_STBMONITOR_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define STBMONITOR_FUNCTION_CALL_READ				0x01
#define STBMONITOR_FUNCTION_CALL_WRITE				0x02
#define STBMONITOR_FUNCTION_CALL_IOCTL				0x03
#define STBMONITOR_FUNCTION_CALL_INFORM				0x04
#define STBMONITOR_FUNCTION_CALL_CONNECT			0x05
/*********************** 
****** reply code ******
************************/
#define STBMONITOR_REPLY_OK					200/* 命令执行成功 */
#define STBMONITOR_REPLY_VERSION_SAME			300/* 版本相同，升级没有执行 */
#define STBMONITOR_REPLY_UPGRADE_ON			301/* 版本正在升级中... */
#define STBMONITOR_REPLY_UPGRADE_FINISH	 	302/* 升级完成，自动重启 */
#define STBMONITOR_REPLY_CHECK_VERSION_ERR	 	303/* 非法的版本文件，校验失败 */
#define STBMONITOR_REPLY_IDE_CODE_ILLEGAL	 	501/* 非法连接，或者用户或密码错误 */
#define STBMONITOR_REPLY_OPERATE_ILLEGAL	 	502/* 非法操作,命令没有定义*/
#define STBMONITOR_REPLY_LOCKED					503/* 非法操作,用户被锁定*/
#define STBMONITOR_REPLY_CHANNEL_NOT_EXIT	 	601/* 输入了不存在的频道号，频道不存在,操作失败 */
#define STBMONITOR_REPLY_RECV_DATA_TIMEOUT	 	602/* 没有数据 */
#define STBMONITOR_REPLY_PLAYURL_ERR			603/* 播放连接不可用 */
#define STBMONITOR_REPLY_PARA_ILLEGAL			401/* 非法参数值 */
#define STBMONITOR_REPLY_CMD_ILLEGAL			402/* 非法命令 */
#define STBMONITOR_REPLY_CHANNELURL_NOT_EXIT	403/* 输入了不存在的频道，频道不存在,操作失败 */

/* logging level */
#define	STBMONITOR_LOG_LEVEL_ALL		0x00	//输出所有日志
#define	STBMONITOR_LOG_LEVEL_DEBUG		0x01	//指出细粒度信息事件对调试应用程序是非常有帮助的
#define	STBMONITOR_LOG_LEVEL_INFO		0x02	//消息在粗粒度级别上突出强调应用程序的运行过程
#define	STBMONITOR_LOG_LEVEL_WARN		0x03	//表明会出现潜在错误的情形
#define	STBMONITOR_LOG_LEVEL_ERROR		0x04	//指出虽然发生错误事件，但仍然不影响系统的继续运行
#define	STBMONITOR_LOG_LEVEL_FATAL		0x05	//指出每个严重的错误事件将会导致应用程序的退出
#define	STBMONITOR_LOG_LEVEL_OFF		0x06	//关闭日志输出

typedef void func_callback_log_output( int level, const char* mod, const char* file, int line, const char *logbuf );

typedef int func_callback_on_read_all_parameter( char *buf, int size );
typedef int func_callback_on_read_parameter( char *name, char*value, int size);
typedef int func_callback_on_read_channellist( char *buf, int size );
typedef int func_callback_on_write_parameter( char *name, char*value, int save );

typedef int func_callback_on_upgrade_data( unsigned char *buf, int size, int total_size, bool force );
typedef int func_callback_on_upgrade_error();
typedef int func_callback_on_upgrade_end();
typedef int func_callback_on_upgrade_online();

typedef int func_callback_on_stb_reboot();
typedef int func_callback_on_restore_setting();

typedef int func_callback_on_set_test_mode();
typedef int func_callback_on_set_autotest_mode();
typedef int func_callback_on_set_work_mode();
typedef int func_callback_on_set_scriptrecord_mode();


typedef int func_callback_on_set_log_out_type( char *type );
typedef int func_callback_on_set_log_level( char *level );
typedef int func_callback_on_set_log_type( char *type );

typedef int func_callback_on_media_play( char *mrl );
typedef int func_callback_on_media_pause();
typedef int func_callback_on_media_stop();
typedef int func_callback_on_media_ffwd();
typedef int func_callback_on_media_fbwd();

typedef int func_callback_on_sftp_write_nonblock(int argc, char *ip, char *servername, char *serverpswd, char *localpath, char *remotepath, char *type, int *uploadsize);

typedef int func_callback_on_syscmd(const char* cmd );

//typedef int func_callback_on_ping(const char* srmping );
//typedef int func_callback_on_traceroute(const char* hostip );
typedef int func_callback_on_ping(char* result,char* srmping );
typedef int func_callback_on_traceroute(char* result, char* hostip );

typedef int func_callback_on_startBootDebugInfo();
typedef int func_callback_on_stopBootDebugInfo(char *url);
typedef int func_callback_on_uploadBootDebugInfo(char *url);

typedef int func_callback_on_startDebugInfo();
typedef int func_callback_on_stopDebugInfo(char *url);
typedef int func_callback_on_uploadDebugInfo();
typedef int func_callback_on_startChannelZapping();
typedef int func_callback_on_stopChannelZapping();
typedef int func_callback_on_startPipSwapping();
typedef int func_callback_on_stopPipSwapping();
typedef int func_callback_on_startStability();
typedef int func_callback_on_stopStability();
typedef int func_callback_on_fileSend();

typedef struct _sw_stbmonitor_callback_funs
{
	func_callback_on_startBootDebugInfo* fcb_on_startBootDebugInfo;
	func_callback_on_stopBootDebugInfo* fcb_on_stopBootDebugInfo;
	func_callback_on_uploadBootDebugInfo* fcb_on_uploadBootDebugInfo;
	func_callback_log_output* fcb_log_output;
	func_callback_on_read_all_parameter* fcb_on_read_all_parameter;
	func_callback_on_read_parameter* fcb_on_read_parameter;
	func_callback_on_read_channellist* fcb_on_read_channellist;
	func_callback_on_write_parameter* fcb_on_write_parameter;
	func_callback_on_upgrade_data* fcb_on_upgrade_data;
	func_callback_on_upgrade_error* fcb_on_upgrade_error;
	func_callback_on_upgrade_end* fcb_on_upgrade_end;
	func_callback_on_stb_reboot* fcb_on_stb_reboot;
	func_callback_on_restore_setting* fcb_on_restore_setting;
	func_callback_on_set_test_mode* fcb_on_set_test_mode;
	func_callback_on_set_autotest_mode*	fcb_on_set_autotest_mode;
	func_callback_on_set_work_mode* fcb_on_set_work_mode;
	func_callback_on_set_scriptrecord_mode* fcb_on_set_scriptrecord_mode;
	func_callback_on_upgrade_online* fcb_on_upgrade_online;
	func_callback_on_set_log_out_type* fcb_on_set_log_out_type;
	func_callback_on_set_log_level* fcb_on_set_log_level;
	func_callback_on_set_log_type* fcb_on_set_log_type;
	func_callback_on_media_play* fcb_on_media_play;
	func_callback_on_media_pause* fcb_on_media_pause;
	func_callback_on_media_stop* fcb_on_media_stop;
	func_callback_on_media_ffwd* fcb_on_media_ffwd;
	func_callback_on_media_fbwd* fcb_on_media_fbwd;
	func_callback_on_sftp_write_nonblock* fcb_on_sftp_write_noblock;
	func_callback_on_syscmd* fcb_on_syscmd;
	func_callback_on_ping* fcb_on_ping;
	func_callback_on_traceroute* fcb_on_traceroute;
	func_callback_on_startDebugInfo* fcb_on_startDebugInfo;
	func_callback_on_stopDebugInfo* fcb_on_stopDebugInfo;
	func_callback_on_uploadDebugInfo* fcb_on_uploadDebugInfo;
	func_callback_on_startChannelZapping* fcb_on_startChannelZapping;
	func_callback_on_stopChannelZapping* fcb_on_stopChannelZapping;
	func_callback_on_startPipSwapping* fcb_on_startPipSwapping;
	func_callback_on_stopPipSwapping* fcb_on_stopPipSwapping;
	func_callback_on_startStability* fcb_on_startStability;
	func_callback_on_stopStability* fcb_on_stopStability;
	func_callback_on_fileSend* fcb_on_fileSend;
}sw_stbmonitor_callback_funcs_t;

int sw_stbmonitor_server_init(sw_stbmonitor_callback_funcs_t *cbfs);
void sw_stbmonitor_server_exit();

char *sw_stbmonitor_get_ip();

int sw_stbmonitor_sleep(bool sleep);

int sw_stbmonitor_connect(sw_stbmonitor_callback_funcs_t *cbfs, char *server_ip, char *local_ip);

#ifdef __cplusplus
}
#endif

#endif /*__HWSTBMONITOR_H__ */
