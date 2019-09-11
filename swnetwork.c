/** 
 * @file swnetwork.c
 * @brief 提供对网络接入进行控制的接口，网络接入参数通过参数获取，并提供接口供外部程序查询网络状态
 * @author ...
 * @date 2007-09-06
 */
#include "swapi.h"
#include "swapp.h"
#include "swnetconnect.h"
#include "swnetwork.h"
#include "swparameter.h"

static char m_mac[32];
static int m_igmp_ver = 2;

static sw_netconnect_t  m_net_connects[MAX_NET_CONNECT];

static sw_netstate_t m_net_states[MAX_NET_CONNECT];

/** 
 * @brief 初始化网络
 * 
 * @return 0,成功; <0,失败
 */
int sw_network_init(event_post_func event_post, void* handler)
{
	return 0;
}

/** 
 * @brief 释放资源
 */
void sw_network_exit()
{

}

/** 
 * @brief 连接网络
 * 
 * @return 0,成功; < 0,失败
 */
int sw_network_connect( )
{
	return 0;
}

/** 
 * @brief 断开网络连接
 */
int sw_network_disconnect( bool service_exit)
{
	return 0;
}


/** 
 * @brief 取得网络模式
 * 
 * @return 
 */
char* sw_network_get_defaultmode()
{
	return m_net_connects[0].netmode;
}


/** 
 * @brief 取得当前网卡的ip地址
 * 
 * @return ip地址
 */
char* sw_network_get_lanip()
{
    if((strncmp(sw_network_get_currentmode(), "wifi_static", 11) == 0) || (strncmp(sw_network_get_currentmode(), "wifi_dhcp", 9) == 0))
        sw_android_property_get("wifi_ip", m_net_states[0].lanv4.ip, sizeof(m_net_states[0].lanv4.ip));
	else
    	sw_android_property_get("lan_ip", m_net_states[0].lanv4.ip, sizeof(m_net_states[0].lanv4.ip));
	return m_net_states[0].lanv4.ip;
}


/** 
 * @brief 取得网卡的网关
 * 
 * @return 
 */
char* sw_network_get_langateway()
{
    if((strncmp(sw_network_get_currentmode(), "wifi_static", 11) == 0) || (strncmp(sw_network_get_currentmode(), "wifi_dhcp", 9) == 0))
        sw_android_property_get("wifi_gateway", m_net_states[0].lanv4.gateway, sizeof(m_net_states[0].lanv4.gateway));
	else
    	sw_android_property_get("lan_gateway", m_net_states[0].lanv4.gateway, sizeof(m_net_states[0].lanv4.gateway));
	return m_net_states[0].lanv4.gateway;
}

/*  */
/** 
 * @brief 取得网卡的子网掩码
 * 
 * @return 
 */
char* sw_network_get_lanmask()
{
    if((strncmp(sw_network_get_currentmode(), "wifi_static", 11) == 0) || (strncmp(sw_network_get_currentmode(), "wifi_dhcp", 9) == 0))
        sw_android_property_get("wifi_mask", m_net_states[0].lanv4.mask, sizeof(m_net_states[0].lanv4.mask));
	else
    	sw_android_property_get("lan_mask", m_net_states[0].lanv4.mask, sizeof(m_net_states[0].lanv4.mask));
	return m_net_states[0].lanv4.mask;
}

/** 
 * @brief 取得当前的网络模式
 * 
 * @return 
 */
char* sw_network_get_currentmode()
{
    if(sw_android_property_get("defaultnetmode", m_net_connects[0].netmode, sizeof(m_net_connects[0].netmode)))
        return m_net_connects[0].netmode;
    else
		return "static";
}

/** 
 * @brief 取得取得当前的route使用的ip
 * 
 * @return 
 */
