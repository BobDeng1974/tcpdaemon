#ifndef _H_TCPDAEMON_
#define _H_TCPDAEMON_

/*
 * tcpdaemon - TCP���ӹ����ػ�
 * author      : calvin
 * email       : calvinwilliams.c@gmail.com
 * LastVersion : 2014-06-29	v1.0.0		����
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <utmpx.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <netinet/tcp.h>

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

/* �汾�� */
extern char		__version_tcpdaemon[] ;

/* ����־���� */
#define LOGLEVEL_DEBUG		0
#define LOGLEVEL_INFO		1
#define LOGLEVEL_WARN		2
#define LOGLEVEL_ERROR		3
#define LOGLEVEL_FATAL		4

void SetLogFile( char *format , ... );
void SetLogLevel( int log_level );
int FatalLog( char *c_filename , long c_fileline , char *format , ... );
int ErrorLog( char *c_filename , long c_fileline , char *format , ... );
int WarnLog( char *c_filename , long c_fileline , char *format , ... );
int InfoLog( char *c_filename , long c_fileline , char *format , ... );
int DebugLog( char *c_filename , long c_fileline , char *format , ... );

/*
*	���ػ�ģʽ : tcpdaemon(main->tcpdaemon->tcpmain) + xxx.so(tcpmain)
*/

#define TCPMAIN		"tcpmain"

/* ͨѶ����Э�鼰Ӧ�ô���ص�����ԭ�� */
typedef int func_tcpmain( void *param_tcpmain , int sock , struct sockaddr *addr );

/*
*	��������ģʽ : xxx.exe(main->tcpmain) + tcpdaemon.so(tcpdaemon)
*/

#define TCPDAEMON_CALLMODE_MAIN	2 /* ����ģʽ:��������ģʽ */

/* ����ڲ����ṹ */
struct TcpdaemonEntryParam
{
	int		call_mode ;
	
	char		server_model[ 2 + 1 ] ;
	long		max_process_count ;
	long		max_requests_per_process ;
	char		ip[ 20 + 1 ] ;
	long		port ;
	char		so_pathfilename[ MAXLEN_FILENAME + 1 ] ;
	
	func_tcpmain	*pfunc_tcpmain ;
	void		*param_tcpmain ;
	
	int		log_level ;
	int		tcp_nodelay ;
	int		tcp_linger ;
} ;

/* ����ں��� */
int tcpdaemon( struct TcpdaemonEntryParam *pcp );

/*
*	�칹ģʽ : xxx.exe(tcpmain) + tcpdaemon.so(main->tcpdaemon->tcpmain)
*/

#define TCPDAEMON_CALLMODE_NOMAIN	3 /* ����ģʽ:�칹ģʽ */

/* ��ڲ����ṹȫ������ */
extern struct TcpdaemonEntryParam	g_TcpdaemonEntryParameter ;
extern func_tcpmain			*g_pfunc_tcpmain ;
extern void				*g_param_tcpmain ;

#ifdef __cplusplus
}
#endif

#endif

