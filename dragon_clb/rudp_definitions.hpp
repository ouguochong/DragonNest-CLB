#pragma once
#pragma warning( disable:4996 )

#define	_PACKET_TYPE(A)			((A) & 7)
#define _PACKET_SEQ(A)			(unsigned short)((A) & (~7))
#define _PACKET_ACK(A)			(unsigned short)((A) & (~7))
#define _PACKET_PINGCNT(A)		(unsigned short)((A) & (~7))
#define _PACKET_ACKN(A)			((A)>>3)
#define _PACKET_HEADER(T, S)	((T) | (S))
#define PING_TEST_COUNT	4							//	4회 테스트 (평균치 사용)

const unsigned int _RESEND_TIME = 250;				//	RTO : retransmission time out
const unsigned int _RTO_LIST_MAX = 24 * 8;			// RTOLISTMAX : retrans instance max size;
const unsigned int _RTO_TICK_MAX = 1000 * 30;
const unsigned int _SELECT_TIMEOUT_SRV = 1000;		//	select timeout time server
const unsigned int _SELECT_BUSYTIME = 100;			//	Message processing time limit
const unsigned int _CHECK_UNREACHABLE = 150;		//	Message timing to check whether kilobytes
const unsigned int _IDLE_PROCESS = 100;				//	The message queue associated processing timing
const unsigned int _PINGSENDTICK = 13 * 1000;		// Ping check interval
const unsigned int _CHECK_RECVTICK = 45 * 1000;		// Richard Bloom frozen zombies with a separate checking packet loss to consider noise a little longer

#pragma pack(1)
typedef struct _PACKET_QUEUE
{
	union
	{
		unsigned short seq;
		unsigned short ack;
	};

	char type;
	unsigned short len;
	unsigned long tick;
	unsigned long origintick;
	char data[1];
} PACKET_QUEUE;

typedef struct _RELIABLE_UDP_HEADER
{
	union 
	{
		unsigned short combo;
		unsigned char flags; // 0x01 => ackreq, 0x02 => seq, 0x03 => ping, 0x06 => pong, 0x04 => ack, 0x05 => ack seq 0x00 => NULL
		unsigned short ack;
		unsigned short seq;
		unsigned short acknum;
	};

	unsigned char crc;
} RELIABLE_UDP_HEADER;
#pragma pack()

enum 
{
	_FAST,							//그냥 냅다 쏴효~
	_RELIABLE_NOORDER,				//순서는 상관 없지만 학실히~ 보내주세효~
	_RELIABLE,						//순서도 상관있고 학실히 보내주세효~
};

typedef struct _ADDR 
{
	int port;
	unsigned char ip[4];
} ADDR;

static unsigned long _inet_addr(const char* cp)
{
	int s_b[4] = { 0, 0, 0, 0};
	struct in_addr in;
	sscanf(cp, "%d.%d.%d.%d", &s_b[0], &s_b[1], &s_b[2], &s_b[3]);
	in.S_un.S_un_b.s_b1 = s_b[0];
	in.S_un.S_un_b.s_b2 = s_b[1];
	in.S_un.S_un_b.s_b3 = s_b[2];
	in.S_un.S_un_b.s_b4 = s_b[3];
	return in.S_un.S_addr;
}

static unsigned long _inet_addr(const WCHAR* cp)
{
	int s_b[4] = { 0, 0, 0, 0};
	struct in_addr in;
	swscanf(cp, L"%d.%d.%d.%d", &s_b[0], &s_b[1], &s_b[2], &s_b[3]);
	in.S_un.S_un_b.s_b1 = s_b[0];
	in.S_un.S_un_b.s_b2 = s_b[1];
	in.S_un.S_un_b.s_b3 = s_b[2];
	in.S_un.S_un_b.s_b4 = s_b[3];
	return in.S_un.S_addr;
}

static unsigned long _inet_addr(const unsigned char* ip)
{
	struct in_addr in;
	in.S_un.S_un_b.s_b1 = ip[0];
	in.S_un.S_un_b.s_b2 = ip[1];
	in.S_un.S_un_b.s_b3 = ip[2];
	in.S_un.S_un_b.s_b4 = ip[3];
	return in.S_un.S_addr;
}

static void _inet_addr(unsigned long i, char* p)
{
	sprintf(p, "%d.%d.%d.%d", reinterpret_cast<unsigned char*>(&i)[0], reinterpret_cast<unsigned char*>(&i)[1],
		reinterpret_cast<unsigned char*>(&i)[2], reinterpret_cast<unsigned char*>(&i)[3]);
}