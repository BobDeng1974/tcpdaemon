#ifndef _H_TCPDAEMON_
#define _H_TCPDAEMON_

/*
 * tcpdaemon - TCP连接管理守护
 * author      : calvin
 * email       : calvinwilliams@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#if ( defined __linux__ ) || ( defined __unix )
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
#include <unistd.h>
#include <dlfcn.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <utmpx.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <pwd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#elif ( defined _WIN32 )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#include <io.h>
#include <process.h>
#include <direct.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if ( defined __unix ) || ( defined __linux__ )
#define _WINDLL_FUNC
#elif ( defined _WIN32 )
#define _WINDLL_FUNC		_declspec(dllexport)
#endif

/* 公共宏 */
#ifndef STRCMP
#define STRCMP(_a_,_C_,_b_) ( strcmp(_a_,_b_) _C_ 0 )
#define STRNCMP(_a_,_C_,_b_,_n_) ( strncmp(_a_,_b_,_n_) _C_ 0 )
#endif

#ifndef MEMCMP
#define MEMCMP(_a_,_C_,_b_,_n_) ( memcmp(_a_,_b_,_n_) _C_ 0 )
#endif

/* 跨平台宏 */
#if ( defined __linux__ ) || ( defined __unix )
#define RECV			recv
#define SEND			send
#elif ( defined _WIN32 )
#define RECV			recv
#define SEND			send
#endif

/* 跨平台宏 */
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
#define SOCKLEN_T		socklen_t
#define PIPE(_pipes_)		pipe(_pipes_)
#define SLEEP(_seconds_)	sleep(_seconds_)
#define CHDIR			chdir
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
#define SOCKLEN_T		long
#define PIPE(_pipes_)		_pipe((_pipes_),256,_O_BINARY)
#define SLEEP(_seconds_)	Sleep(_seconds_*1000)
#define CHDIR			_chdir
#endif

/* 日志等级 */
#ifndef LOGLEVEL_DEBUG
#define LOGLEVEL_DEBUG		0
#define LOGLEVEL_INFO		1
#define LOGLEVEL_WARN		2
#define LOGLEVEL_ERROR		3
#define LOGLEVEL_FATAL		4
#endif

/* 版本号字符串 */
extern char		*__TCPDAEMON_VERSION ;

/*
 * 工具层
 */


/*
 * 通讯层
 */

/* 主守护模式 : tcpdaemon(main->tcpdaemon->tcpmain) + xxx.so(tcpmain) */

#define TCPMAIN		"tcpmain"

/* 通讯数据协议及应用处理回调函数原型 */
struct TcpdaemonEntryParameter ;
struct TcpdaemonServerEnvirment ;
typedef int func_tcpdaemon( struct TcpdaemonEntryParameter *p_para );
typedef int func_tcpmain( struct TcpdaemonServerEnvirment *p_env , int sock , struct sockaddr *addr );
				/*
				IF
								p_env , int accepted_sock , struct sockaddr *accepted_addr
				LF
								p_env , int accepted_sock , struct sockaddr *accepted_addr
				MPIO
					OnAcceptingSocket	p_env , int accepted_sock , struct sockaddr *accepted_addr
					OnClosingSocket		p_env , int events , void *custem_data_ptr
					OnSendingSocket		p_env , int events , void *custem_data_ptr
					OnReceivingSocket	p_env , int events , void *custem_data_ptr
					OnClosingSocket		p_env , 0 , void *custem_data_ptr
				WIN-TLF
								p_env , int accepted_sock , struct sockaddr *accepted_addr
				*/

/* 函数调用模式 : xxx.exe(main->tcpmain) + tcpdaemon.so(tcpdaemon) */

#define TCPDAEMON_CALLMODE_MAIN		0 /* 运行模式:主函数调用模式 */

/* 主入口参数结构 */
struct TcpdaemonEntryParameter
{
	int		daemon_level ;	/* 是否转化为守护服务 1:转化 0:不转化（缺省） */
	
	char		log_pathfilename[ 256 + 1 ] ;	/* 日志输出文件名，不设置则输出到标准输出上 */
	int		log_level ;	/* 日志等级，不设置则缺省DEBUG等级 */
	
	int		call_mode ;	/* 应用接口模式
					   TCPDAEMON_CALLMODE_CALLBACK:主守护模式
					   TCPDAEMON_CALLMODE_MAIN:函数调用模式
					*/
	
	char		server_model[ 10 + 1 ] ;	/* TCP连接管理模型
							LF:领导者-追随者预派生进程池模型 for UNIX,Linux
							IF:即时派生进程模型 for UNIX,Linux
							WIN-TLF:领导者-追随者预派生线程池模型 for win32
							*/
	long		process_count ;	/* 当为领导者-追随者预派生进程池模型时为工作进程池进程数量，当为即时派生进程模型时为最大子进程数量 */
	long		max_requests_per_process ;	/* 当为领导者-追随者预派生进程池模型时为单个工作进程最大处理应用次数 */
	char		ip[ 20 + 1 ] ;	/* 本地侦听IP */
	int		port ;	/* 本地侦听PORT */
	char		so_pathfilename[ 256 + 1 ] ;	/* 用绝对路径或相对路径表达的应用动态库文件名 */
	
	char		work_user[ 64 + 1 ] ;	/* 切换为其它用户运行。可选 */
	char		work_path[ 256 + 1 ] ;	/* 切换到指定目录运行。可选 */
	
	func_tcpmain	*pfunc_tcpmain ;	/* 当函数调用模式时，指向把TCP连接交给应用入口函数指针 */
	void		*param_tcpmain ;	/* 当函数调用模式时，指向把TCP连接交给应用入口函数的参数指针。特别注意：自己保证线程安全 */
	
	int		tcp_nodelay ;	/* 启用TCP_NODELAY选项 1:启用 0:不启用（缺省）。可选 */
	int		tcp_linger ;	/* 启用TCP_LINGER选项 1:启用并设置成参数值-1值 0:不启用（缺省）。可选 */
	
	/* 以下为内部使用 */
	int		install_winservice ;
	int		uninstall_winservice ;
} ;

/* 主入口函数 */
_WINDLL_FUNC int tcpdaemon( struct TcpdaemonEntryParameter *p_para );

/* WINDOWS服务名 */
#define TCPDAEMON_SERVICE		"TcpDaemon Service"

/* 环境结构成员 */
void *GetTcpmainParameter( struct TcpdaemonServerEnvirment *p_env );
int GetListenSocket( struct TcpdaemonServerEnvirment *p_env );
int *GetListenSocketPtr( struct TcpdaemonServerEnvirment *p_env );
struct sockaddr_in GetListenAddress( struct TcpdaemonServerEnvirment *p_env );
struct sockaddr_in *GetListenAddressPtr( struct TcpdaemonServerEnvirment *p_env );
int GetProcessCount( struct TcpdaemonServerEnvirment *p_env );
int *GetEpollArrayBase( struct TcpdaemonServerEnvirment *p_env );

#ifdef __cplusplus
}
#endif

#endif

