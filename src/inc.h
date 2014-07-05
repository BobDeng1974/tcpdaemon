#ifndef _H_INC_
#define _H_INC_

/*
 * tcpdaemon - TCP���ӹ����ػ�
 * author      : calvin
 * email       : calvinwilliams.c@gmail.com
 * LastVersion : 2014-06-29	v1.0.0		����
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define TCPDAEMON_CALLMODE_DAEMON		1 /* ����ģʽ:���ػ�ģʽ */

#include "tcpdaemon.h"

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
} pipe_t ;

struct TcpdaemonServerEnv
{
	void		*so_handle ; /* ��̬��򿪾�� */
	func_tcpmain	*pfunc_tcpmain ; /* ��̬�����:ͨѶ����Э�鼰Ӧ�ô���ص����� */
	void		*param_tcpmain ; /* ��ڲ��� */
	int		listen_sock ; /* �����׽��� */
	
	/* ��Instance-Forkģ��ʹ�� */
	long		process_count ;
	
	/* ��Leader-Followģ��ʹ�� */
	int		accept_mutex ; /* accept�ٽ��� */
	
	long		index ; /* ����������� */
	pipe_t		*alive_pipe ; /* �������̻�֪������̻��ܵ�������˵�ǹ������֪ͨ�������̽���������ܵ� */
					/* parent fd[1] -> child fd[0] */
	
	long		requests_per_process ; /* �������̵�ǰ�������� */
	
	pid_t		*pids ;
	pipe_t		*alive_pipes ;
} ;

int _main( int argc , char *argv[] , struct TcpdaemonEntryParam *pep , func_tcpmain *pfunc_tcpmain , void *param_tcpmain );

#ifdef __cplusplus
}
#endif

#endif

