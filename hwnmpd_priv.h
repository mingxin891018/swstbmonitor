#ifndef __HWNMP_PRIV_H__
#define __HWNMP_PRIV_H__

#include "swlog.h"
#include "swapi.h"
#include "swstbmonitorserver.h"

#define HWNMPD_LOG_DEBUG( format, ...) 	sw_log_syslog( LOG_LEVEL_DEBUG, LOG_TYPE_OPERATION, "hwnmpd", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define HWNMPD_LOG_INFO( format, ... ) 	sw_log_syslog( LOG_LEVEL_INFO, LOG_TYPE_OPERATION, "hwnmpd", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define HWNMPD_LOG_WARN( format, ... ) 	sw_log_syslog( LOG_LEVEL_WARN, LOG_TYPE_OPERATION, "hwnmpd", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define HWNMPD_LOG_ERROR( format, ... ) 	sw_log_syslog( LOG_LEVEL_ERROR, LOG_TYPE_OPERATION, "hwnmpd", __FILE__, __LINE__, format, ##__VA_ARGS__  )
#define HWNMPD_LOG_FATAL( format, ... ) 	sw_log_syslog( LOG_LEVEL_FATAL, LOG_TYPE_OPERATION, "hwnmpd", __FILE__, __LINE__, format, ##__VA_ARGS__  )

int sw_stbmonitor_hwparameter_size(void);
int sw_stbmonitor_hwparameter_read_byindex(int index, char **key, char *buf, int length);

int sw_stbmonitor_hwparameter_read(const char *params, char *buf, int length);
int sw_stbmonitor_hwparameter_write(const char *params, char *buf, int length);
void sw_close_sFtpUpload(void);

int sw_stbmonitor_read_channellist();
int sw_stbmonitor_read_channelurl(int channum);
char* sw_time2clock(time_t time,char* strtime, int size);

bool hw_telnet_is_enable();
void hw_telnet_enable(bool val);
bool hw_stbmonitor_is_enable();
void hw_stbmonitor_enable(bool val);
void hw_telnet_hwnmp_disable();

int property_get(const char *key, char *value, const char *default_value);


#endif //__HWNMP_PRIV_H__