char* sw_network_get_currentip()
{
    if((strncmp(sw_network_get_currentmode(), "static", 6) == 0) || (strncmp(sw_network_get_currentmode(), "dhcp", 4) == 0))
        sw_android_property_get("lan_ip", m_net_states[0].lanv4.ip, sizeof(m_net_states[0].lanv4.ip));
    if(strncmp(sw_network_get_currentmode(), "pppoe", 5) == 0)
        sw_android_property_get("pppoe_ip", m_net_states[0].lanv4.ip, sizeof(m_net_states[0].lanv4.ip));
    if((strncmp(sw_network_get_currentmode(), "wifi_static", 11) == 0) || (strncmp(sw_network_get_currentmode(), "wifi_dhcp", 9) == 0))
        sw_android_property_get("wifi_ip", m_net_states[0].lanv4.ip, sizeof(m_net_states[0].lanv4.ip));

	return m_net_states[0].lanv4.ip;
}

/** 
 * @brief 取得当前route使用的网关
 * 
 * @return 
 */
char* sw_network_get_currentgateway()
{
    if((strncmp(sw_network_get_currentmode(), "static", 6) == 0) || (strncmp(sw_network_get_currentmode(), "dhcp", 4) == 0))
        sw_android_property_get("lan_gateway", m_net_states[0].lanv4.gateway, sizeof(m_net_states[0].lanv4.gateway));
    if(strncmp(sw_network_get_currentmode(), "pppoe", 5) == 0)
        sw_android_property_get("pppoe_gateway", m_net_states[0].lanv4.gateway, sizeof(m_net_states[0].lanv4.gateway));
    if((strncmp(sw_network_get_currentmode(), "wifi_static", 11) == 0) || (strncmp(sw_network_get_currentmode(), "wifi_dhcp", 9) == 0))
        sw_android_property_get("wifi_gateway", m_net_states[0].lanv4.gateway, sizeof(m_net_states[0].lanv4.gateway));

    return m_net_states[0].lanv4.gateway;
}

/** 
 * @brief 取得当前route使用的子网掩码
 * 
 * @return 
 */
char* sw_network_get_currentmask()
{
    if((strncmp(sw_network_get_currentmode(), "static", 6) == 0) || (strncmp(sw_network_get_currentmode(), "dhcp", 4) == 0))
        sw_android_property_get("lan_mask", m_net_states[0].lanv4.mask, sizeof(m_net_states[0].lanv4.mask));
    if(strncmp(sw_network_get_currentmode(), "pppoe", 5) == 0)
        sw_android_property_get("pppoe_mask", m_net_states[0].lanv4.mask, sizeof(m_net_states[0].lanv4.mask));
    if((strncmp(sw_network_get_currentmode(), "wifi_static", 11) == 0) || (strncmp(sw_network_get_currentmode(), "wifi_dhcp", 9) == 0))
        sw_android_property_get("wifi_mask", m_net_states[0].lanv4.mask, sizeof(m_net_states[0].lanv4.mask));

	return m_net_states[0].lanv4.mask;
}

/** 
 * @brief 取得当前route使用的dns
 * 
 * @return 
 */
char* sw_network_get_currentdns()
{
    if((strncmp(sw_network_get_currentmode(), "static", 6) == 0) || (strncmp(sw_network_get_currentmode(), "dhcp", 4) == 0))
        sw_android_property_get("lan_dns", m_net_states[0].lanv4.dns, sizeof(m_net_states[0].lanv4.dns));
    if(strncmp(sw_network_get_currentmode(), "pppoe", 5) == 0)
        sw_android_property_get("pppoe_dns", m_net_states[0].lanv4.dns, sizeof(m_net_states[0].lanv4.dns));
    if((strncmp(sw_network_get_currentmode(), "wifi_static", 11) == 0) || (strncmp(sw_network_get_currentmode(), "wifi_dhcp", 9) == 0))
        sw_android_property_get("wifi_dns", m_net_states[0].lanv4.dns, sizeof(m_net_states[0].lanv4.dns));

    return m_net_states[0].lanv4.dns;
}

/**
 * @brief 取得备份route使用的dns 
 * 
 * @return 
 */
