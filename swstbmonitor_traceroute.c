/***************************************************************************
* Copyright: sunniwell
* file     : nmpserver.c
* AUTHOR   : ???ٸ?
* brief    : ??Ϊͳһ???���?߷??????ӿ?
****************************************************************************/
#include "swstbmonitor_priv.h"
#include "swstbmonitorserver.h"

#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include "swthrd.h"

#define TRACEROUTE_PORT 	(32768+666)
struct trace_opacket {
    struct ip ip;
    struct udphdr udp;
    u_char seq;                /* sequence number of this packet */
    u_char ttl;                /* ttl packet left with */
    struct timeval tv;        /* time packet left */
};
struct trace_opacket *outpacket = NULL;        /* last output (udp) packet */
static int m_trace_raw_s = -1;
static int m_trace_icmp_s = -1;
static bool m_traceroute_stop = false;
static int m_trace_seq = 0;
static int m_trace_ttl = 0;
static HANDLE m_trace_thread = NULL; 
struct sockaddr_in m_trace_to_addr;
static uint16_t m_trace_ident; 

static sw_stbmonitor_reply_callback m_cbfs_reply = NULL;
static sw_stbmonitor_callback_funcs_t *m_cbfs = NULL;

static int sw_swstbmonitor_traceroute_init(char *_ip, char *p_buf, int size);
static int sw_swstbminitor_traceroute_get_data(char *p_buf, int size);
static void sw_hwnmp_traceroute_exit();
static int traceroute_send_probe(int seq, int ttl);
static int traceroute_wait_for_reply(int sock, struct sockaddr_in *from, char *buf, int buflen);
static int traceroute_packet_ok(uint8_t *buf, int buflen, struct sockaddr_in *from, int seq);

int sw_stbmonitor_set_traceroute_reply_callback( sw_stbmonitor_reply_callback cbfs_reply )
{
	m_cbfs_reply = cbfs_reply;
	return 0;
}
int  sw_stbmonitor_traceroute_stop()
{
	m_traceroute_stop = true;
	m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^traceroute", NULL, NULL); 
	return 0;
}
static bool sw_stbmonitor_traceroute_start_proc(unsigned long wParam, unsigned long lParam)
{
	char *url = (char *)wParam;
	char ip[256] = {0};
	char p_buf[512] = {0};
	int ret = 0;
	snprintf(ip, sizeof(ip), url);
	printf("traceroute thread :ip = %s, url = %s %d\n", ip, url, sizeof(p_buf));
	if ( ip[0] == '\0' )
	{
		m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^traceroute^no ip address", NULL,NULL); 
		return false;
	}
	ret = sw_swstbmonitor_traceroute_init(ip, p_buf, sizeof(p_buf));
	if ( ret != 0 )
	{
		m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^traceroute", p_buf,""); 
		return false;
	}
	m_traceroute_stop = false;
	for (;;)
	{
		memset(p_buf, 0, sizeof(p_buf));
		ret = sw_swstbminitor_traceroute_get_data(p_buf, sizeof(p_buf));
		if ( *p_buf != '\0' )
			m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^traceroute", p_buf,""); 
		if ( (ret <= 0) || m_traceroute_stop || (30 <= ret))
		{
			sw_thrd_delay(300);
			break;
		}
		sw_thrd_delay(300);
	}
	//sw_thrd_delay(300);
	//m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^traceroute", NULL, NULL);
	sw_hwnmp_traceroute_exit();
	m_trace_thread = NULL;
	return false;
}
int sw_stbmonitor_traceroute_start( sw_stbmonitor_callback_funcs_t *cbfs, char* url )
{
	printf("traceroute start!\n");
m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^traceroute^ traceroute to ", url, " 30 hops max, 40 byte packets");
	if(cbfs != NULL)
		m_cbfs = cbfs;
	if (NULL == m_trace_thread)
	{
		m_trace_thread = sw_thrd_open("tTraceroute_Proc", 10, 0, 1028*1024, (threadhandler_t)sw_stbmonitor_traceroute_start_proc, (unsigned long)url, 0);
	}
	if (m_trace_thread)
	{
		sw_thrd_resume(m_trace_thread);
	}
#if 0
	for ( ; ; )
	{
		ret = sw_swstbminitor_traceroute_get_data(p_buf, sizeof(p_buf));
		if ( *p_buf != '\0' )
			m_cbfs_reply(STBMONITOR_REPLY_OK, "connect^traceroute", p_buf,""); 
		if ( ret <= 0 )
			break;	
	}
	sw_hwnmp_traceroute_exit();
#endif
	return 0;
}

