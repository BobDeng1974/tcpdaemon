#ifndef _H_TCPDAEMON_
#define _H_TCPDAEMON_

/*
 * tcpdaemon - TCP���ӹ����ػ�
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

/* �汾���ַ��� */
extern char		*__TCPDAEMON_VERSION ;

/*
 * ���߲�
 */


/*
 * ͨѶ��
 */

/* ���ػ�ģʽ : tcpdaemon(main->tcpdaemon->tcpmain) + xxx.so(tcpmain) */

#define TCPMAIN		"tcpmain"

/* ͨѶ����Э�鼰Ӧ�ô���ص�����ԭ�� */
typedef int func_tcpmain( void *param_tcpmain , int sock , struct sockaddr *addr );

/* ��������ģʽ : xxx.exe(main->tcpmain) + tcpdaemon.so(tcpdaemon) */

#define TCPDAEMON_CALLMODE_MAIN		2 /* ����ģʽ2:��������ģʽ */

/* ����ڲ����ṹ */
struct TcpdaemonEntryParameter
{
	int		daemon_level ;	/* �Ƿ�ת��Ϊ�ػ����� 1:ת�� 0:��ת����ȱʡ�� */
	
	char		log_pathfilename[ MAXLEN_FILENAME + 1 ] ;	/* ��־����ļ��������������������׼����� */
	int		log_level ;	/* ��־�ȼ�����������ȱʡDEBUG�ȼ� */
	
	int		call_mode ;	/* Ӧ�ýӿ�ģʽ
					   TCPDAEMON_CALLMODE_DAEMON:���ػ�ģʽ
					   TCPDAEMON_CALLMODE_MAIN:��������ģʽ
					*/
	
	char		server_model[ 10 + 1 ] ;	/* TCP���ӹ���ģ��
							LF:�쵼��-׷����Ԥ�������̳�ģ�� for UNIX,Linux
							IF:��ʱ��������ģ�� for UNIX,Linux
							WIN-TLF:�쵼��-׷����Ԥ�����̳߳�ģ�� for win32
							*/
	long		max_process_count ;	/* ��Ϊ�쵼��-׷����Ԥ�������̳�ģ��ʱΪ�������̳ؽ�����������Ϊ��ʱ��������ģ��ʱΪ����ӽ������� */
	long		max_requests_per_process ;	/* ��Ϊ�쵼��-׷����Ԥ�������̳�ģ��ʱΪ�����������������Ӧ�ô��� */
	char		ip[ 20 + 1 ] ;	/* ��������IP */
	long		port ;	/* ��������PORT */
	char		so_pathfilename[ MAXLEN_FILENAME + 1 ] ;	/* �þ���·�������·������Ӧ�ö�̬���ļ��� */
	
	char		work_user[ 64 + 1 ] ;
	char		work_path[ MAXLEN_FILENAME + 1 ] ;
	
	func_tcpmain	*pfunc_tcpmain ;	/* ����������ģʽʱ��ָ���TCP���ӽ���Ӧ����ں���ָ�� */
	void		*param_tcpmain ;	/* ����������ģʽʱ��ָ���TCP���ӽ���Ӧ����ں����Ĳ���ָ�룬�ر�ע�⣺�Լ���֤�̰߳�ȫ */
	
	int		tcp_nodelay ;	/* ����TCP_NODELAYѡ�� 1:���� 0:�����ã�ȱʡ�� */
	int		tcp_linger ;	/* ����TCP_LINGERѡ�� 1:���ò����óɲ���ֵ-1ֵ 0:�����ã�ȱʡ�� */
	
	/* ����Ϊ�ڲ�ʹ�� */
	int		install_winservice ;
	int		uninstall_winservice ;
} ;

/* ����ں��� */
_WINDLL_FUNC int tcpdaemon( struct TcpdaemonEntryParameter *p_para );

/* WINDOWS������ */
#define TCPDAEMON_SERVICE		"TcpDaemon Service"

#ifdef __cplusplus
}
#endif

#endif

