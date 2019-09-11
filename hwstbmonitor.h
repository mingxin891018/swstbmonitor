#ifndef __HWSTBMONITOR_H__
#define __HWSTBMONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#define STBMONITOR_READ				0
#define STBMONITOR_WRITE			1
#define STBMONITOR_WRITE_VALIDATE	2

/** 
 * @brief 华为统一管理工具哈希表所调用的函数原型
 * 
 * @param name 华为统一管理工具下发的参数名
 * @param value 华为统一管理工具下发的参数值
 * @param size  华为统一管理工具下发的参数长度 
 * @param act_type  要求函数操作的类型，STBMONITOR_READ为读，STBMONITOR_WWRITE普通写不立即生效，STBMONITOR_WRITE_VALIDATE需要立即生效的写参数
 * 
 * @return 0: 处理成功, -1:处理失败
 */
typedef int (*stbmonitor_handle)(const char *name, char *value, int size, int act_type );

/* 启动网管服务 */
void hw_nmp_init();
/* 关闭网管服务 */
void hw_nmp_exit();

int hw_nmp_connect( char *server_ip );

int hw_stbmonitor_parameter_init(void);

void hw_stbmonitor_parameter_exit(void);

int hw_stbmonitor_parameter_write(const char *params, char *buf,int length, bool bvalidate);

int hw_stbmonitor_parameter_read(const char *params, char *buf, int length);

/* 检测stbmonitor是否存在此参数 */
bool hw_stbmonitor_parameter_check(const char *params);

void hw_stbmonitor_add_parameter(char *name, stbmonitor_handle fun);


bool hw_telnet_is_enable();
void hw_telnet_enable(bool val);
bool hw_stbmonitor_is_enable();
void hw_stbmonitor_enable(bool val);
void hw_telnet_hwnmp_disable();


#ifdef __cplusplus
}
#endif

#endif /*__HWSTBMONITOR_H__ */
