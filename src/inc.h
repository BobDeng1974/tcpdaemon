#ifndef _H_INC_
#define _H_INC_

/*
 * tcpdaemon - TCP���ӹ����ػ�
 * author      : calvin
 * email       : calvinwilliams.c@gmail.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define TCPDAEMON_CALLMODE_DAEMON		1 /* ����ģʽ1:���ػ�ģʽ */

#include "LOGC.h"
#include "service.h"

#include "tcpdaemon.h"

/* �汾�� */
extern char		__version_tcpdaemon[] ;

/* ��ƽ̨�� */
#if ( defined __linux__ ) || ( defined __unix )
#define PID_T			pid_t
#define TID_T			pthread_t
#define THANDLE_T		pthread_t
#define OBJECTHANDLE		void *
#define ERRNO			errno
#define DLERROR			dlerror()
#define OPEN			open
#define CLOSE			close
#define CLOSESOCKET		close
#define VSNPRINTF		vsnprintf
#define SNPRINTF		snprintf
#define TLS			__thread
#define SOCKLEN_T		socklen_t
#define PIPE(_pipes_)		pipe(_pipes_)
#define SLEEP(_seconds_)	sleep(_seconds_)
#elif ( defined _WIN32 )
#define PID_T			long
#define TID_T			unsigned long
#define THANDLE_T		HANDLE
#define OBJECTHANDLE		HINSTANCE
#define ERRNO			GetLastError()
#define DLERROR			""
#define OPEN			_open
#define CLOSE			_close
#define CLOSESOCKET		closesocket
#define VSNPRINTF		_vsnprintf
#define SNPRINTF		_snprintf
#define TLS			__declspec( thread )
#define SOCKLEN_T		long
#define PIPE(_pipes_)		_pipe((_pipes_),256,_O_BINARY)
#define SLEEP(_seconds_)	Sleep(_seconds_*1000)
#endif

/* �ź���ֵ�ṹ */
union semun
{
	int		val ;
	struct semid_ds	*buf ;
	unsigned short	*array ;
	struct seminfo	*__buf ;
};

/* �ػ������ṹ */
typedef struct
{
	int		fd[ 2 ] ;
} PIPE_T ;

struct TcpdaemonServerEnv
{
	struct TcpdaemonEntryParam	*pep ;
	
	OBJECTHANDLE			so_handle ; /* ��̬��򿪾�� */
	func_tcpmain			*pfunc_tcpmain ; /* ��̬�����:ͨѶ����Э�鼰Ӧ�ô���ص����� */
	void				*param_tcpmain ; /* ��ڲ��� */
	int				listen_sock ; /* �����׽��� */
	
	/* ��Instance-Fork����ģ��ʹ�� */
	long				process_count ;
	
	/* ��Leader-Follow���̳�ģ��ʹ�� */
	int				accept_mutex ; /* accept�ٽ��� */
	
	long				index ; /* ����������� */
	
	long				requests_per_process ; /* �������̵�ǰ�������� */
	
	PID_T				*pids ;
	PIPE_T				*alive_pipes ; /* �������̻�֪������̻��ܵ�������˵�ǹ������֪ͨ�������̽���������ܵ� */
					/* parent fd[1] -> child fd[0] */
	
	/* ��Leader-Follow�̳߳�ģ��ʹ�� */
	THANDLE_T			*thandles ;
	TID_T				*tids ;
} ;

int ParseCommandParameter( int argc , char *argv[] , struct TcpdaemonEntryParam *pep );
int CheckCommandParameter( struct TcpdaemonEntryParam *pep );

struct TcpdaemonServerEnv *DuplicateServerEnv( struct TcpdaemonServerEnv *pse );

void usage();
void version();

#if ( defined _WIN32 )
extern CRITICAL_SECTION			accept_critical_section; /* accept�ٽ��� */
#endif

#ifdef __cplusplus
}
#endif

#endif

