/***************************************************************************
* Copyright: sunniwell
* file     : nmpserver.c
* AUTHOR   : ???ٸ?
* brief    : ??Ϊͳһ???���?߷??????ӿ?
****************************************************************************/
#include "swstbmonitor_priv.h"
#include "swstbmonitorserver.h"
#include "swthrd.h"

/*********************************************
 *ping function
 **********************************************/
#define ICMP_ECHO 8 
#define ICMP_ECHOREPLY 0 
#define ICMP_MIN (8 + 4)	// minimum 8 byte icmp packet (just header + timestamp)
// ICMP header
typedef struct _tag_icmphdr
{
	unsigned char	i_type;
	unsigned char	i_code;
	unsigned short	i_cksum;
	unsigned short	i_id;
	unsigned short	i_seq;
	struct timeval	i_timestamp;
}STD_IcmpHeader;

// IP header
typedef struct _tag_iphdr
{
	unsigned int	h_len:4;          // length of the header 
	unsigned int	version:4;        // Version of IP 
    unsigned int	tos:8;             // Type of service 
    unsigned int	total_len:16;      // total length of the packet 

	unsigned short	ident;          // unique identifier 
	unsigned short	frag_and_flags; // flags 
	
	unsigned char	ttl;				// ttl
	unsigned char	proto;           // protocol (TCP, UDP etc) 
	unsigned short	checksum;       // IP checksum 
	
	unsigned int	sourceIP; 
	unsigned int	destIP; 
}STD_IpHeader;

//重复次数，超时，主机，数据块大小，DSCP(Ping请求)
typedef struct tagSRMPing
{
	char host[128];		//主机地址
	int no_reps;		//重复次数
	int timeout;		//超时ms
	int blk_size;		//数据包大小
	int DSCP;			//数据阻塞控制
}SRMPing;

//成功次数，失败次数，平均响应时长，最大响应时长，最小响应时长(Ping结果)
typedef struct tagSRMPingReslt
{
	int success_cnt;
	int failure_cnt;
	int avg_resp_time;
	int min_resp_time;
	int max_resp_time;
	char status[32];
}SRMPingReslt;

static HANDLE m_ping_thread = NULL;
static bool sw_stbmonitor_ping_proc( unsigned long wParam, unsigned long lParam );
static int sw_stbmontor_ping_inline( char* length, char* url );
static int sw_stbmontor_ping_outsize( char* length, char* url );
static int sw_swstbmonitor_ping_inline_init( char *_ip, char *p_buf, int size );
static int sw_swstbmonitor_ping_get_data( char *p_buf, int size );
static void fill_IcmpData(char *buf, int datasize);
static void fill_IcmpHeader(char *buf, int datasize);
static int decode_IpIcmp(char *inbuf, int insize, char *outbuf, int size);
static unsigned short in_cksum(unsigned short* addr, int len);
static void tv_sub(struct timeval *out,struct timeval *in);

static SRMPing m_ping_request;
static SRMPingReslt *m_ping_result = NULL;

/* ping*/
static int m_ping_time = 65535;
static bool m_b_ping_stop = false;
static int m_ping_send = 0;

static sw_stbmonitor_reply_callback m_cbfs_reply = NULL;
static sw_stbmonitor_callback_funcs_t *m_cbfs = NULL;

int sw_stbmonitor_set_ping_reply_callback( sw_stbmonitor_reply_callback cbfs_reply )
{
	m_cbfs_reply = cbfs_reply;
	return 0;
}

int sw_stbmonitor_ping_start( sw_stbmonitor_callback_funcs_t *cbfs, char* length,char* url )
{
	if(cbfs != NULL)
		m_cbfs = cbfs;
	if(m_ping_thread == NULL)
		m_ping_thread = sw_thrd_open( "tPingProc", 10, 0, 128*1024, (threadhandler_t)sw_stbmonitor_ping_proc, (unsigned long)length, (unsigned long)url);
	if(m_ping_thread)
		sw_thrd_resume(m_ping_thread);
	return 0;
}

int sw_stbmonitor_ping_stop()
{
	m_b_ping_stop = true;
	return 0;
}