static int sw_swstbmonitor_traceroute_init(char *_ip, char *p_buf, int size)
{
    struct hostent *hp;
	struct sockaddr_in *to;
	struct in_addr addr;
	if ( outpacket != NULL || m_trace_raw_s >= 0 || m_trace_icmp_s >= 0 )
	{
		snprintf(p_buf, size, "traceroute has run");
		return -1;
	}
    to = (struct sockaddr_in *)&m_trace_to_addr;
    to->sin_family = AF_INET;
    if ( inet_aton(_ip, &addr) != 0)
		to->sin_addr.s_addr = addr.s_addr;
    else 
	{
		hp = gethostbyname(_ip);
        if (hp) 
		{
            to->sin_family = hp->h_addrtype;
			memcpy(&(to->sin_addr), hp->h_addr,(size_t)hp->h_length < sizeof(to->sin_addr) ? (size_t)hp->h_length : sizeof(to->sin_addr) );
        }
		else 
		{
            snprintf(p_buf,size,"Unable to resolve hostname\n");
            return(-1001);
        }
    }
    outpacket = (struct trace_opacket *)malloc(sizeof(struct trace_opacket));
    if ( outpacket == NULL)
	{
		snprintf(p_buf, size, "malloc memory failed");
		return(-1);
    }
    bzero((char *)outpacket, sizeof(struct trace_opacket));
    outpacket->ip.ip_dst = to->sin_addr;
    outpacket->ip.ip_tos = 0;
    outpacket->ip.ip_v = IPVERSION;
    outpacket->ip.ip_id = 0;

    m_trace_ident = (pthread_self() & 0xffff) | 0x8000;

    if ((m_trace_icmp_s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) 
	{
        snprintf(p_buf, size, "Create icmp socket failed. Or you are not root !");
		sw_hwnmp_traceroute_exit();
        return(-5);
    }
    if ((m_trace_raw_s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) 
	{
        snprintf(p_buf, size, "Create raw socket failed. Or you are not root !");
		sw_hwnmp_traceroute_exit();
        return(-5);
    }
	m_trace_seq = 0;
	m_trace_ttl = 1;
	return 0;
}

static int sw_swstbminitor_traceroute_get_data(char *p_buf, int size)
{
    struct sockaddr_in from;
    int i = 0, probe = 0, bufLen = 0;
	u_long lastaddr = 0;
	int got_there = 0;
	uint8_t packet[512]; 
	double delaytime;
	if ( m_trace_ttl > 30 || m_trace_raw_s < 0 || m_trace_icmp_s < 0)
	{
		sw_hwnmp_traceroute_exit();
		*p_buf = '\0';
		return 0;
	}
	for (probe = 0; probe < 3; probe++)
	{
		int cc;
		struct timeval t1, t2;
		struct timezone tz;
		char addr_p[32] = {0};
		
		gettimeofday(&t1, &tz);
		if ( 0 != traceroute_send_probe(++m_trace_seq, m_trace_ttl) )
		{
			snprintf(p_buf, size, "fail to send udp data");
			sw_hwnmp_traceroute_exit();
			return -1;
		}
		if ((cc = traceroute_wait_for_reply(m_trace_icmp_s, &from, (char*)packet, sizeof(packet)))) 
		{
			gettimeofday(&t2, &tz);
			if ((i = traceroute_packet_ok(packet, cc, &from, m_trace_seq)))
			{
				memset(p_buf, 0, size);
				bufLen = snprintf(p_buf, size, "%2d ", m_trace_ttl);
				if (from.sin_addr.s_addr != lastaddr)
				{
					bufLen += snprintf(&p_buf[bufLen], size-bufLen, " %s", inet_ntop(AF_INET,&from.sin_addr,addr_p,(socklen_t )sizeof(addr_p)));
					lastaddr = from.sin_addr.s_addr;
				}
				delaytime = (double)(t2.tv_sec - t1.tv_sec) * 1000.0 + (double)(t2.tv_usec - t1.tv_usec) / 1000.0;
				bufLen += snprintf(&p_buf[bufLen], size-bufLen, "  %g ms", delaytime);
				if ( i - 1 == ICMP_UNREACH_PORT || i-1 == ICMP_UNREACH_PROTOCOL )
				{
					++got_there;
				}
				break;
			}
		}
		else
		{
			memset(p_buf, 0, size);
			bufLen = snprintf(p_buf, size, "%2d ", m_trace_ttl);
			bufLen += snprintf(&p_buf[bufLen], size-bufLen, " *");
		}
	}
	m_trace_ttl++;
	if ( got_there )
	{
		sw_hwnmp_traceroute_exit();
		return 0;
	}
	return m_trace_ttl;
}

static void sw_hwnmp_traceroute_exit()
{
	if ( m_trace_raw_s >= 0 )
		close( m_trace_raw_s );
	if ( m_trace_icmp_s >= 0 )
		close( m_trace_icmp_s );
	if ( outpacket )
		free( outpacket );
	m_trace_raw_s = -1;
	m_trace_icmp_s = -1;
	outpacket = NULL;
	m_trace_ttl = 0;
}

static int traceroute_send_probe(int seq, int ttl)
{
    struct trace_opacket *op = outpacket;
    struct ip *ip = &op->ip;
    struct udphdr *up = &op->udp;
	struct timezone tz;
	int i;
    ip->ip_off = 0;
    ip->ip_hl = sizeof(*ip) >> 2;        
    ip->ip_p = IPPROTO_UDP;
    ip->ip_len = sizeof(struct trace_opacket);
    ip->ip_ttl = ttl;
    ip->ip_v = IPVERSION;
    ip->ip_id = htons(m_trace_ident+seq);
	up->source= htons(m_trace_ident);
    up->dest = htons(TRACEROUTE_PORT+seq);
    up->len = htons((u_short)(sizeof(struct trace_opacket) - sizeof(struct ip)));
    up->check = 0;
    op->seq = seq;
    op->ttl = ttl;
    (void) gettimeofday(&op->tv, &tz);
    i = sendto(m_trace_raw_s, (char *)outpacket, sizeof(struct trace_opacket), 0, (struct sockaddr *)&m_trace_to_addr,
               sizeof(struct sockaddr));
    if (i < 0 || i != sizeof(struct trace_opacket))
	{/* 发送失败消息 */
        if ( i< 0 )
			perror("sendto");
		return -1;
    }
	return 0;
}

static int traceroute_wait_for_reply(int sock, struct sockaddr_in *from, char *buf, int buflen)
{
	fd_set fds;
	struct timeval wait;
	int rlen = 0;
	int fromlen = sizeof(*from);

	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	wait.tv_sec = 2;
	wait.tv_usec = 0;

	if ( select(sock+1, &fds, (fd_set *)0, (fd_set *)0, &wait) > 0 )
		rlen = recvfrom(sock, (char *)buf, buflen, 0,
							(struct sockaddr *)from, (socklen_t*)&fromlen);
	return rlen;
}

static int traceroute_packet_ok(uint8_t *buf, int buflen, struct sockaddr_in *from, int seq)
{
	register struct icmp *icp;
	u_char type, code;
	int hlen;
	struct ip *ip;

	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;        /*header length,include option*/
	if (buflen < hlen + ICMP_MINLEN)
	{
		return (0);
	}
	buflen -= hlen;
	icp = (struct icmp *)(buf + hlen);
	type = icp->icmp_type; 
	code = icp->icmp_code;

	if ((type == ICMP_TIMXCEED && code == ICMP_TIMXCEED_INTRANS) || type == ICMP_UNREACH) 
	{
		struct ip *hip;
		struct udphdr *up;

		hip = &icp->icmp_ip;
		hlen = hip->ip_hl << 2;
		up = (struct udphdr *)((u_char *)hip + hlen);
		if (hlen + 12 <= buflen && hip->ip_p == IPPROTO_UDP &&
			up->source == htons(m_trace_ident) &&
			up->dest == htons(TRACEROUTE_PORT+seq) )
		return (type == ICMP_TIMXCEED? -1 : code+1);
	}
	return 0;
}

