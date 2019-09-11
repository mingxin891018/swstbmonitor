/****************************************************************************************** 
 * @file swstbmonitor_md5.c
 * @brief 华为管理工具加密接口
 * @author Yushuilong
 * @Created On 09.5.22
 * @Copyright sunniwell
 * @upgrade by Yu shuiong and Chen xu
 *********************************************************************************************/
#include "swapi.h"
#include "des.h"
#include "swssl.h"
#include "swparameter.h"
#include "swstbmonitor_priv.h"
#include "swparamsafe.h"

#define MD5KEY "huawei" 

static int sw_stbmonitor_md5encryp_acct(char* in_buf, char* output_buf, int*  output_size,int cut_size)
{  		
	char szParamBuf[128];
	unsigned char checksum1[16];
	unsigned int i;

	sw_md5_sum((unsigned char*)checksum1,(unsigned char*)in_buf,strlen(in_buf),NULL,0);
	
	/*start checksum1 string*/ 
	memset(szParamBuf,0,sizeof(szParamBuf));
	for(i=0; i< sizeof(checksum1); i++)
	{
		char digit[4];
		memset(digit,0,sizeof(digit));
 
		snprintf(digit,sizeof(digit),"%02x",checksum1[i]&0xff ); //mind the x case
 
		strlcat(szParamBuf,digit,sizeof(szParamBuf));
	}
	//HWSTBMONITOR_LOG_DEBUG("szParamBuf:%s\n",szParamBuf);
	//取前8个字符作密文
 
	strncpy(output_buf, szParamBuf, cut_size);
	*output_size = strlen(output_buf); 
	return 0;
}

static int sw_stbmonitor_md5encrypt(char* in_buf, char* output_buf, int*  output_size)
{  		
	char szParamBuf[128];
	unsigned char checksum1[16];
	unsigned int i;

	sw_md5_sum((unsigned char*)checksum1,(unsigned char*)in_buf,strlen(in_buf),(unsigned char*)MD5KEY,strlen(MD5KEY));
	
	/*start checksum1 string*/ 
	memset(szParamBuf,0,sizeof(szParamBuf));
	for(i=0; i < sizeof(checksum1); i++)
	{
		char digit[4];
		memset(digit,0,sizeof(digit));
		snprintf(digit,sizeof(digit),"%02X",checksum1[i]&0xff ); //mind the x case
		strlcat(szParamBuf,digit,sizeof(szParamBuf));
	}
	SWSTBMONITOR_LOG_DEBUG("szParamBuf:%s\n",szParamBuf);
	//取前8个字符作密文
	strncpy(output_buf, szParamBuf, 8);
	*output_size = strlen(output_buf);
	return 0;
}

int sw_stbmonitor_get_sessionid(char *session_id ,int cutsize)
{
 	char acct[64] = {0};
	char pwd[64] = {0};
	char acct_pwd[64] = {0};
	int encrypt_size = 0;
#ifndef SUPPORT_MONITOR_CHECKCODE
	sw_parameter_get("manager_user",acct,sizeof(acct));
	sw_parameter_safe_get("manager_password",pwd,sizeof(pwd));
	//sw_parameter_get("manager_password",pwd,sizeof(pwd));
#else
	sw_android_property_get("manager_user",acct,sizeof(acct));
	sw_android_property_get("manager_password",pwd,sizeof(pwd));
#endif

	if(strlen(acct)==0)
		strlcpy(acct,MANAGER_USER,sizeof(acct));
	if(strlen(pwd)==0)
		strlcpy(pwd,MANAGER_PASSWORD,sizeof(pwd));	
	snprintf(acct_pwd,sizeof(acct_pwd),"%s%s",acct,pwd);
	sw_stbmonitor_md5encryp_acct(acct_pwd,session_id,&encrypt_size,cutsize);
	return encrypt_size;	
}

bool sw_stbmonitor_check_identify_code(char *identify_code, char *session_id)
{
	unsigned char encrypt_val[16];
	int encrypt_size = 0;

	if(!session_id || !identify_code)
	{
		SWSTBMONITOR_LOG_DEBUG("sessionId and identify code is NULL \n");
		return false;
	}
	memset(encrypt_val, 0, sizeof(encrypt_val));
	sw_stbmonitor_md5encrypt( session_id, (char *)(encrypt_val), &encrypt_size );
	if(encrypt_size != 8)
	{
		SWSTBMONITOR_LOG_DEBUG("Password Encryted Fail\n\n");
		return false;
	}
	if( strncasecmp((char*)identify_code, (char*)encrypt_val, 8) == 0 )
	{
		SWSTBMONITOR_LOG_DEBUG("check identify code success!\n\n");
		return true;
	}
	return false;
}	
