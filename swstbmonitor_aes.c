/****************************************************************************************** 
 * @file hwstbmonitor_aes.c
 * @brief 华为管理工具加密接口
 * @author 吴荣富
 * @Created On 09.5.22
 * @Copyright sunniwell
 * @upgrade by Yu shuiong and Chen xu
 *********************************************************************************************/
#include "swstbmonitor_priv.h"
#include "swssl.h"

//public_key
#ifdef SUPPORT_HWSTBMONITOR_VER4
#define MODULUS_HUAWEI "A7964C580775747B016CB256310C59FD8856BBAC0652690618BC4A9D05BF32A8DEC90A6929E7957C843358F1D55D0FDED1A3924D1EA6FA5381EB70BB0A74B6BE1E554A65392D69F36A65FC01461BEB4018644DEFDBA9647649F196872CBBEA86EAB0E4F8112BF92B5FE3365771F62F0D28D6FA8BF25B48860C64109B9F274F09"
#else
#define MODULUS_HUAWEI "AE23D0599167563D571A701D6AAEA4C4E3A865E7BBB69426830E56514A4241F83FF8BF47A03C8C4300B6E512FDEC9973257ADCB48A98776F7B655BA44B8E9942A9F725D3207C77FB8779C7D25FC2AD045BB8C0A9797D29A8C6C861F4FAAE4ED6AF3604658D3528CCC149D0327BF9B34388974C76F5AA04FD95C731652E64AEE1"
#endif

#define MODULUS_SUNNIWELL "6DC4C3E99719C967DFC55657A61E839393A82409122BDA4394DC270A68F3EB73EBABCD2D54040DCBD0D3D97F9440EE02E06CF1B77F888AA27260CAA1F6B9DE07E4EB9DB59DEBC864CFAFC692255581253BB3F396598B670FEB2B424F5AB80EC924998A7B00C2660B7715FAB5D92443721B64E7E0DD98371F63CD3CF6F885D101"

//#define PRIVATE_EXPONENT  "490D8AD50B1ABE4FC33D321963152BBC2BA8DB52AE25413D78F48B87021253C0F23784758654AC59D8C1251F232B933B96E0EA7D9502EA40F29E383D5256A0CD00401BBAB9DFF6A0BF83E6EB7C8D168EA57006D3E9295A2F24F13F3A9BA5DD2FC015F891D5F3F94AE75BED000E6012FC53391ED74AEFEF365D5AA4AE1683A781"

#define PUBLIC_EXPONENT 65537

#define RSA_PKCS1_PADDING 1	

static unsigned char RandChar128bit[32] = {0};

//create key
int sw_stbmonitor_aes_create_key( unsigned char *rsabuf, swstbmonitor_rsa_type_e type )
{
	swrsa_st decrypt;
	memset(&decrypt,0,sizeof(decrypt));
	
	struct timeval tv;
	int ret = -1;
	int Random1 = 0;
	int Random2 = 0;
	if( rsabuf==NULL )
	{
		SWSTBMONITOR_LOG_ERROR("error due to rsabuf being null\n");
		return -1;
	}
	memset(&tv,0,sizeof(tv));
	gettimeofday(&tv,NULL);
	srand(tv.tv_usec);    
	Random1 = rand()%100000000;
	Random2 = rand()%100000000;
	memset( RandChar128bit,0,sizeof(RandChar128bit) );
	snprintf((char *)(RandChar128bit),sizeof(RandChar128bit),"%08u%08u",Random1,Random2);
		
	swrsa_keyinfo key;
	memset(&key, 0, sizeof(key));
	key.type = 0;
	//根据type选择
	if(type == SW_STBMONITOR_RSA_SUNNIWELL)
		key.n = (unsigned char*)(MODULUS_SUNNIWELL);
	else
		key.n = (unsigned char*)(MODULUS_HUAWEI);
	key.d = NULL;
	key.e = PUBLIC_EXPONENT;
	if( sw_rsa_init( &decrypt, &key, SWRSA_PKCS1_PADDING , SWRSA_ENCRYPT_PUBLIC) )
	{
		SWSTBMONITOR_LOG_ERROR("sw_rsa_init failed\n");
		return -1;
	}
	ret = sw_rsa_enc( &decrypt, rsabuf, RandChar128bit, strlen((char *)(RandChar128bit)) );
	if ( ret < 0 )
	{
		SWSTBMONITOR_LOG_ERROR("sw_rsa_enc failed\n");
		return -1;
	}
	return ret;
}

//aes_decryption
int sw_stbmonitor_aes_dec( char *outbuf, char *inbuf, int inlen )
{
	swaes_st decrypt;
	memset(&decrypt,0,sizeof(swaes_st));
	
	int len = -1;
	int cipher_size = 128;
	int encrypt_mode = SWAES_MODE_CBC;
	unsigned char IV[] = "8D352C149D0327BF";
	int padding_mode = SWAES_PADDING_PKCS5;
	if( strlen((char *)(RandChar128bit)) <= 0)
	{
		SWSTBMONITOR_LOG_ERROR("RandChar128bit is empty\n");
		return -1;
	}
	if( sw_aes_init( &decrypt, RandChar128bit, cipher_size, encrypt_mode, IV, padding_mode, 0 ) )
	{
		SWSTBMONITOR_LOG_ERROR("sw_aes_init failed\n");
		return -1;
	}
	len = sw_aes_dec(&decrypt, (unsigned char*)outbuf, (unsigned char*)inbuf, inlen);
	if( len < 0)
	{
		SWSTBMONITOR_LOG_ERROR("sw_aes_dec failed\n");
		return -1;
	}
	return len;
}

//aes_encryption
int sw_stbmonitor_aes_enc( char *outbuf, char *inbuf, int inlen )
{
	swaes_st decrypt;
	memset(&decrypt,0,sizeof(swaes_st));
	
	int len = -1;
	int cipher_size = 128;
	int encrypt_mode = SWAES_MODE_CBC;
	unsigned char IV[] = "8D352C149D0327BF";
	int padding_mode = SWAES_PADDING_PKCS5;
	if( strlen((char *)(RandChar128bit)) <= 0)
	{
		SWSTBMONITOR_LOG_ERROR("sw_aes_init failed\n");
		return -1;
	}
	if( sw_aes_init( &decrypt, RandChar128bit, cipher_size, encrypt_mode, IV, padding_mode, 1 ) )
	{
		SWSTBMONITOR_LOG_ERROR("sw_aes_init failed\n");
		return -1;
	}
	len = sw_aes_enc(&decrypt, (unsigned char*)outbuf, (unsigned char*)inbuf, inlen);
	if( len < 0 )
	{
		SWSTBMONITOR_LOG_ERROR("sw_aes_enc failed\n");
		return -1;
	}
	return len;
}