static bool sw_stbmonitor_ping_proc( unsigned long wParam, unsigned long lParam )
{
	char *length = (char*)wParam;
	char *url = (char*)lParam;
	if(m_cbfs != NULL && m_cbfs->fcb_on_ping)
	{
		sw_stbmontor_ping_outsize(length,url);
	}
	else
	{
		sw_stbmontor_ping_inline(length,url);
	}
	m_ping_thread = NULL;
	return false;
}
static int sw_stbmontor_ping_outsize( char* length, char* url )
{
	char timelen[32];
	char cmdbuf[256];
	char buf[1024];
	FILE* fp = NULL;
	if(length == NULL || url == NULL)
		return -1;
	xstrgetval( length, "/length", timelen, sizeof(timelen));
	if ( timelen[0] != '\0' )
		m_ping_time = atoi(timelen)-1;
	else
		m_ping_time = 65535;
	m_b_ping_stop = false;
	m_ping_send = 0;
	/* start ip ping diagnostics thread */
	if( m_ping_request.blk_size > 64 )
		m_ping_request.blk_size = 64;
	memset( m_ping_request.host, 0, sizeof(m_ping_request.host) );
	snprintf( m_ping_request.host, sizeof(m_ping_request.host), "%s", url );
	m_ping_request.timeout = 2;
	snprintf(cmdbuf,sizeof(cmdbuf),"ping -c 1 %s > /tmp/.ping",url);
	while( m_ping_send <= m_ping_time && !m_b_ping_stop )
	{
#if 0
		m_cbfs->fcb_on_ping((char*)m_ping_result, (char*)(&m_ping_request));
#endif
		system(cmdbuf);
		fp = fopen("/tmp/.ping","rb");
		if(fp)
		{
			if( m_ping_send== 0 )
			{
				fgets(buf, sizeof(buf), fp);
				if( m_cbfs_reply != NULL )
					m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping", buf, NULL);
				sw_thrd_delay(300);
				fgets(buf, sizeof(buf), fp);
				if( strstr(buf,"icmp_seq") != NULL )
				{
					if( m_cbfs_reply != NULL )
						m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping", buf, NULL); 
				}
			}
			else
			{
				fgets(buf, sizeof(buf), fp);
				fgets(buf, sizeof(buf), fp);
				if( strstr(buf,"icmp_seq") != NULL )
				{
					if( m_cbfs_reply != NULL )
						m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping", buf, NULL); 
				}
				else
				{
					snprintf(buf,sizeof(buf),"From %s icmp_seq=1 Destination Host Unreachable",url);
					if( m_cbfs_reply != NULL )
						m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping", buf, NULL); 
				}
			}
		}
		if(fp)
		{
			fclose(fp);
		}
		m_ping_send++;
		sleep(1);
	}	
	if( m_cbfs_reply != NULL )
		m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping", NULL, NULL); 
	return 0;
}

static int sw_stbmontor_ping_inline( char* length, char* url )
{
	int i;
	char message[128];
	char p_buf[128];
	char timelen[32];
	int ret = -1;
	xstrgetval( length, "/length", timelen, sizeof(timelen));
	if ( timelen[0] != '\0' )
		m_ping_time = atoi(timelen)-1;
	else
		m_ping_time = 65535;
	m_b_ping_stop = false;
	memset(p_buf, 0, sizeof(p_buf));
	ret = sw_swstbmonitor_ping_inline_init( url , p_buf, sizeof(p_buf) );
	if(ret)
	{
		m_ping_send = 8;
		for(i = 0; i < 10; i++)
			m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping", p_buf, ""); 
		return -1;
	}
	m_ping_send = 0;
	m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping", p_buf, ""); 
	sleep(1);
	while(m_ping_send <= m_ping_time && !m_b_ping_stop )
	{
		memset(message, 0, sizeof(message));
		ret = sw_swstbmonitor_ping_get_data( message, sizeof(message) );
		if( ret > 0 && m_cbfs_reply != NULL )
		{
			m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping", message, ""); 
		}
		else if(ret <= 0 && m_cbfs_reply != NULL)
		{
			//snprintf(p_buf, size, "%s Unable to resolve", _ip);	
			m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping^From  icmp_seq= Destination Host Unreachable", message, "");
		}
		//m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping^From %s icmp_seq=%d Destination Host Unreachable", url, m_ping_send);
		sleep(1); //
		m_ping_send++;
	}
	if( m_cbfs_reply != NULL )
		m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^ping", NULL, NULL); 
	return 0;
}