char* sw_network_get_currentdns2()
{
    if((strncmp(sw_network_get_currentmode(), "static", 6) == 0) || (strncmp(sw_network_get_currentmode(), "dhcp", 4) == 0))
        sw_android_property_get("lan_dns2", m_net_states[0].lanv4.dns2, sizeof(m_net_states[0].lanv4.dns2));
    if(strncmp(sw_network_get_currentmode(), "pppoe", 5) == 0)
        sw_android_property_get("pppoe_dns2", m_net_states[0].lanv4.dns2, sizeof(m_net_states[0].lanv4.dns2));
    if((strncmp(sw_network_get_currentmode(), "wifi_static", 11) == 0) || (strncmp(sw_network_get_currentmode(), "wifi_dhcp", 9) == 0))
        sw_android_property_get("wifi_dns2", m_net_states[0].lanv4.dns2, sizeof(m_net_states[0].lanv4.dns2));

	return m_net_states[0].lanv4.dns2;
}

char* sw_network_get_current_v6_localip()
{
    return NULL;
}

char* sw_network_get_current_v6_ip()
{
    return NULL;
}

int sw_network_get_current_v6_pref_len()
{
    return 0;
}

char* sw_network_get_current_v6_dns()
{
    return NULL;
}

char* sw_network_get_current_v6_dns2()
{
    return NULL;
}


/** 
 * @brief 取得取得当前的route使用的ip
 * 
 * @return 
 */
char* sw_network_get_current_vlan_ip()
{
	return NULL;	
}

/** 
 * @brief 取得当前route使用的网关
 * 
 * @return 
 */
char* sw_network_get_current_vlan_gateway()
{
    return NULL;
}

/** 
 * @brief 取得当前route使用的子网掩码
 * 
 * @return 
 */
char* sw_network_get_current_vlan_netmask()
{
    return NULL;
}

/** 
 * @brief 取得当前route使用的dns
 * 
 * @return 
 */
char* sw_network_get_current_vlan_dns()
{
    return NULL;
}

/**
 * @brief 取得备份route使用的dns 
 * 
 * @return 
 */
char* sw_network_get_current_vlan_dns2()
{
    return NULL;
}

char* sw_network_get_current_vlan_v6_localip()
{
    return NULL;
}

char* sw_network_get_current_vlan_v6_ip()
{
    return NULL;
}

int sw_network_get_current_vlan_v6_pref_len()
{
    return 0;
}

char* sw_network_get_current_vlan_v6_dns()
{
    return NULL;
}

char* sw_network_get_current_vlan_v6_dns2()
{
    return NULL;
}


/** 
 * @brief 取得MAC地址: xx:xx:xx:xx:xx:xx
 * 
 * @return 
 */
char* sw_network_get_mac()
{
    sw_android_property_get("mac", m_mac, sizeof(m_mac));
	return m_mac;	
}


/** 
 * @brief 取得当前IGMP版本
 * 
 * @return 
 */
int sw_network_get_igmpver()
{
	return m_igmp_ver;
}

/** 
 * @brief 设置当前IGMP版本
 * 
 * @param ver 
 * 
 * @return true,成功; false,失败
 */
bool sw_network_set_igmpver(  int ver  )
{
	FILE* fp = NULL;

	if( ( fp = fopen( "/proc/sys/net/ipv4/conf/default/force_igmp_version", "wb" ) ) )
	{
    	fprintf( fp, "%d", ver );
    	fclose( fp );
	}
	if( ( fp = fopen( "/proc/sys/net/ipv4/conf/eth0/force_igmp_version", "wb" ) ) )
	{
		fprintf( fp, "%d", ver );
		fclose( fp );
	}
    m_igmp_ver = ver;
	return true;
}


/** 
 * @brief 取得当前网线是否连接
 * 
 * @return 
 */
bool sw_network_get_cable_connected()
{
	return true;
}


net_state_t sw_network_get_state()
{

	return NET_STATE_NULL;
}

