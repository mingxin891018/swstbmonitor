#include "swapi.h"
#include "swstbmonitorserver.h"
#include "hwnmpd_priv.h"
#include "swthrd.h"
#include "swevtdispatcher.h"
#include "swapp.h"
#include "swparameter.h"
#include "hwstbmonitor.h"
#include "swstb.h"
#include "string_ext.h"
#include <paths.h>

#define MANAGER_USERNAME "sunniwell"
#define MANAGER_PASSWORD ".287aW"

static sw_stbmonitor_callback_funcs_t m_scbfs;
static bool m_callback_function_is_add  = false;
static char last_server_ip[32] = {0};

extern char* sw_network_get_currentip(void);
extern bool sw_stbmonitor_session_is_create(void);

static int cb_on_set_log_type(char *type)
{
	int ret = sw_parameter_set("log_type", type);
	sw_parameter_save();
	return ret;
}

static int cb_on_set_log_level(char *level)
{
	char value[32] = {0};
	sw_strlcpy(value, sizeof(value), level, sizeof(value));
	int ret = sw_parameter_set("log_level", value);
	sw_parameter_save();
	return 0;
}

static int cb_on_set_log_out_type(char *out_type)
{
	int ret = -1;
	return ret;
}

static int cb_on_read_parameter( char *name, char *value, int size)
{
	int ret = sw_parameter_get(name, value, size);
	return ret = 1 ? strlen(value) : -1;
}

static int cb_on_write_parameter( char *name, char *value, int save)
{
	int ret = sw_parameter_set(name, value);
	sw_parameter_save();
	return ret = 1 ? 0 : -1;
}

static int cb_on_restore_setting(void)
{
	int ret = -1;
	return ret;
}

static int cb_on_stb_reboot(void)
{
	int ret = -1;
	return ret;
}

static int cb_on_stb_close_remote_connect(void)
{
	int ret = -1;
	return ret;
}

static void hw_stbmonitor_add_callback_function(void)
{
	if( m_callback_function_is_add == true )
		return;
	
	sw_memset( &m_scbfs, sizeof(m_scbfs), 0, sizeof(m_scbfs) );
	m_scbfs.fcb_on_set_log_type = cb_on_set_log_type;
	m_scbfs.fcb_on_set_log_level = cb_on_set_log_level;
	m_scbfs.fcb_on_set_log_out_type = cb_on_set_log_out_type;
	
	m_scbfs.fcb_on_read_parameter = cb_on_read_parameter;
	m_scbfs.fcb_on_write_parameter = cb_on_write_parameter;
	
	m_scbfs.fcb_on_restore_setting = cb_on_restore_setting;
	m_scbfs.fcb_on_stb_reboot = cb_on_stb_reboot;
	m_scbfs.fcb_on_stb_close_remote_connect = cb_on_stb_close_remote_connect;

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
	char manager_name[32] = {0};
	char manager_password[32] = {0};

	if(!sw_parameter_get("manager_username", manager_name, sizeof(manager_name)) || !sw_parameter_get("manager_password", manager_password, sizeof(manager_password))
			|| manager_name[0] == 0 || manager_password[0] == 0){
		sw_parameter_set("manager_username",MANAGER_USERNAME);
		sw_parameter_set("manager_password",MANAGER_PASSWORD);
		sw_parameter_save();
	}

	hw_stbmonitor_add_callback_function();
	sw_stbmonitor_server_init(&m_scbfs);
}

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
	HWNMPD_LOG_DEBUG("connectret=%d\n", connectret);
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

int main(int argv, char *argc[])
{
	hw_nmp_init();
	return 0;
}