static struct sockaddr_in m_ping_dest;
static int m_ping_sockfd = -1;
static int sw_swstbmonitor_ping_inline_init( char *_ip, char *p_buf, int size )
{
	struct hostent *hp;
	int datalen = 56;
	struct in_addr addr;
	if ( _ip[0] == '\0')	
	{
		snprintf(p_buf, size, "ipaddr is empty!");	
		return -2;
	}
	if ( m_ping_sockfd >= 0 )
		close(m_ping_sockfd);
	m_ping_sockfd = -1;
	memset(&m_ping_dest, 0, sizeof(m_ping_dest));
	m_ping_dest.sin_family = AF_INET;
	if ( inet_aton(_ip, &addr) )
	{
		m_ping_dest.sin_addr.s_addr = addr.s_addr;
	}
	else
	{
		hp = gethostbyname(_ip);
		if ( !hp )
		{
			snprintf(p_buf, size, "%s Unable to resolve", _ip);	
			return -3;
		}
		memcpy(&(m_ping_dest.sin_addr), hp->h_addr,(size_t)hp->h_length < sizeof(m_ping_dest.sin_addr) ? (size_t)hp->h_length : sizeof(m_ping_dest.sin_addr));
	}

	{
		struct protoent *proto;
		proto = getprotobyname("icmp");
		if ((m_ping_sockfd = socket(AF_INET, SOCK_RAW,
						(proto ? proto->p_proto : 1))) < 0) {        /* 1 == ICMP */
			snprintf( p_buf, size, "Create socket failed. Or you are not root !" );
			return -1;
		}
	}
	char addr_p[32] = {0};
	snprintf(p_buf, size, "PING %s(%s): %d bytes data in ICMP packets.", _ip,inet_ntop(AF_INET,&m_ping_dest.sin_addr,addr_p,(socklen_t )sizeof(addr_p)), datalen);
	return 0;	
}

static int sw_swstbmonitor_ping_get_data( char *p_buf, int size )
{
	int iIcmpDataSize = 0;
	struct sockaddr_in from;
	socklen_t fromlen;
	char ip_hdr[128];
	STD_IcmpHeader *icmp_hdr=NULL;
	int iRecvSize=0;
	int timeout = 1500;

	iIcmpDataSize = 10;
	bzero( ip_hdr, sizeof(ip_hdr) );
	fill_IcmpData(ip_hdr, iIcmpDataSize);
	fill_IcmpHeader(ip_hdr, iIcmpDataSize);
	icmp_hdr = (STD_IcmpHeader *)ip_hdr;
	if ( sendto(m_ping_sockfd, ip_hdr, sizeof(STD_IcmpHeader) + iIcmpDataSize, 0, (struct sockaddr*)&m_ping_dest, sizeof(m_ping_dest)) < 0)
	{
		return -1;
	}
	memset(&from, 0, sizeof from);
	bzero( ip_hdr, sizeof(ip_hdr) );
	fromlen = sizeof(from);
	{
		struct timeval tv;

		tv.tv_sec = timeout/1000;
		tv.tv_usec = timeout-timeout/1000*1000;  

		//......
		if ( setsockopt( m_ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv) ) < 0 )
		{
			return -1;
		}
	}
	iRecvSize = recvfrom(m_ping_sockfd, ip_hdr, 128, 0, (struct sockaddr*)&from,  &fromlen);
	if (iRecvSize > 0)
		return decode_IpIcmp(ip_hdr, iRecvSize, p_buf, size);
	else
		return -1;	
}

static void fill_IcmpData(char *buf, int datasize)
{
	if (buf)	{
		int i;
		char ch = 0;
		char* icmpdata = buf + sizeof(STD_IcmpHeader);
		for (i = 0; i < datasize; i++)		{
			ch = 'A' + i%('z' - 'A');
			*(icmpdata + i) = ch;
		}
	}
}

static void fill_IcmpHeader(char *buf, int datasize)
{
	static unsigned short seq_no = 0;
	STD_IcmpHeader *icmp_hdr = (STD_IcmpHeader *)buf;
	if (icmp_hdr)	{
		icmp_hdr->i_type = ICMP_ECHO;
		icmp_hdr->i_code = 0;
		icmp_hdr->i_cksum = 0;
		icmp_hdr->i_id = (unsigned short)getpid(); 
		icmp_hdr->i_seq = seq_no++;
		gettimeofday ( &icmp_hdr->i_timestamp, NULL );
		icmp_hdr->i_cksum = in_cksum((unsigned short*)buf, sizeof(STD_IcmpHeader) + datasize);
	}
}

