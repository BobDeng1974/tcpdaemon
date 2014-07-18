#ifndef _H_TCPDAEMON_
#define _H_TCPDAEMON_

/*
 * tcpdaemon - TCP���ӹ����ػ�
 * author      : calvin
 * email       : calvinwilliams.c@gmail.com
 * LastVersion : 2014-04-27	v1.0.1		����
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#if ( defined __linux__ ) || ( defined __unix )
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <utmpx.h>
#include <netdb.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#elif ( defined _WIN32 )
#include <windows.h>
#include <io.h>
#include <process.h>
#endif

#if ( defined __unix ) || ( defined __linux__ )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC
#endif
#elif ( defined _WIN32 )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC		_declspec(dllexport)
#endif
#endif

#ifndef WINAPI
#define WINAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ������ */
#ifndef MAXLEN_FILENAME
#define MAXLEN_FILENAME			256
#endif

#ifndef STRCMP
#define STRCMP(_a_,_C_,_b_) ( strcmp(_a_,_b_) _C_ 0 )
#define STRNCMP(_a_,_C_,_b_,_n_) ( strncmp(_a_,_b_,_n_) _C_ 0 )
#endif

#ifndef MEMCMP
#define MEMCMP(_a_,_C_,_b_,_n_) ( memcmp(_a_,_b_,_n_) _C_ 0 )
#endif

/* ��ƽ̨�� */
#if ( defined __linux__ ) || ( defined __unix )
#define RECV			recv
#define SEND			send
#elif ( defined _WIN32 )
#define RECV			recv
#define SEND			send
#endif

/* ����־���� */
#ifndef LOGLEVEL_DEBUG
#define LOGLEVEL_DEBUG		0
#define LOGLEVEL_INFO		1
#define LOGLEVEL_WARN		2
#define LOGLEVEL_ERROR		3
#define LOGLEVEL_FATAL		4
#endif

/*
*	���ػ�ģʽ : tcpdaemon(main->tcpdaemon->tcpmain) + xxx.so(tcpmain)
*/

#define TCPMAIN		"tcpmain"

/* ͨѶ����Э�鼰Ӧ�ô���ص�����ԭ�� */
typedef int func_tcpmain( void *param_tcpmain , int sock , struct sockaddr *addr );

/*
*	��������ģʽ : xxx.exe(main->tcpmain) + tcpdaemon.so(tcpdaemon)
*/

#define TCPDAEMON_CALLMODE_MAIN	2 /* ����ģʽ2:��������ģʽ */

/* ����ڲ����ṹ */
struct TcpdaemonEntryParam
{
	int		daemon_level ;
	
	char		log_pathfilename[ MAXLEN_FILENAME + 1 ] ;
	int		log_level ;
	
	int		call_mode ;	/* LF �쵼��-׷����Ԥ�������̳�ģ�� for UNIX,Linux
					   IF ��ʱ��������ģ�� for UNIX,Linux
					   WIN-TLF �쵼��-׷����Ԥ�����̳߳�ģ�� for win32
					*/
	
	char		server_model[ 10 + 1 ] ;
	long		max_process_count ;
	long		max_requests_per_process ;
	char		ip[ 20 + 1 ] ;
	long		port ;
	char		so_pathfilename[ MAXLEN_FILENAME + 1 ] ;
	
	func_tcpmain	*pfunc_tcpmain ;
	void		*param_tcpmain ;
	
	int		tcp_nodelay ;
	int		tcp_linger ;

	/* ����Ϊ�ڲ�ʹ�� */
	int		install_winservice ;
	int		uninstall_winservice ;
} ;

/* ����ں��� */
_WINDLL_FUNC int tcpdaemon( struct TcpdaemonEntryParam *pcp );

#ifdef __cplusplus
}
#endif

#endif