// decode
static int decode_IpIcmp(char *inbuf, int insize, char *outbuf, int size)
{
	int outsize = 0;
	STD_IpHeader *ip_hdr = (STD_IpHeader *)inbuf;
	unsigned short iphdrlen;
	struct timeval tvrecv;
	if(inbuf[0] == '0')
		return -1;
	gettimeofday(&tvrecv, NULL);//记录接收时间
	if (ip_hdr)
	{
		iphdrlen = ip_hdr->h_len << 2; // number of 32-bit words *4 = bytes
		if (insize < iphdrlen + ICMP_MIN)
			outsize = snprintf( outbuf, size, "Reply %d bytes Too few\r\n", insize);
		else
		{
			STD_IcmpHeader *icmp_hdr = (STD_IcmpHeader *)(inbuf + iphdrlen);
			unsigned long timestamp = 0;
			struct sockaddr_in from;
			struct timeval *tvsend;
			double rtt;

			timestamp = (unsigned long)time(NULL);
			tvsend = &icmp_hdr->i_timestamp;
			tv_sub(&tvrecv,tvsend); /*接收和发送的时间差*/
			rtt=tvrecv.tv_sec*1000+((double)tvrecv.tv_usec)/1000; /*以毫秒为单位计算rtt*/
			if(rtt < 0.000)
				return -1;
			from.sin_addr.s_addr = ip_hdr->sourceIP;
			char addr_p[32] = {0};
			outsize = snprintf(outbuf, size, "Reply %d bytes from: %s icmp_seq=%d TTL=%d time=%.3fms",
					insize,
					inet_ntop(AF_INET,&from.sin_addr,addr_p,(socklen_t )sizeof(addr_p)),
					icmp_hdr->i_seq,
					ip_hdr->ttl,
					rtt
					);
		}
		return outsize;
	}
	return -1;
}


//puclic code
//.........
//...........16......(ones-complement sum)
static unsigned short in_cksum(unsigned short* addr, int len)
{
	int		nleft = len;
	int		sum = 0;
	unsigned short* w = addr;
	unsigned short answer = 0;

	while(nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if(nleft == 1) {
		*(unsigned char*)(&answer) = *(unsigned char*)w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;

	return (answer);
}

static void tv_sub(struct timeval *out,struct timeval *in)
{
	if( (out->tv_usec-=in->tv_usec)<0)
	{
		--out->tv_sec;
		out->tv_usec+=1000000;
	}
	out->tv_sec-=in->tv_sec;
}

/* ??ȡ?ֶ?ֵ */
char *xstrgetval( char *buf, char *name, char *value, int valuelen )
{
	char *s, *e, *p;
	int len = strlen(buf );

	memset( value, 0, valuelen );

	/* ?????汾??Ϣ?ļ? */
	p = s = buf;
	while( p < buf+len )
	{
		/* ȥ????ʼ?Ŀո? */
		while( *p == ' ' ||  *p == ',' ||  *p == '\t' || *p == '\r' || *p == '\n' )
			p++;

		/* ??¼?п?ʼ */
		s = p;

		/* ?ҵ??н??? */
		while( *p != '\r' && *p != '\n' && *p != '\0' )
			p++;
		e = p;
		/* ?ҵ???Ҫ???ֶ? */
		if( strncasecmp( s, name, strlen(name) ) == 0 && !isalpha(s[strlen(name)]) )
		{/* ?????ֶβ????໥????,ĿǰֻҪ????Ϊ??ĸ?Ͳ?????Ҫ???ֶ? */
			/* ?ҵ????ƽ??? */
			p = s;
			while( *p != ':' &&  *p != '=' &&  *p != '\t' && *p != '\r' && *p != '\n' && *p != '\0' )
				p++;
			if( *p == 0 || e <= p )
				goto NEXT_LINE;

			/* ?ҵ?ֵ??ʼ */
			p++;
			while( *p == ':' ||  *p == '=' ||  *p == ',' ||  *p == '\t' || *p == '\r' || *p == '\n' )
				p++;

			if( p < e && *p != '\0' )
			{
				memcpy( value, p, valuelen-1<e-p ? valuelen-1:e-p );
				return value;
			}
			else
				return NULL;
		}
NEXT_LINE:
		p = e + 1;
	}
	return NULL;
}

