#if ( defined __linux__ ) || ( defined __unix )
#define _GNU_SOURCE
#include <sched.h>
#endif

#include "tcpdaemon_in.h"

/*
 * tcpdaemon - TCP���ӹ����ػ�
 * author      : calvin
 * email       : calvinwilliams@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

/* �汾�ַ��� */
char		__TCPDAEMON_VERSION_1_5_0[] = "1.5.0" ;
char		*__TCPDAEMON_VERSION = __TCPDAEMON_VERSION_1_5_0 ;

static struct TcpdaemonServerEnvironment	*g_p_env = NULL ;
static int				g_EXIT_flag = 0 ;

#if ( defined _WIN32 )
static CRITICAL_SECTION		accept_critical_section; /* accept�ٽ��� */
#endif

#define MAX_INT(_a_,_b_)	((_a_)>(_b_)?(_a_):(_b_))

/* ��CPU��Ե�� */
static int BindCpuAffinity( int processor_no )
{
	cpu_set_t	cpu_mask ;
	
	int		nret = 0 ;
	
	CPU_ZERO( & cpu_mask );
	CPU_SET( processor_no , & cpu_mask );
	nret = sched_setaffinity( 0 , sizeof(cpu_mask) , & cpu_mask ) ;
	DEBUGLOG( "sched_setaffinity[%d] return[%d]" , processor_no , nret );
	return nret;
}

/* ��������в��� */
int CheckCommandParameter( struct TcpdaemonEntryParameter *p_para )
{
#if ( defined __linux__ ) || ( defined __unix )
	if( STRCMP( p_para->server_model , == , "IF" ) )
	{
		return 0;
	}
	if( STRCMP( p_para->server_model , == , "LF" ) )
	{
		if( p_para->process_count <= 0 )
		{
			ERRORLOG( "tcpdaemon_LF_worker poll size[%ld] invalid" , p_para->process_count );
			return -1;
		}
		
		return 0;
	}
	if( STRCMP( p_para->server_model , == , "IOMP" ) )
	{
		if( p_para->process_count <= 0 )
		{
			ERRORLOG( "tcpdaemon_IOMP_worker poll size[%ld] invalid" , p_para->process_count );
			return -1;
		}
		
		return 0;
	}
#elif ( defined _WIN32 )
	if( STRCMP( p_para->server_model , == , "WIN-TLF" ) )
	{
		if( p_para->process_count <= 0 )
		{
			ERRORLOG( "tcpdaemon_WIN-TLF_worker poll size[%ld] invalid" , p_para->process_count );
			return -1;
		}
		
		return 0;
	}
#endif
	
	ERRORLOG( "server mode[%s] invalid" , p_para->server_model );
	return -2;
}

#if ( defined __unix ) || ( defined __linux__ )
static unsigned int tcpdaemon_LF_worker( void *pv )
#elif ( defined _WIN32 )
static unsigned int WINAPI tcpdaemon_LF_worker( void *pv )
#endif
{
	struct TcpdaemonServerEnvironment	*p_env = (struct TcpdaemonServerEnvironment *)pv ;
#if ( defined __linux__ ) || ( defined __unix )
	struct sembuf		sb ;
#endif
	fd_set			readfds ;
	
	struct sockaddr		accepted_addr ;
	SOCKLEN_T		accepted_addrlen ;
	int			accepted_sock ;
	
	int			nret = 0 ;
	
	signal( SIGTERM , SIG_DFL );
	
	if( p_env->p_para->cpu_affinity > 0 )
		BindCpuAffinity( p_env->p_para->cpu_affinity+p_env->index );
	
	/* ������־���� */
	SetLogFile( p_env->p_para->log_pathfilename );
	SetLogLevel( p_env->p_para->log_level );
	
	DEBUGLOG( "tcpdaemon_LF_worker(%d) | begin" , p_env->index );
	
	while(1)
	{
		DEBUGLOG( "tcpdaemon_LF_worker(%d) | waiting for entering accept mutex" , p_env->index );
		
#if ( defined __linux__ ) || ( defined __unix )
		/* �����ٽ��� */
		memset( & sb , 0x00 , sizeof(struct sembuf) );
		sb.sem_num = 0 ;
		sb.sem_op = -1 ;
		sb.sem_flg = SEM_UNDO ;
		nret = semop( p_env->accept_mutex , & sb , 1 ) ;
		if( nret == -1 )
		{
			ERRORLOG( "tcpdaemon_LF_worker(%d) | enter accept mutex failed , errno[%d]" , p_env->index , ERRNO );
			break;
		}
#elif ( defined _WIN32 )
		EnterCriticalSection( & accept_critical_section );
#endif
		DEBUGLOG( "tcpdaemon_LF_worker(%d) | enter accept mutex ok" , p_env->index );
		
		/* �������socket����ܵ��¼� */
		FD_ZERO( & readfds );
		FD_SET( p_env->listen_sock , & readfds );
#if ( defined __linux__ ) || ( defined __unix )
		FD_SET( p_env->alive_pipes[p_env->index].fd[0] , & readfds );
		nret = select( MAX_INT(p_env->listen_sock,p_env->alive_pipes[p_env->index].fd[0])+1 , & readfds , NULL , NULL , NULL ) ;
#elif ( defined _WIN32 )
		nret = select( p_env->listen_sock+1 , & readfds , NULL , NULL , NULL ) ;
#endif
		if( nret == -1 )
		{	
			ERRORLOG( "tcpdaemon_LF_worker(%d) | select failed , errno[%d]" , p_env->index , ERRNO );
			break;
		}
		
#if ( defined __linux__ ) || ( defined __unix )
		if( FD_ISSET( p_env->alive_pipes[p_env->index].fd[0] , & readfds ) )
		{
			DEBUGLOG( "tcpdaemon_LF_worker(%d) | alive_pipe received quit command" , p_env->index );
			break;
		}
#endif
		
		/* �����¿ͻ������� */
		accepted_addrlen = sizeof(struct sockaddr) ;
		memset( & accepted_addr , 0x00 , accepted_addrlen );
		accepted_sock = accept( p_env->listen_sock , & accepted_addr , & accepted_addrlen ) ;
		if( accepted_sock == -1 )
		{
			ERRORLOG( "tcpdaemon_LF_worker(%d) | accept failed , errno[%d]" , p_env->index , ERRNO );
			break;
		}
		else
		{
			DEBUGLOG( "tcpdaemon_LF_worker(%d) | accept ok , [%d]accept[%d]" , p_env->index , p_env->listen_sock , accepted_sock );
		}
		
		/* ���������˿ڹر�nagle�㷨 */
		if( p_env->p_para->tcp_nodelay == 1 )
		{
			int	onoff = 1 ;
			setsockopt( accepted_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) );
		}
		
		/* ���������˿�lingerֵ */
		if( p_env->p_para->tcp_linger > 0 )
		{
			struct linger	lg ;
			lg.l_onoff = 1 ;
			lg.l_linger = p_env->p_para->tcp_linger ;
			setsockopt( accepted_sock , SOL_SOCKET , SO_LINGER , (void *) & lg , sizeof(struct linger) );
		}
		
#if ( defined __linux__ ) || ( defined __unix )
		/* �뿪�ٽ��� */
		memset( & sb , 0x00 , sizeof(struct sembuf) );
		sb.sem_num = 0 ;
		sb.sem_op = 1 ;
		sb.sem_flg = SEM_UNDO ;
		nret = semop( p_env->accept_mutex , & sb , 1 ) ;
		if( nret == -1 )
		{
			ERRORLOG( "tcpdaemon_LF_worker(%d) | leave accept mutex failed , errno[%d]" , p_env->index , ERRNO );
			break;
		}
#elif ( defined _WIN32 )
		LeaveCriticalSection( & accept_critical_section );
#endif
		DEBUGLOG( "tcpdaemon_LF_worker(%d) | leave accept mutex ok" , p_env->index );
		
		/* ����ͨѶ����Э�鼰Ӧ�ô���ص����� */
		INFOLOG( "tcpdaemon_LF_worker(%d) | call tcpmain sock[%d]" , p_env->index , accepted_sock );
		nret = p_env->pfunc_tcpmain( p_env , accepted_sock , & accepted_addr ) ;
		if( nret == TCPMAIN_RETURN_CLOSE )
		{
			INFOLOG( "tcpdaemon_LF_worker(%d) | tcpmain return[%d]" , p_env->index , nret );
		}
		else
		{
			ERRORLOG( "tcpdaemon_LF_worker(%d) | tcpmain return[%d]" , p_env->index , nret );
			break;
		}
		
		/* �رտͻ������� */
		CLOSESOCKET( accepted_sock );
		DEBUGLOG( "tcpdaemon_LF_worker(%d) | close[%d]" , p_env->index , accepted_sock );
		
		/* ��鹤�����̴������� */
		p_env->requests_per_process++;
		if( p_env->p_para->max_requests_per_process != 0 && p_env->requests_per_process >= p_env->p_para->max_requests_per_process )
		{
			INFOLOG( "tcpdaemon_LF_worker(%d) | maximum number of processing[%ld][%ld] , ending" , p_env->index , p_env->requests_per_process , p_env->p_para->max_requests_per_process );
			return 1;
		}
	}
	
#if ( defined __linux__ ) || ( defined __unix )
	/* �����뿪�ٽ��� */
	memset( & sb , 0x00 , sizeof(struct sembuf) );
	sb.sem_num = 0 ;
	sb.sem_op = 1 ;
	sb.sem_flg = SEM_UNDO ;
	nret = semop( p_env->accept_mutex , & sb , 1 ) ;
	if( nret == -1 )
	{
		INFOLOG( "tcpdaemon_LF_worker(%d) | leave accept mutex finally failed , errno[%d]" , p_env->index , ERRNO );
		return 1;
	}
#elif ( defined _WIN32 )
	LeaveCriticalSection( & accept_critical_section );
#endif
	DEBUGLOG( "tcpdaemon_LF_worker(%d) | leave accept mutex finally ok" , p_env->index );
	
	DEBUGLOG( "tcpdaemon_LF_worker(%d) | end" , p_env->index );
	
#if ( defined __linux__ ) || ( defined __unix )
	return 0;
#elif ( defined _WIN32 )
	free( p_env );
	_endthreadex(0);
	return 0;
#endif
}

/* Ԥ������пͻ��˻Ự�ṹ */
static int IncreaseTcpdaemonAcceptedSessions( struct TcpdaemonServerEnvironment *p_env )
{
	struct TcpdaemonAcceptedSessionArray	*p_accepted_session_array = NULL ;
	struct TcpdaemonAcceptedSession		*p_accepted_session = NULL ;
	int					i ;
	
	/* �������ӿ���HTTPͨѶ�Ự */
	p_accepted_session_array = (struct TcpdaemonAcceptedSessionArray *)malloc( sizeof(struct TcpdaemonAcceptedSessionArray) ) ;
	if( p_accepted_session_array == NULL )
	{
		ERRORLOG( "malloc failed , errno[%d]" , ERRNO );
		return -1;
	}
	memset( p_accepted_session_array , 0x00 , sizeof(struct TcpdaemonAcceptedSessionArray) );
	list_add_tail( & (p_accepted_session_array->prealloc_node) , & (p_env->accepted_session_array_list.prealloc_node) );
	
	for( i = 0 , p_accepted_session = p_accepted_session_array->accepted_session_array ; i < sizeof(p_accepted_session_array->accepted_session_array)/sizeof(p_accepted_session_array->accepted_session_array[0]) ; i++ , p_accepted_session++ )
	{
		list_add_tail( & (p_accepted_session->unused_node) , & (p_env->accepted_session_unused_list.unused_node) );
		DEBUGLOG( "init accepted session[%p]" , p_accepted_session );
	}
	
	return 0;
}

/* ��Ԥ����Ŀ��пͻ��˻Ự�ṹ��ȡ��һ�� */
static struct TcpdaemonAcceptedSession *FetchTcpdaemonAcceptedSessionUnused( struct TcpdaemonServerEnvironment *p_env )
{
	struct TcpdaemonAcceptedSession	*p_accepted_session = NULL ;
	
	int				nret = 0 ;
	
	/* ������пͻ��˻Ự����Ϊ�� */
	if( list_empty( & (p_env->accepted_session_unused_list.unused_node) ) )
	{
		nret = IncreaseTcpdaemonAcceptedSessions( p_env ) ;
		if( nret )
			return NULL;
	}
	
	/* �ӿ���HTTPͨѶ�Ự�������Ƴ�һ���Ự��������֮ */
	p_accepted_session = list_first_entry( & (p_env->accepted_session_unused_list.unused_node) , struct TcpdaemonAcceptedSession , unused_node ) ;
	list_del( & (p_accepted_session->unused_node) );
	
	DEBUGLOG( "fetch accepted session[%p]" , p_accepted_session );
	memset( p_accepted_session , 0x00 , sizeof(struct TcpdaemonAcceptedSession) );
	return p_accepted_session;
}

/* �ѵ�ǰ�ͻ��˻Ự�Żؿ���������ȥ */
static void SetTcpdaemonAcceptedSessionUnused( struct TcpdaemonServerEnvironment *p_env , struct TcpdaemonAcceptedSession *p_accepted_session )
{
	DEBUGLOG( "putback accepted session[%p]" , p_accepted_session );
	
	/* �ѵ�ǰ����HTTPͨѶ�Ự�Ƶ�����HTTPͨѶ�Ự������ */
	list_add_tail( & (p_accepted_session->unused_node) , & (p_env->accepted_session_unused_list.unused_node) );
	
	return;
}

/* �������пͻ�������Ự�ṹ */
static void FreeAllTcpdaemonAcceptedSessionArray( struct TcpdaemonServerEnvironment *p_env )
{
	struct list_head			*p_curr = NULL , *p_next = NULL ;
	struct TcpdaemonAcceptedSessionArray	*p_accepted_session_array = NULL ;
	
	list_for_each_safe( p_curr , p_next , & (p_env->accepted_session_array_list.prealloc_node) )
	{
		p_accepted_session_array = container_of( p_curr , struct TcpdaemonAcceptedSessionArray , prealloc_node ) ;
		list_del( & (p_accepted_session_array->prealloc_node) );
		
		free( p_accepted_session_array );
	}
	
	return;
}

/* �ر����� */
static void CloseTcpdaemonAcceptedSession( struct TcpdaemonServerEnvironment *p_env , struct TcpdaemonAcceptedSession *p_session )
{
	INFOLOG( "tcpdaemon_IOMP_worker(%d) | close sock[%d] io multiplex data ptr[%p]" , p_env->index , p_session->sock , p_session->io_multiplex_data_ptr );
	p_env->io_multiplex_event = IOMP_ON_CLOSING_SOCKET ;
	p_env->pfunc_tcpmain( p_env , 0 , p_session->io_multiplex_data_ptr );
	epoll_ctl( p_env->this_epoll_fd , EPOLL_CTL_DEL , p_session->sock , NULL );
	close( p_session->sock );
	UnlinkTcpdaemonAcceptedSessionBeginTimestampTreeNode( p_env , p_session );
	SetTcpdaemonAcceptedSessionUnused( p_env , p_session );
	
	return;
}

static unsigned int tcpdaemon_IOMP_worker( void *pv )
{
	struct TcpdaemonServerEnvironment	*p_env = (struct TcpdaemonServerEnvironment *)pv ;
	int				quit_flag ;
	int				now_timestamp ;
	struct epoll_event		event ;
	struct epoll_event		events[ MAX_IOMP_EVENTS ] ;
	int				epoll_nfds ;
	int				i ;
	struct epoll_event		*p_event = NULL ;
	struct TcpdaemonAcceptedSession	*p_session = NULL ;
	
	int				nret = 0 ;
	
	signal( SIGTERM , SIG_DFL );
	
	if( p_env->p_para->cpu_affinity > 0 )
		BindCpuAffinity( p_env->p_para->cpu_affinity+p_env->index );
	
	p_env->this_epoll_fd = p_env->epoll_array[p_env->index] ;
	
	/* ������־���� */
	SetLogFile( p_env->p_para->log_pathfilename );
	SetLogLevel( p_env->p_para->log_level );
	
	DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | begin" , p_env->index );
	
	/* ���������ɶ��¼���epoll */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = & (p_env->alive_pipes[p_env->index].fd[0]) ;
	nret = epoll_ctl( p_env->this_epoll_fd , EPOLL_CTL_ADD , p_env->alive_pipes[p_env->index].fd[0] , & event ) ;
	if( nret == -1 )
	{
		ERRORLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] add pipe_session[%d] failed , errno[%d]" , p_env->index , p_env->this_epoll_fd , p_env->alive_pipes[p_env->index].fd[0] , errno );
		return 1;
	}
	else
	{
		INFOLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] add pipe_session[%d] ok" , p_env->index , p_env->this_epoll_fd , p_env->alive_pipes[p_env->index].fd[0] );
	}
	
	/* �¼���ѭ�� */
	quit_flag = 0 ;
	while( ! quit_flag )
	{
		/* �ȴ�epoll�¼�������1�볬ʱ */
		DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | epoll_wait[%d] ..." , p_env->index , p_env->this_epoll_fd );
		memset( events , 0x00 , sizeof(events) );
		epoll_nfds = epoll_wait( p_env->this_epoll_fd , events , MAX_IOMP_EVENTS , 1000 ) ;
		if( epoll_nfds == -1 )
		{
			if( errno == EINTR )
			{
				INFOLOG( "tcpdaemon_IOMP_worker(%d) | epoll_wait[%d] interrupted" , p_env->index , p_env->this_epoll_fd );
				continue;
			}
			else
			{
				ERRORLOG( "tcpdaemon_IOMP_worker(%d) | epoll_wait[%d] failed , errno[%d]" , p_env->index , p_env->this_epoll_fd , ERRNO );
			}
			
			return 1;
		}
		else
		{
			DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | epoll_wait[%d] return[%d]events" , p_env->index , p_env->this_epoll_fd , epoll_nfds );
		}
		
		if( p_env->p_para->timeout_seconds > 0 )
		{
			now_timestamp = time(NULL) ;
			while(1)
			{
				p_session = GetTimeoutAcceptedSession( p_env , now_timestamp ) ;
				if( p_session == NULL )
					break;
				
				WARNLOG( "SOCKET TIMEOUT ------ sock[%d] session[%p]" , p_session->sock , p_session );
				/* ���ùر����ӻص����� */
				CloseTcpdaemonAcceptedSession( p_env , p_session );
			}
		}
		
		/* ���������¼� */
		for( i = 0 , p_event = events ; i < epoll_nfds ; i++ , p_event++ )
		{
			/* �����׽����¼� */
			if( p_event->data.ptr == & (p_env->listen_sock) )
			{
				/* �ɶ��¼� */
				if( p_event->events & EPOLLIN )
				{
					int			accepted_sock ;
					struct sockaddr		accepted_addr ;
					SOCKLEN_T		accepted_addr_len ;
					
					while(1)
					{
						/* ���������� */
						memset( & accepted_addr , 0x00 , sizeof(struct sockaddr_in) );
						accepted_addr_len = sizeof(struct sockaddr) ;
						accepted_sock = accept( p_env->listen_sock , (struct sockaddr *) & accepted_addr , & accepted_addr_len ) ;
						if( accepted_sock == -1 )
						{
							if( errno == EAGAIN )
								break;
							ERRORLOG( "tcpdaemon_IOMP_worker(%d) | accept failed , errno[%d]" , p_env->index , errno );
							return 1;
						}
						
						/* ���÷Ƕ��� */
						{
							fcntl( accepted_sock , F_SETFL , fcntl(accepted_sock,F_GETFL) | O_NONBLOCK );
						}
						
						/* ���������˿ڹر�nagle�㷨 */
						if( p_env->p_para->tcp_nodelay == 1 )
						{
							int	onoff = 1 ;
							setsockopt( accepted_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) );
						}
						
						/* ���������˿�lingerֵ */
						if( p_env->p_para->tcp_linger > 0 )
						{
							struct linger	lg ;
							lg.l_onoff = 1 ;
							lg.l_linger = p_env->p_para->tcp_linger ;
							setsockopt( accepted_sock , SOL_SOCKET , SO_LINGER , (void *) & lg , sizeof(struct linger) );
						}
						
						/* ���ý���ͨѶ���ӻص����� */
						INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnAcceptingSocket[%d]" , p_env->index , accepted_sock );
						p_env->io_multiplex_data_ptr = NULL ;
						p_env->io_multiplex_event = IOMP_ON_ACCEPTING_SOCKET ;
						nret = p_env->pfunc_tcpmain( p_env , accepted_sock , & accepted_addr ) ;
						if( p_env->io_multiplex_data_ptr == NULL )
						{
							FATALLOG( "tcpdaemon_IOMP_worker(%d) | need call TDSetIoMultiplexDataPtr" , p_env->index );
							close( accepted_sock );
							continue;
						}
						
						/* �ҽӳ�ʱ�Ự��������ָ���� */
						p_session = FetchTcpdaemonAcceptedSessionUnused( p_env ) ;
						if( p_session == NULL )
						{
							FATALLOG( "tcpdaemon_IOMP_worker(%d) | FetchTcpdaemonAcceptedSessionUnused failed , errno[%d]" , p_env->index , errno );
							close( accepted_sock );
							continue;
						}
						memset( p_session , 0x00 , sizeof(struct TcpdaemonAcceptedSession) );
						p_session->sock = accepted_sock ;
						p_session->io_multiplex_data_ptr = p_env->io_multiplex_data_ptr ;
						p_session->begin_timestamp = time(NULL) ;
						
						LinkTcpdaemonAcceptedSessionBeginTimestampTreeNode( p_env , p_session );
						
						/* �жϷ���ֵ */
						if( nret == TCPMAIN_RETURN_WAITINGFOR_RECEIVING )
						{
							INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnAcceptingSocket[%d] return[%d]" , p_env->index , accepted_sock , nret );
							
							memset( & event , 0x00 , sizeof(struct epoll_event) );
							event.events = EPOLLIN | EPOLLERR ;
							event.data.ptr = p_session ;
							nret = epoll_ctl( p_env->this_epoll_fd , EPOLL_CTL_ADD , accepted_sock , & event ) ;
							if( nret == -1 )
							{
								ERRORLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] add accepted sock[%d] waiting for reading failed , errno[%d]" , p_env->index , p_env->this_epoll_fd , accepted_sock , errno );
								return 1;
							}
							else
							{
								DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] add accepted sock[%d] waiting for reading" , p_env->index , p_env->this_epoll_fd , accepted_sock );
							}
						}
						else if( nret == TCPMAIN_RETURN_WAITINGFOR_SENDING )
						{
							INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnAcceptingSocket[%d] return[%d]" , p_env->index , accepted_sock , nret );
							
							memset( & event , 0x00 , sizeof(struct epoll_event) );
							event.events = EPOLLOUT | EPOLLERR ;
							event.data.ptr = p_session ;
							nret = epoll_ctl( p_env->this_epoll_fd , EPOLL_CTL_ADD , accepted_sock , & event ) ;
							if( nret == -1 )
							{
								ERRORLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] add accepted sock[%d] waiting for writing failed , errno[%d]" , p_env->index , p_env->this_epoll_fd , accepted_sock , errno );
								return 1;
							}
							else
							{
								DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] add accepted sock[%d] waiting for writing" , p_env->index , p_env->this_epoll_fd , accepted_sock );
							}
						}
						else if( nret == TCPMAIN_RETURN_CLOSE )
						{
							INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnAcceptingSocket[%d] return[%d]" , p_env->index , accepted_sock , nret );
							CloseTcpdaemonAcceptedSession( p_env , p_session );
							continue;
						}
						else
						{
							ERRORLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnAcceptingSocket[%d] return[%d]" , p_env->index , accepted_sock , nret );
							CloseTcpdaemonAcceptedSession( p_env , p_session );
							continue;
						}
					}
					
					if( p_env->p_para->process_count > 1 )
					{
						int		 j ;
						
						/* ת�������ɶ��¼�����һ��epoll */
						j = p_env->index + 1 ;
						if( j >= p_env->p_para->process_count )
							j = 0 ;
						
						memset( & event , 0x00 , sizeof(struct epoll_event) );
						event.events = EPOLLIN | EPOLLERR | EPOLLONESHOT;
						event.data.ptr = & (p_env->listen_sock) ;
						nret = epoll_ctl( p_env->epoll_array[j] , EPOLL_CTL_MOD , p_env->listen_sock , & event ) ;
						if( nret == -1 )
						{
							ERRORLOG( "tcpdaemon_IOMP_worker(%d) | [%d]epoll_ctl[%d] modify listen sock failed , errno[%d]" , p_env->index , j , p_env->epoll_array[j] , errno );
							return 1;
						}
						else
						{
							DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | [%d]epoll_ctl[%d] modify listen sock[%d] ok" , p_env->index , j , p_env->epoll_array[j] , p_env->listen_sock );
						}
					}
				}
				/* �����¼� */
				else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
				{
					FATALLOG( "tcpdaemon_IOMP_worker(%d) | listen session err or hup event[0x%X]" , p_env->index , p_event->events );
					return 1;
				}
				/* �����¼� */
				else
				{
					FATALLOG( "tcpdaemon_IOMP_worker(%d) | Unknow listen session event[0x%X]" , p_env->index , p_event->events );
					return 1;
				}
			}
			/* ����ܵ��¼� */
			else if( p_event->data.ptr == & (p_env->alive_pipes[p_env->index].fd[0]) )
			{
				quit_flag = 1 ;
			}
			/* �����¼������ͻ������ӻỰ�¼� */
			else
			{
				p_session = p_event->data.ptr ;
				
				/* �ɶ��¼� */
				if( p_event->events & EPOLLIN )
				{
					/* ���ý���ͨѶ���ݻص����� */
					INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnReceivingSocket[%d] ..." , p_env->index , p_session->sock );
					p_env->io_multiplex_event = IOMP_ON_RECEIVING_SOCKET ;
					nret = p_env->pfunc_tcpmain( p_env , 0 , p_session->io_multiplex_data_ptr ) ;
					if( nret == TCPMAIN_RETURN_WAITINGFOR_NEXT )
					{
						INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnReceivingSocket[%d] return[%d]" , p_env->index , p_session->sock , nret );
					}
					else if( nret == TCPMAIN_RETURN_WAITINGFOR_RECEIVING )
					{
						INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnReceivingSocket[%d] return[%d]" , p_env->index , p_session->sock , nret );
						
						memset( & event , 0x00 , sizeof(struct epoll_event) );
						event.events = EPOLLIN | EPOLLERR ;
						event.data.ptr = p_session ;
						nret = epoll_ctl( p_env->this_epoll_fd , EPOLL_CTL_MOD , p_session->sock , & event ) ;
						if( nret == -1 )
						{
							ERRORLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] modify accepted sock[%d] waiting for reading failed , errno[%d]" , p_env->index , p_env->this_epoll_fd , p_session->sock , errno );
							return 1;
						}
						else
						{
							DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] modify accepted sock[%d] waiting for reading" , p_env->index , p_env->this_epoll_fd , p_session->sock );
						}
					}
					else if( nret == TCPMAIN_RETURN_WAITINGFOR_SENDING )
					{
						INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnReceivingSocket[%d] return[%d]" , p_env->index , p_session->sock , nret );
						
						memset( & event , 0x00 , sizeof(struct epoll_event) );
						event.events = EPOLLOUT | EPOLLERR ;
						event.data.ptr = p_session ;
						nret = epoll_ctl( p_env->this_epoll_fd , EPOLL_CTL_MOD , p_session->sock , & event ) ;
						if( nret == -1 )
						{
							ERRORLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] modify accepted sock[%d] waiting for writing failed , errno[%d]" , p_env->index , p_env->this_epoll_fd , p_session->sock , errno );
							return 1;
						}
						else
						{
							DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] modify accepted sock[%d] waiting for writing" , p_env->index , p_env->this_epoll_fd , p_session->sock );
						}
					}
					else if( nret == TCPMAIN_RETURN_CLOSE )
					{
						INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnReceivingSocket[%d] return[%d]" , p_env->index , p_session->sock , nret );
						
						/* ���ùر����ӻص����� */
						CloseTcpdaemonAcceptedSession( p_env , p_session );
					}
					else
					{
						ERRORLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnReceivingSocket[%d] return[%d]" , p_env->index , p_session->sock , nret );
						
						/* ���ùر����ӻص����� */
						CloseTcpdaemonAcceptedSession( p_env , p_session );
					}
				}
				/* ��д�¼� */
				else if( p_event->events & EPOLLOUT )
				{
					/* ���÷���ͨѶ���ݻص����� */
					INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnSendingSocket[%d] ..." , p_env->index , p_session->sock );
					p_env->io_multiplex_event = IOMP_ON_SENDING_SOCKET ;
					nret = p_env->pfunc_tcpmain( p_env , 0 , p_session->io_multiplex_data_ptr ) ;
					if( nret == TCPMAIN_RETURN_WAITINGFOR_NEXT )
					{
						INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnSendingSocket[%d] return[%d]" , p_env->index , p_session->sock , nret );
					}
					else if( nret == TCPMAIN_RETURN_WAITINGFOR_RECEIVING )
					{
						INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnSendingSocket[%d] return[%d]" , p_env->index , p_session->sock , nret );
						
						memset( & event , 0x00 , sizeof(struct epoll_event) );
						event.events = EPOLLIN | EPOLLERR ;
						event.data.ptr = p_session ;
						nret = epoll_ctl( p_env->this_epoll_fd , EPOLL_CTL_MOD , p_session->sock , & event ) ;
						if( nret == -1 )
						{
							ERRORLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] modify accepted sock[%d] waiting for reading failed , errno[%d]" , p_env->index , p_env->this_epoll_fd , p_session->sock , errno );
							return 1;
						}
						else
						{
							DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] modify accepted sock[%d] waiting for reading" , p_env->index , p_env->this_epoll_fd , p_session->sock );
						}
					}
					else if( nret == TCPMAIN_RETURN_WAITINGFOR_SENDING )
					{
						INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnSendingSocket[%d] return[%d]" , p_env->index , p_session->sock , nret );
						
						memset( & event , 0x00 , sizeof(struct epoll_event) );
						event.events = EPOLLOUT | EPOLLERR ;
						event.data.ptr = p_session ;
						nret = epoll_ctl( p_env->this_epoll_fd , EPOLL_CTL_MOD , p_session->sock , & event ) ;
						if( nret == -1 )
						{
							ERRORLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] modify accepted sock[%d] waiting for writing failed , errno[%d]" , p_env->index , p_env->this_epoll_fd , p_session->sock , errno );
							return 1;
						}
						else
						{
							DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | epoll_ctl[%d] modify accepted sock[%d] waiting for writing" , p_env->index , p_env->this_epoll_fd , p_session->sock );
						}
					}
					else if( nret == TCPMAIN_RETURN_CLOSE )
					{
						INFOLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnSendingSocket[%d] return[%d]" , p_env->index , p_session->sock , nret );
						
						/* ���ùر����ӻص����� */
						CloseTcpdaemonAcceptedSession( p_env , p_session );
					}
					else
					{
						ERRORLOG( "tcpdaemon_IOMP_worker(%d) | call tcpmain on OnSendingSocket[%d] return[%d]" , p_env->index , p_session->sock , nret );
						
						/* ���ùر����ӻص����� */
						CloseTcpdaemonAcceptedSession( p_env , p_session );
					}
				}
				/* �����¼� */
				else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
				{
					FATALLOG( "tcpdaemon_IOMP_worker(%d) | accepted session err or hup event[0x%X]" , p_env->index , p_event->events );
					/* ���ùر����ӻص����� */
					CloseTcpdaemonAcceptedSession( p_env , p_event->data.ptr );
				}
				/* �����¼� */
				else
				{
					FATALLOG( "tcpdaemon_IOMP_worker(%d) | Unknow accepted session event[0x%X]" , p_env->index , p_event->events );
					return 1;
				}
			}
		}
	}
	
	DEBUGLOG( "tcpdaemon_IOMP_worker(%d) | end" , p_env->index );
	
	return 0;
}

struct TcpdaemonServerEnvironment *DuplicateServerEnv( struct TcpdaemonServerEnvironment *p_env )
{
	struct TcpdaemonServerEnvironment	*pse_new = NULL ;

	pse_new = (struct TcpdaemonServerEnvironment *)malloc( sizeof(struct TcpdaemonServerEnvironment) );
	if( pse_new == NULL )
		return NULL;
	
	memcpy( pse_new , p_env , sizeof(struct TcpdaemonServerEnvironment) );
	
	return pse_new;
}
#if ( defined __linux__ ) || ( defined __unix )

/* TERM�źŻص����� */
void sigproc_SIGTERM( int signo )
{
	g_EXIT_flag = 1 ;
	return;
}

/* CHLD�źŻص����� */
void sigproc_SIGCHLD( int signo )
{
	PID_T		pid ;
	int		status ;
	
	do
	{
		pid = waitpid( -1 , & status , WNOHANG ) ;
		if( pid > 0 )
		{
			g_p_env->process_count--;
		}
	}
	while( pid > 0 );
	
	return;
}

#endif

/* ��ʼ���ػ����� */
static int InitDaemonEnv( struct TcpdaemonServerEnvironment *p_env )
{
	int		nret = 0 ;
	
	p_env->listen_sock = -1 ;
	
	/* �õ�ͨѶ����Э�鼰Ӧ�ô���ص�����ָ�� */
	if( p_env->p_para->so_pathfilename[0] )
	{
#if ( defined __linux__ ) || ( defined __unix )
		p_env->so_handle = dlopen( p_env->p_para->so_pathfilename , RTLD_NOW ) ;
#elif ( defined _WIN32 )
		p_env->so_handle = LoadLibrary( p_env->p_para->so_pathfilename ) ;
#endif
		if( p_env->so_handle == NULL )
		{
			ERRORLOG( "dlopen[%s]failed[%s]" , p_env->p_para->so_pathfilename , DLERROR );
			return -1;
		}
		else
		{
			INFOLOG( "dlopen[%s]ok" , p_env->p_para->so_pathfilename );
		}
		
#if ( defined __linux__ ) || ( defined __unix )
		p_env->pfunc_tcpmain = (func_tcpmain*)dlsym( p_env->so_handle , TCPMAIN ) ;
#elif ( defined _WIN32 )
		p_env->pfunc_tcpmain = (func_tcpmain*)GetProcAddress( p_env->so_handle , TCPMAIN ) ;
#endif
		if( p_env->pfunc_tcpmain == NULL )
		{
			ERRORLOG( "dlsym[%s]failed[%s]" , TCPMAIN , DLERROR );
			return -1;
		}
		else
		{
			INFOLOG( "dlsym[%s]ok" , TCPMAIN );
		}
	}
	else
	{
		p_env->pfunc_tcpmain = p_env->p_para->pfunc_tcpmain ;
	}
	
	p_env->param_tcpmain = p_env->p_para->param_tcpmain ;
	
	/* ��������socket */
	p_env->listen_sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( p_env->listen_sock == -1 )
	{
		ERRORLOG( "socket failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	
	/* ���������˿ڵ�ַ���� */
	{
		int	on = 1 ;
		setsockopt( p_env->listen_sock , SOL_SOCKET , SO_REUSEADDR , (void *) & on , sizeof(on) );
	}
	
	/* ������socket */
	{
	memset( & (p_env->listen_addr) , 0x00 , sizeof(struct sockaddr_in) );
	p_env->listen_addr.sin_family = AF_INET ;
	p_env->listen_addr.sin_addr.s_addr = inet_addr( p_env->p_para->ip ) ;
	p_env->listen_addr.sin_port = htons( (unsigned short)p_env->p_para->port ) ;
	nret = bind( p_env->listen_sock , (struct sockaddr *) & (p_env->listen_addr), sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		ERRORLOG( "bind[%s:%ld]failed , ERRNO[%d]" , p_env->p_para->ip , p_env->p_para->port , ERRNO );
		return -1;
	}
	}
	
	/* ��ʼ���� */
	nret = listen( p_env->listen_sock , p_env->p_para->process_count * 2 ) ;
	if( nret == -1 )
	{
		ERRORLOG( "listen failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	
	/* �л��û� */
#if ( defined __linux__ ) || ( defined __unix )
	if( STRCMP( p_env->p_para->work_user , != , "" ) )
	{
	struct passwd	*pw ;
	pw = getpwnam( p_env->p_para->work_user ) ;
	if( pw )
	{
		setuid( pw->pw_uid );
		setgid( pw->pw_gid );
	}
	}
#endif
	
	/* �л�����Ŀ¼ */
	if( STRCMP( p_env->p_para->work_path , != , "" ) )
	{
		CHDIR( p_env->p_para->work_path );
	}
	
	return 0;
}

/* �����ػ����� */
static int CleanDaemonEnv( struct TcpdaemonServerEnvironment *p_env )
{
	if( p_env->so_handle )
	{
#if ( defined __linux__ ) || ( defined __unix )
		dlclose( p_env->so_handle );
#elif ( defined _WIN32 )
		FreeLibrary( p_env->so_handle );
#endif
	}
	
	if( p_env->listen_sock != -1 )
	{
		CLOSE( p_env->listen_sock );
	}
	
	return 0;
}

#if ( defined __linux__ ) || ( defined __unix )

/* Instance-Fork����ģ�� ��ʼ���ػ����� */
static int InitDaemonEnv_IF( struct TcpdaemonServerEnvironment *p_env )
{
	return InitDaemonEnv( p_env );
}

/* Instance-Fork����ģ�� �����ػ����� */
static int CleanDaemonEnv_IF( struct TcpdaemonServerEnvironment *p_env )
{
	return CleanDaemonEnv( p_env );
}

/* Instance-Fork����ģ�� ��ں��� */
int tcpdaemon_IF( struct TcpdaemonServerEnvironment *p_env )
{
	struct sigaction	act , oldact ;
	
	fd_set			readfds ;
	struct timeval		tv ;
	
	struct sockaddr		accepted_addr ;
	socklen_t		accepted_addrlen ;
	int			accepted_sock ;
	
	PID_T			pid ;
	int			status ;
	
	int			nret = 0 ;
	
	/* ��ʼ���ػ����� */
	nret = InitDaemonEnv_IF( p_env ) ;
	if( nret )
	{
		ERRORLOG( "init IF failed[%d]" , nret );
		CleanDaemonEnv_IF( p_env );
		return nret;
	}
	
	/* �����źŵ� */
	memset( & act , 0x00 , sizeof(struct sigaction) );
	act.sa_handler = sigproc_SIGTERM ;
	sigemptyset( & (act.sa_mask) );
#ifdef SA_INTERRUPT
	act.sa_flags = SA_INTERRUPT ;
#endif
	nret = sigaction( SIGTERM , & act , & oldact );
	
	memset( & act , 0x00 , sizeof(struct sigaction) );
	act.sa_handler = & sigproc_SIGCHLD ;
	sigemptyset( & (act.sa_mask) );
	act.sa_flags = SA_RESTART ;
	nret = sigaction( SIGCHLD , & act , NULL );
	
	INFOLOG( "parent listen starting" );
	
	/* ������������ʼ */
	while(1)
	{
		/* �������socket�¼� */
_DO_SELECT :
		FD_ZERO( & readfds );
		FD_SET( p_env->listen_sock , & readfds );
		tv.tv_sec = 1 ;
		tv.tv_usec = 0 ;
		nret = select( p_env->listen_sock+1 , & readfds , NULL , NULL , & tv ) ;
		if( nret == -1 )
		{
			if( ERRNO == EINTR )
			{
				if( g_EXIT_flag == 1 )
				{
					break;
				}
				else
				{
					goto _DO_SELECT;
				}
			}
			else
			{
				ERRORLOG( "select failed , ERRNO[%d]" , ERRNO );
				break;
			}
		}
		else if( nret == 0 )
		{
			do
			{
				pid = waitpid( -1 , & status , WNOHANG ) ;
				if( pid > 0 )
				{
					g_p_env->process_count--;
				}
			}
			while( pid > 0 );
			
			continue;
		}
		
		/* �����¿ͻ������� */
_DO_ACCEPT :
		accepted_addrlen = sizeof(struct sockaddr) ;
		memset( & accepted_addr , 0x00 , accepted_addrlen );
		accepted_sock = accept( p_env->listen_sock , & accepted_addr , & accepted_addrlen ) ;
		if( accepted_sock == -1 )
		{
			if( ERRNO == EINTR )
			{
				if( g_EXIT_flag == 1 )
				{
					break;
				}
				else
				{
					goto _DO_ACCEPT;
				}
			}
			else
			{
				ERRORLOG( "accept failed , ERRNO[%d]" , ERRNO );
				break;
			}
		}
		else
		{
			
			DEBUGLOG( "accept ok , [%d]accept[%d]" , p_env->listen_sock , accepted_sock );
		}
		
		if( p_env->p_para->process_count != 0 && p_env->process_count + 1 > p_env->p_para->process_count )
		{
			ERRORLOG( "too many sockets" );
			CLOSE( accepted_sock );
			continue;
		}
		
		/* ���������˿ڹر�nagle�㷨 */
		if( p_env->p_para->tcp_nodelay == 1 )
		{
			int	onoff = 1 ;
			setsockopt( accepted_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) );
		}
		
		/* ���������˿�lingerֵ */
		if( p_env->p_para->tcp_linger > 0 )
		{
			struct linger	lg ;
			lg.l_onoff = 1 ;
			lg.l_linger = p_env->p_para->tcp_linger ;
			setsockopt( accepted_sock , SOL_SOCKET , SO_LINGER , (void *) & lg , sizeof(struct linger) );
		}
		
		/* �����ӽ��� */
_DO_FORK :
		pid = fork() ;
		if( pid == -1 )
		{
			if( ERRNO == EINTR )
			{
				if( g_EXIT_flag == 1 )
				{
					break;
				}
				else
				{
					goto _DO_FORK;
				}
			}
			else
			{
				ERRORLOG( "fork failed , ERRNO[%d]" , ERRNO );
				CLOSE( accepted_sock );
				continue;
			}
		}
		else if( pid == 0 )
		{
			signal( SIGTERM , SIG_DFL );
			signal( SIGCHLD , SIG_DFL );
			
			CLOSESOCKET( p_env->listen_sock );
			
			/* ����ͨѶ����Э�鼰Ӧ�ô���ص����� */
			INFOLOG( "call tcpmain sock[%d]" , accepted_sock );
			nret = p_env->pfunc_tcpmain( p_env , accepted_sock , & accepted_addr ) ;
			if( nret < 0 )
			{
				ERRORLOG( "tcpmain return[%d]" , nret );
			}
			else if( nret > 0 )
			{
				WARNLOG( "tcpmain return[%d]" , nret );
			}
			else
			{
				INFOLOG( "tcpmain return[%d]" , nret );
			}
			
			/* �رտͻ������� */
			CLOSESOCKET( accepted_sock );
			DEBUGLOG( "close[%d]" , accepted_sock );
			
			/* �ӽ����˳� */
			DEBUGLOG( "child exit" );
			exit(0);
		}
		else
		{
			CLOSESOCKET( accepted_sock );
			p_env->process_count++;
		}
	}
	
	sigaction( SIGTERM , & oldact , NULL );
	
	INFOLOG( "parent listen ended" );
	
	INFOLOG( "waiting for all children exit starting" );
	
	while( p_env->process_count > 0 )
	{
		waitpid( -1 , & status , 0 );
	}
	
	INFOLOG( "waiting for all chdilren exit ended" );
	
	/* �����ػ����� */
	CleanDaemonEnv_IF( p_env );
	
	return 0;
}

/* Leader-Follower���̳�ģ�� ��ʼ���ػ����� */
static int InitDaemonEnv_LF( struct TcpdaemonServerEnvironment *p_env )
{
	int		nret = 0 ;
	
	/* ������ʼ������ */
	nret = InitDaemonEnv( p_env ) ;
	if( nret )
		return nret;
	
	/* ����pid������Ϣ���� */
	p_env->pids = (pid_t*)malloc( sizeof(pid_t) * (p_env->p_para->process_count) ) ;
	if( p_env->pids == NULL )
	{
		ERRORLOG( "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->pids , 0x00 , sizeof(pid_t) * (p_env->p_para->process_count) );
	
	/* �������ܵ���Ϣ���� */
	p_env->alive_pipes = (PIPE_T*)malloc( sizeof(PIPE_T) * (p_env->p_para->process_count) ) ;
	if( p_env->alive_pipes == NULL )
	{
		ERRORLOG( "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->alive_pipes , 0x00 , sizeof(PIPE_T) * (p_env->p_para->process_count) );
	
	/* ����accept�ٽ��� */
	p_env->accept_mutex = semget( IPC_PRIVATE , 1 , IPC_CREAT | 00777 ) ;
	if( p_env->accept_mutex == -1 )
	{
		ERRORLOG( "create mutex failed" );
		return -1;
	}
	
	{
	union semun	semopts ;
	semopts.val = 1 ;
	nret = semctl( p_env->accept_mutex , 0 , SETVAL , semopts ) ;
	if( nret == -1 )
	{
		ERRORLOG( "set mutex failed" );
		return -1;
	}
	}
	
	return 0;
}

/* Leader-Follower���̳�ģ�� �����ػ����� */
static int CleanDaemonEnv_LF( struct TcpdaemonServerEnvironment *p_env )
{
	/* �ͷ��ڴ� */
	if( p_env->accept_mutex )
	{
		semctl( p_env->accept_mutex , 0 , IPC_RMID , 0 );
	}
	
	if( p_env->pids )
	{
		free( p_env->pids );
	}
	
	if( p_env->alive_pipes )
	{
		free( p_env->alive_pipes );
	}
	
	return CleanDaemonEnv( p_env );
}

/* Leader-Follow���̳�ģ�� ��ں��� */
int tcpdaemon_LF( struct TcpdaemonServerEnvironment *p_env )
{
	PID_T			pid ;
	int			status ;
	
	struct sigaction	act , oldact ;
	
	int			nret = 0 ;
	
	/* ��ʼ���ػ����� */
	nret = InitDaemonEnv_LF( p_env ) ;
	if( nret )
	{
		ERRORLOG( "init LF failed[%d]" , nret );
		CleanDaemonEnv_LF( p_env );
		return nret;
	}
	
	/* �����źŵ� */
	memset( & act , 0x00 , sizeof(struct sigaction) );
	act.sa_handler = & sigproc_SIGTERM ;
#ifdef SA_INTERRUPT
	act.sa_flags = SA_INTERRUPT ;
#endif
	sigaction( SIGTERM , & act , & oldact );
	
	signal( SIGCHLD , SIG_DFL );
	
	/* �����������̳� */
	INFOLOG( "create tcpdaemon_LF_worker pool starting" );
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		nret = pipe( p_env->alive_pipes[p_env->index].fd ) ;
		if( nret == -1 )
		{
			ERRORLOG( "pipe failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_LF( p_env );
			return -11;
		}
		
		p_env->pids[p_env->index] = fork() ;
		if( p_env->pids[p_env->index] == -1 )
		{
			ERRORLOG( "fork failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_LF( p_env );
			return -11;
		}
		else if( p_env->pids[p_env->index] == 0 )
		{
			CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
			tcpdaemon_LF_worker( p_env );
			CLOSE( p_env->alive_pipes[p_env->index].fd[0] );
			
			CLOSESOCKET( p_env->listen_sock );
			
			exit(0);
		}
		else
		{
			CLOSE( p_env->alive_pipes[p_env->index].fd[0] );
		}
		
		SLEEP(1);
	}
	
	INFOLOG( "create tcpdaemon_LF_worker pool ok" );
	
	/* ��ع������̳� */
	INFOLOG( "monitoring all children starting" );
	
	while(1)
	{
		/* ��ع������̽����¼� */
		pid = waitpid( -1 , & status , 0 ) ;
		if( pid == -1 )
		{
			break;
		}
		
		for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
		{
			if( p_env->pids[p_env->index] == pid )
			{
				/* ������������ */
				CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
				
				INFOLOG( "detecting child[%ld]pid[%ld] exit , rebooting" , p_env->index , (long)pid );
				
				SLEEP(1);
				
				p_env->requests_per_process = 0 ;
				
				nret = pipe( p_env->alive_pipes[p_env->index].fd ) ;
				if( nret )
				{
					ERRORLOG( "pipe failed , ERRNO[%d]" , ERRNO );
					CleanDaemonEnv_LF( p_env );
					return -11;
				}
				
				p_env->pids[p_env->index] = fork() ;
				if( p_env->pids[p_env->index] == -1 )
				{
					ERRORLOG( "fork failed , ERRNO[%d]" , ERRNO );
					CleanDaemonEnv_LF( p_env );
					return -11;
				}
				else if( p_env->pids[p_env->index] == 0 )
				{
					CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
					tcpdaemon_LF_worker( p_env );
					CLOSE( p_env->alive_pipes[p_env->index].fd[0] );
					
					CLOSESOCKET( p_env->listen_sock );
					
					exit(0);
				}
				else
				{
					CLOSE( p_env->alive_pipes[p_env->index].fd[0] );
				}
				
				break;
			}
		}
		if( p_env->index > p_env->p_para->process_count )
		{
			INFOLOG( "detecting unknow child pid[%ld] exit" , (long)pid );
			continue;
		}
	}
	
	INFOLOG( "monitoring all children ok" );
	
	sigaction( SIGTERM , & oldact , NULL );
	
	/* ���ٽ��̳� */
	INFOLOG( "destroy tcpdaemon_LF_worker poll starting" );
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		write( p_env->alive_pipes[p_env->index].fd[1] , "\0" , 1 );
	}
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		waitpid( -1 , & status , 0 );
		CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
	}
	
	INFOLOG( "destroy tcpdaemon_LF_worker poll ended" );
	
	/* �����ػ����� */
	CleanDaemonEnv_LF( p_env );
	
	return 0;
}

/* IOMultiplex���̳�ģ�� ��ʼ���ػ����� */
static int InitDaemonEnv_IOMP( struct TcpdaemonServerEnvironment *p_env )
{
	struct epoll_event	event ;
	int			i ;
	
	int			nret = 0 ;
	
	/* �������пͻ��˻Ự����ṹ���� */
	memset( & (p_env->accepted_session_array_list) , 0x00 , sizeof(struct TcpdaemonAcceptedSessionArray) );
	INIT_LIST_HEAD( & (p_env->accepted_session_array_list.prealloc_node) );
	
	/* �������пͻ��˻Ự�ṹ���� */
	memset( & (p_env->accepted_session_unused_list) , 0x00 , sizeof(struct TcpdaemonAcceptedSession) );
	INIT_LIST_HEAD( & (p_env->accepted_session_unused_list.unused_node) );
	
	/* ������ʼ������ */
	nret = InitDaemonEnv( p_env ) ;
	if( nret )
		return nret;
	
	/* ���������˿ڷǶ��� */
	{
		fcntl( p_env->listen_sock , F_SETFL , fcntl(p_env->listen_sock,F_GETFL) | O_NONBLOCK );
	}
	
	/* ���������˿ڹر�nagle�㷨 */
	if( p_env->p_para->tcp_nodelay == 1 )
	{
		int	onoff = 1 ;
		setsockopt( p_env->listen_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) );
	}
	
	/* ����pid������Ϣ���� */
	p_env->pids = (pid_t*)malloc( sizeof(pid_t) * (p_env->p_para->process_count) ) ;
	if( p_env->pids == NULL )
	{
		ERRORLOG( "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->pids , 0x00 , sizeof(pid_t) * (p_env->p_para->process_count) );
	
	/* �������ܵ���Ϣ���� */
	p_env->alive_pipes = (PIPE_T*)malloc( sizeof(PIPE_T) * (p_env->p_para->process_count) ) ;
	if( p_env->alive_pipes == NULL )
	{
		ERRORLOG( "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->alive_pipes , 0x00 , sizeof(PIPE_T) * (p_env->p_para->process_count) );
	
	/* ����epoll���� */
	p_env->epoll_array = (int*)malloc( sizeof(int) * (p_env->p_para->process_count) ) ;
	if( p_env->epoll_array == NULL )
	{
		ERRORLOG( "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->epoll_array , 0x00 , sizeof(int) * (p_env->p_para->process_count) );
	
	for( i = 0 ; i < p_env->p_para->process_count ; i++ )
	{
		p_env->epoll_array[i] = epoll_create( 1024 ) ;
		if( p_env->epoll_array[i] == -1 )
		{
			ERRORLOG( "epoll_create failed , errno[%d]" , errno );
			return -1;
		}
		else
		{
			INFOLOG( "epoll_create ok" );
		}
	}
	
	/* ���������ɶ��¼���epoll */
	if( p_env->p_para->process_count > 1 )
	{
		for( i = 0 ; i < p_env->p_para->process_count ; i++ )
		{
			memset( & event , 0x00 , sizeof(struct epoll_event) );
			if( i == 0 )
				event.events = EPOLLIN | EPOLLERR | EPOLLONESHOT ;
			else
				event.events = EPOLLERR ;
			event.data.ptr = & (p_env->listen_sock) ;
			nret = epoll_ctl( p_env->epoll_array[i] , EPOLL_CTL_ADD , p_env->listen_sock , & event ) ;
			if( nret == -1 )
			{
				ERRORLOG( "[%d]epoll_ctl[%d] add listen_session failed , errno[%d]" , i , p_env->epoll_array[i] , errno );
				return -1;
			}
			else
			{
				INFOLOG( "[%d]epoll_ctl[%d] add listen_session[%d] ok" , i , p_env->epoll_array[i] , p_env->listen_sock );
			}
		}
	}
	else
	{
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.events = EPOLLIN | EPOLLERR ;
		event.data.ptr = & (p_env->listen_sock) ;
		nret = epoll_ctl( p_env->epoll_array[0] , EPOLL_CTL_ADD , p_env->listen_sock , & event ) ;
		if( nret == -1 )
		{
			ERRORLOG( "epoll_ctl[%d] add listen_session failed , errno[%d]" , p_env->epoll_array[0] , errno );
			return -1;
		}
		else
		{
			INFOLOG( "epoll_ctl[%d] add listen_session[%d] ok" , p_env->epoll_array[0] , p_env->listen_sock );
		}
	}
	
	/* Ԥ���������ӻỰ */
	nret = IncreaseTcpdaemonAcceptedSessions( p_env ) ;
	if( nret )
	{
		ERRORLOG( "IncreaseTcpdaemonAcceptedSessions failed[%d]" , nret );
		return -1;
	}
	
	return 0;
}

/* IOMultiplex���̳�ģ�� �����ػ����� */
static int CleanDaemonEnv_IOMP( struct TcpdaemonServerEnvironment *p_env )
{
	int		i ;
	
	/* �ͷ��ڴ� */
	if( p_env->pids )
	{
		free( p_env->pids );
	}
	
	if( p_env->alive_pipes )
	{
		free( p_env->alive_pipes );
	}
	
	if( p_env->epoll_array )
	{
		for( i = 0 ; i < p_env->p_para->process_count ; i++ )
		{
			close( p_env->epoll_array[i] );
		}
		
		free( p_env->epoll_array );
	}
	
	FreeAllTcpdaemonAcceptedSessionArray( p_env );
	
	return CleanDaemonEnv( p_env );
}

/* IOMultiplex���̳�ģ�� ��ں��� */
int tcpdaemon_IOMP( struct TcpdaemonServerEnvironment *p_env )
{
	PID_T			pid ;
	int			status ;
	
	struct sigaction	act , oldact ;
	
	int			nret = 0 ;
	
	/* ��ʼ���ػ����� */
	nret = InitDaemonEnv_IOMP( p_env ) ;
	if( nret )
	{
		ERRORLOG( "init IOMP failed[%d]" , nret );
		CleanDaemonEnv_IOMP( p_env );
		return nret;
	}
	
	/* �����źŵ� */
	memset( & act , 0x00 , sizeof(struct sigaction) );
	act.sa_handler = & sigproc_SIGTERM ;
#ifdef SA_INTERRUPT
	act.sa_flags = SA_INTERRUPT ;
#endif
	sigaction( SIGTERM , & act , & oldact );
	
	signal( SIGCHLD , SIG_DFL );
	
	/* �����������̳� */
	INFOLOG( "create tcpdaemon_IOMP_worker pool starting" );
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		nret = pipe( p_env->alive_pipes[p_env->index].fd ) ;
		if( nret == -1 )
		{
			ERRORLOG( "pipe failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_IOMP( p_env );
			return -11;
		}
		
		p_env->pids[p_env->index] = fork() ;
		if( p_env->pids[p_env->index] == -1 )
		{
			ERRORLOG( "fork failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_IOMP( p_env );
			return -11;
		}
		else if( p_env->pids[p_env->index] == 0 )
		{
			CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
			tcpdaemon_IOMP_worker( p_env );
			CLOSE( p_env->alive_pipes[p_env->index].fd[0] );
			
			CLOSESOCKET( p_env->listen_sock );
			
			exit(0);
		}
		else
		{
			CLOSE( p_env->alive_pipes[p_env->index].fd[0] );
		}
		
		SLEEP(1);
	}
	
	INFOLOG( "create tcpdaemon_IOMP_worker pool ok" );
	
	/* ��ع������̳� */
	INFOLOG( "monitoring all children starting" );
	
	while(1)
	{
		/* ��ع������̽����¼� */
		pid = waitpid( -1 , & status , 0 ) ;
		if( pid == -1 )
		{
			break;
		}
		
		for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
		{
			if( p_env->pids[p_env->index] == pid )
			{
				/* ������������ */
				CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
				
				INFOLOG( "detecting child[%ld]pid[%ld] exit , rebooting" , p_env->index , (long)pid );
				
				SLEEP(1);
				
				p_env->requests_per_process = 0 ;
				
				nret = pipe( p_env->alive_pipes[p_env->index].fd ) ;
				if( nret )
				{
					ERRORLOG( "pipe failed , ERRNO[%d]" , ERRNO );
					CleanDaemonEnv_IOMP( p_env );
					return -11;
				}
				
				p_env->pids[p_env->index] = fork() ;
				if( p_env->pids[p_env->index] == -1 )
				{
					ERRORLOG( "fork failed , ERRNO[%d]" , ERRNO );
					CleanDaemonEnv_IOMP( p_env );
					return -11;
				}
				else if( p_env->pids[p_env->index] == 0 )
				{
					CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
					tcpdaemon_IOMP_worker( p_env );
					CLOSE( p_env->alive_pipes[p_env->index].fd[0] );
					
					CLOSESOCKET( p_env->listen_sock );
					
					exit(0);
				}
				else
				{
					CLOSE( p_env->alive_pipes[p_env->index].fd[0] );
				}
				
				break;
			}
		}
		if( p_env->index > p_env->p_para->process_count )
		{
			INFOLOG( "detecting unknow child pid[%ld] exit" , (long)pid );
			continue;
		}
	}
	
	INFOLOG( "monitoring all children ok" );
	
	sigaction( SIGTERM , & oldact , NULL );
	
	/* ���ٽ��̳� */
	INFOLOG( "destroy tcpdaemon_IOMP_worker poll starting" );
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		write( p_env->alive_pipes[p_env->index].fd[1] , "\0" , 1 );
	}
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		waitpid( -1 , & status , 0 );
		CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
	}
	
	INFOLOG( "destroy tcpdaemon_IOMP_worker poll ok" );
	
	/* �����ػ����� */
	CleanDaemonEnv_IOMP( p_env );
	
	return 0;
}

#endif

#if ( defined _WIN32 )

/* Leader-Follower�̳߳�ģ�� ��ʼ���ػ����� */
static int InitDaemonEnv_WIN_TLF( struct TcpdaemonServerEnvironment *p_env )
{
	int		nret = 0 ;
	
	/* ������ʼ������ */
	nret = InitDaemonEnv( p_env ) ;
	if( nret )
		return nret;
	
	/* ��ʼ���ٽ��� */
	InitializeCriticalSection( & accept_critical_section ); 
	
	/* ����tid������Ϣ���� */
	p_env->thandles = (THANDLE_T*)malloc( sizeof(THANDLE_T) * (p_env->p_para->process_count+1) ) ;
	if( p_env->thandles == NULL )
	{
		ERRORLOG( "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->thandles , 0x00 , sizeof(THANDLE_T) * (p_env->p_para->process_count+1) );
	
	p_env->tids = (TID_T*)malloc( sizeof(TID_T) * (p_env->p_para->process_count+1) ) ;
	if( p_env->tids == NULL )
	{
		ERRORLOG( "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->tids , 0x00 , sizeof(TID_T) * (p_env->p_para->process_count+1) );
	
	return 0;
}

/* Leader-Follower�̳߳�ģ�� �����ػ����� */
static int CleanDaemonEnv_WIN_TLF( struct TcpdaemonServerEnvironment *p_env )
{
	/* ��ʼ���ٽ��� */
	DeleteCriticalSection( & accept_critical_section );
	
	/* �ͷ��ڴ� */
	if( p_env->thandles )
	{
		free( p_env->thandles );
	}

	if( p_env->tids )
	{
		free( p_env->tids );
	}
	
	return CleanDaemonEnv( p_env );
}

/* Leader-Follow�̳߳�ģ��for win32 ��ں��� */
int tcpdaemon_WIN_TLF( struct TcpdaemonServerEnvironment *p_env )
{
	struct TcpdaemonServerEnvironment	*pse_new = NULL ;
	unsigned long			index ;
	
	int				nret = 0 ;
	long				lret = 0 ;
	
	/* ��ʼ���ػ����� */
	nret = InitDaemonEnv_WIN_TLF( p_env ) ;
	if( nret )
	{
		ERRORLOG( "init WIN-TLF failed[%d]" , nret );
		CleanDaemonEnv_WIN_TLF( p_env );
		return nret;
	}
	
	/* �����������̳� */
	INFOLOG( "create tcpdaemon_LF_worker pool starting" );
	
	for( p_env->index = 1 ; p_env->index <= p_env->p_para->process_count ; p_env->index++ )
	{
		pse_new = DuplicateServerEnv( p_env ) ;
		if( pse_new == NULL )
		{
			ERRORLOG( "DuplicateServerEnv failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_WIN_TLF( p_env );
			return -11;
		}
		
		p_env->thandles[p_env->index] = (THANDLE_T)_beginthreadex( NULL , 0 , tcpdaemon_LF_worker , pse_new , 0 , & (p_env->tids[p_env->index]) ) ;
		if( p_env->thandles[p_env->index] == NULL )
		{
			ERRORLOG( "_beginthreadex failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_WIN_TLF( p_env );
			return -12;
		}
		
		SLEEP(1);
	}
	
	INFOLOG( "create tcpdaemon_LF_worker pool ended" );
	
	/* ��ع����̳߳� */
	INFOLOG( "monitoring all children starting" );
	
	while(1)
	{
		/* ��ع����߳̽����¼� */
		index = WaitForMultipleObjects( p_env->p_para->process_count , p_env->thandles , FALSE , INFINITE ) ;
		if( index == WAIT_FAILED )
		{
			ERRORLOG( "WaitForMultipleObjects failed , errno[%d]" , ERRNO );
			break;
		}
		index = index - WAIT_OBJECT_0 + 1 ;
		CloseHandle( p_env->thandles[index-1] );
		
		/* �����߳̽��� */
		INFOLOG( "detecting child[%ld]tid[%ld] exit , rebooting" , index , p_env->tids[index-1] );
		
		Sleep(1000);
		
		p_env->requests_per_process = 0 ;
		
		pse_new = DuplicateServerEnv( p_env ) ;
		if( pse_new == NULL )
		{
			ERRORLOG( "DuplicateServerEnv failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_WIN_TLF( p_env );
			return -11;
		}
		
		p_env->thandles[index-1] = (THANDLE_T)_beginthreadex( NULL , 0 , tcpdaemon_LF_worker , pse_new , 0 , & (p_env->tids[index-1]) ) ;
		if( p_env->thandles[index-1] == NULL )
		{
			ERRORLOG( "_beginthreadex failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_WIN_TLF( p_env );
			return -12;
		}
	}
	
	INFOLOG( "monitoring all children ended" );
	
	/* �����̳߳� */
	INFOLOG( "destroy tcpdaemon_LF_worker poll starting" );
	
	for( p_env->index = 1 ; p_env->index <= p_env->p_para->process_count ; p_env->index++ )
	{
		lret = WaitForMultipleObjects( p_env->p_para->process_count , p_env->thandles , TRUE , INFINITE ) ;
		if( lret == WAIT_FAILED )
		{
			break;
		}	
	}
	
	INFOLOG( "destroy tcpdaemon_LF_worker poll ended" );
	
	/* �����ػ����� */
	CleanDaemonEnv_WIN_TLF( p_env );
	
	return 0;
}

#endif

#if ( defined __linux__ ) || ( defined __unix )

/* ת��Ϊ�ػ����� */
static int BindDaemonServer( func_tcpdaemon *tcpdaemon , struct TcpdaemonEntryParameter	*p_para )
{
	pid_t		pid ;
	
	pid = fork() ;
	if( pid == -1 )
	{
		return -1;
	}
	else if( pid > 0 )
	{
		exit(0);
	}
	
	setsid();
	signal( SIGHUP,SIG_IGN );
	
	pid = fork() ;
	if( pid == -1 )
	{
		return -2;
	}
	else if( pid > 0 )
	{
		exit(0);
	}
	
	setuid( getpid() ) ;
	
	umask( 0 ) ;
	
	return tcpdaemon( p_para );
}

#elif ( defined _WIN32 )

STARTUPINFO		g_si ;
PROCESS_INFORMATION	g_li ;
	
static int (* _iRTF_g_pfControlMain)(long lControlStatus) ;
SERVICE_STATUS		_gssServiceStatus ;
SERVICE_STATUS_HANDLE	_gsshServiceStatusHandle ;
HANDLE			_ghServerHandle ;
void			*_gfServerParameter ;
char			_gacServerName[257] ;
int			(* _gfServerMain)( void *pv ) ;

static void WINAPI ServiceCtrlHandler( DWORD dwControl )
{
	switch ( dwControl )
	{
		case SERVICE_CONTROL_STOP :
		case SERVICE_CONTROL_SHUTDOWN :
			
			_gssServiceStatus.dwCurrentState = SERVICE_STOP_PENDING ;
			SetServiceStatus( _gsshServiceStatusHandle , &_gssServiceStatus ) ;
			_gssServiceStatus.dwCurrentState = SERVICE_STOPPED ;
			SetServiceStatus( _gsshServiceStatusHandle , &_gssServiceStatus) ;
			break;
			
		case SERVICE_CONTROL_PAUSE :
			
			_gssServiceStatus.dwCurrentState = SERVICE_PAUSE_PENDING ;
			SetServiceStatus( _gsshServiceStatusHandle , &_gssServiceStatus ) ;
			_gssServiceStatus.dwCurrentState = SERVICE_PAUSED ;
			SetServiceStatus( _gsshServiceStatusHandle , &_gssServiceStatus) ;
			break;
		
		case SERVICE_CONTROL_CONTINUE :
			
			_gssServiceStatus.dwCurrentState = SERVICE_CONTINUE_PENDING ;
			SetServiceStatus( _gsshServiceStatusHandle , &_gssServiceStatus ) ;
			_gssServiceStatus.dwCurrentState = SERVICE_RUNNING ;
			SetServiceStatus( _gsshServiceStatusHandle , &_gssServiceStatus) ;
			break;
			
		case SERVICE_CONTROL_INTERROGATE :
			
			_gssServiceStatus.dwCurrentState = SERVICE_RUNNING ;
			SetServiceStatus( _gsshServiceStatusHandle , &_gssServiceStatus) ;
			break;
			
		default:
			
			break;
			
	}
	
	if( _iRTF_g_pfControlMain )
		(* _iRTF_g_pfControlMain)( dwControl );
	
	return;
}

static void WINAPI ServiceMainProc( DWORD argc , LPTSTR *argv )
{
	_gssServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS ;
	_gssServiceStatus.dwCurrentState = SERVICE_START_PENDING ;
	_gssServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP ;
	_gssServiceStatus.dwWin32ExitCode = 0 ;
	_gssServiceStatus.dwServiceSpecificExitCode = 0 ;
	_gssServiceStatus.dwCheckPoint = 0 ;
	_gssServiceStatus.dwWaitHint = 0 ;
	
	_gsshServiceStatusHandle = RegisterServiceCtrlHandler( _gacServerName , ServiceCtrlHandler ) ;
	if( _gsshServiceStatusHandle == (SERVICE_STATUS_HANDLE)0 )
		return;
	
	_gssServiceStatus.dwCheckPoint = 0 ;
	_gssServiceStatus.dwWaitHint = 0 ;
	_gssServiceStatus.dwCurrentState = SERVICE_RUNNING ;
	
	SetServiceStatus( _gsshServiceStatusHandle , &_gssServiceStatus );
	
	_gfServerMain( _gfServerParameter );
	
	return;
}

static int BindDaemonServer( char *pcServerName , int (* ServerMain)( void *pv ) , void *pv , int (* pfuncControlMain)(long lControlStatus) )
{
	SERVICE_TABLE_ENTRY ste[] =
	{
		{ _gacServerName , ServiceMainProc },
		{ NULL , NULL }
	} ;
	
	BOOL		bret ;
	
	memset( _gacServerName , 0x00 , sizeof( _gacServerName ) );
	if( pcServerName )
	{
		strncpy( _gacServerName , pcServerName , sizeof( _gacServerName ) - 1 );
	}
	
	_ghServerHandle = GetCurrentProcess() ;
	_gfServerMain = ServerMain ;
	_gfServerParameter = pv ;
	_iRTF_g_pfControlMain = pfuncControlMain ;
	
	bret = StartServiceCtrlDispatcher( ste ) ;
	if( bret != TRUE )
		return -1;
	
	return 0;
}

static int monitor( void *command_line )
{
	BOOL			bret = TRUE ;
	
	/* �����ӽ��̣������������¼�������֮ */
	while(1)
	{
		memset( & g_si , 0x00 , sizeof(STARTUPINFO) );
		g_si.dwFlags = STARTF_USESHOWWINDOW ;
		g_si.wShowWindow = SW_HIDE ;
		memset( & g_li , 0x00 , sizeof(LPPROCESS_INFORMATION) );
		bret = CreateProcess( NULL , (char*)command_line , NULL , NULL , TRUE , 0 , NULL , NULL , & g_si , & g_li ) ;
		if( bret != TRUE )
		{
			fprintf( stderr , "CreateProcess failed , errno[%d]\n" , GetLastError() );
			return 1;
		}
		
		CloseHandle( g_li.hThread );
		
		WaitForSingleObject( g_li.hProcess , INFINITE );
		CloseHandle( g_li.hProcess );
		
		Sleep(10*1000);
	}
	
	return 0;
}

static int control( long control_status )
{
	/* ���յ�WINDOWS��������¼����Ƚ����ӽ��� */
	if( control_status == SERVICE_CONTROL_STOP || control_status == SERVICE_CONTROL_SHUTDOWN )
	{
		TerminateProcess( g_li.hProcess , 0 );
	}
	
	return 0;
}

#endif

/* ����ں��� */
static int _tcpdaemon( struct TcpdaemonEntryParameter *p_para )
{
	struct TcpdaemonServerEnvironment	env ;
	
	int				nret = 0 ;
	
	memset( & env , 0x00 , sizeof(struct TcpdaemonServerEnvironment) );
	env.p_para = p_para ;
	g_p_env = & env ;
	
	/* ������־���� */
	SetLogFile( p_para->log_pathfilename );
	SetLogLevel( p_para->log_level );
	INFOLOG( "tcpdaemon startup - v%s" , __TCPDAEMON_VERSION );
	
	/* �����ڲ��� */
	nret = CheckCommandParameter( p_para ) ;
	if( nret )
		return nret;
	
#if ( defined __linux__ ) || ( defined __unix )
	if( STRCMP( p_para->server_model , == , "IF" ) )
	{
		/* ����Instance-Fork����ģ�� ��ں��� */
		nret = tcpdaemon_IF( & env ) ;
	}
	if( STRCMP( p_para->server_model , == , "LF" ) )
	{
		/* ����Leader-Follow���̳�ģ�� ��ں��� */
		nret = tcpdaemon_LF( & env ) ;
	}
	if( STRCMP( p_para->server_model , == , "IOMP" ) )
	{
		/* ����IOMultiplex���̳�ģ�� ��ں��� */
		nret = tcpdaemon_IOMP( & env ) ;
	}
#elif ( defined _WIN32 )
	if( STRCMP( p_para->server_model , == , "WIN-TLF" ) )
	{
		/* ����Leader-Follow�̳߳�ģ�� ��ں��� */
		nret = tcpdaemon_WIN_TLF( & env ) ;
	}
#endif
	
	INFOLOG( "tcpdaemon ended - v%s" , __TCPDAEMON_VERSION );
	
	return nret;
}

func_tcpdaemon tcpdaemon ;
int tcpdaemon( struct TcpdaemonEntryParameter *p_para )
{
	int		nret = 0 ;
	
	if( p_para->daemon_level == 1 )
	{
#if ( defined __unix ) || ( defined __linux__ )
		/* ת��Ϊ�ػ����� */
		nret = BindDaemonServer( & _tcpdaemon , p_para ) ;
		if( nret )
		{
			ERRORLOG( "convert to daemon failed[%d]" , nret );
			return nret;
		}
		else
		{
			return 0;
		}
#elif ( defined _WIN32 )
		long		c , len ;
		char		command_line[ 256 + 1 ] ;
		
		/* �����̣�ת��Ϊ���� */
		memset( command_line , 0x00 , sizeof(command_line) );
		len = SNPRINTF( command_line , sizeof(command_line) , "%s" , "tcpdaemon.exe" ) ;
		for( c = 1 ; c < argc ; c++ )
		{
			if( STRCMP( argv[c] , != , "--install-winservice" ) && STRCMP( argv[c] , != , "--uninstall-winservice" ) && STRCMP( argv[c] , != , "--daemon-level" ) )
			{
				if( strchr( argv[c] , ' ' ) || strchr( argv[c] , '\t' ) )
					len += SNPRINTF( command_line + len , sizeof(command_line)-1 - len , " \"%s\"" , argv[c] ) ;
				else
					len += SNPRINTF( command_line + len , sizeof(command_line)-1 - len , " %s" , argv[c] ) ;
			}
		}
		
		nret = BindDaemonServer( "TcpDaemon" , monitor , command_line , & control ) ;
		if( nret )
		{
			ERRORLOG( "convert to daemon failed[%d]" , nret );
			return nret;
		}
		else
		{
			return 0;
		}
#endif
	}
	else
	{
#if ( defined __unix ) || ( defined __linux__ )
		/* ����tcpdaemon���� */
		return _tcpdaemon( p_para );
#elif ( defined _WIN32 )
		/* �ӽ��̣��ɻ� */
		{
		WSADATA		wsd;
		if( WSAStartup( MAKEWORD(2,2) , & wsd ) != 0 )
			return 1;
		}
		
		/* ����tcpdaemon���� */
		nret = _tcpdaemon( & para ) ;
		WSACleanup();
		retutn nret;
#endif
	}
}

void *TDGetTcpmainParameter( struct TcpdaemonServerEnvironment *p_env )
{
	return p_env->param_tcpmain;
}

int TDGetListenSocket( struct TcpdaemonServerEnvironment *p_env )
{
	return p_env->listen_sock;
}

int *TDGetListenSocketPtr( struct TcpdaemonServerEnvironment *p_env )
{
	return &(p_env->listen_sock);
}

struct sockaddr_in TDGetListenAddress( struct TcpdaemonServerEnvironment *p_env )
{
	return p_env->listen_addr;
}

struct sockaddr_in *TDGetListenAddressPtr( struct TcpdaemonServerEnvironment *p_env )
{
	return &(p_env->listen_addr);
}

int TDGetProcessCount( struct TcpdaemonServerEnvironment *p_env )
{
	return p_env->p_para->process_count;
}

int *TDGetEpollArrayBase( struct TcpdaemonServerEnvironment *p_env )
{
	return p_env->epoll_array;
}

int TDGetThisEpoll( struct TcpdaemonServerEnvironment *p_env )
{
	return p_env->epoll_array[p_env->index];
}

int TDGetIoMultiplexEvent( struct TcpdaemonServerEnvironment *p_env )
{
	return p_env->io_multiplex_event;
}

void TDSetIoMultiplexDataPtr( struct TcpdaemonServerEnvironment *p_env , void *io_multiplex_data_ptr )
{
	p_env->io_multiplex_data_ptr = io_multiplex_data_ptr ;
	return;
}

#if ( defined __linux__ ) || ( defined __unix )

/* �̶�ͨѶͷ+ͨѶ��Э�� ͬ��������ʽ�������� */
int TDHBSendData( int sock , int head_len , char *body_buffer , int *p_body_len , struct timeval *timeout )
{
	struct TDHBContext	context ;
	struct iovec		iovs[ 2 ] ;
	fd_set			write_fds ;
	struct timeval		begin_timestamp ;
	struct timeval		end_timestamp ;
	int			len ;
	
	int			nret = 0 ;
	
	memset( & context , 0x00 , sizeof(struct TDHBContext) );
	
	if( head_len > sizeof(context.head_buffer)-1 )
		return TDHB_ERROR_HEAD_TOO_LONG;
	
	context.body_len = (*p_body_len) ;
	sprintf( context.head_buffer , "%0*d" , head_len , context.body_len );
	
	context.head_remain_len = head_len ;
	context.body_remain_len = context.body_len ;
	while( context.head_remain_len || context.body_remain_len )
	{
		gettimeofday( & begin_timestamp , NULL );
		
		FD_ZERO( & write_fds );
		FD_SET( sock , & write_fds );
		nret = select( sock+1 , NULL , & write_fds , NULL , timeout ) ;
		if( nret == 0 )
		{
			(*p_body_len) = head_len + context.body_len - context.head_remain_len - context.body_remain_len ;
			return TDHB_ERROR_SEND_TIMEOUT;
		}
		else if( nret == -1 )
		{
			(*p_body_len) = head_len + context.body_len - context.head_remain_len - context.body_remain_len ;
			return TDHB_ERROR_SELECT_SEND_FAILED;
		}
		
		if( context.head_remain_len )
		{
			iovs[0].iov_base = context.head_buffer+(head_len-context.head_remain_len) ;
			iovs[0].iov_len = context.head_remain_len ;
			iovs[1].iov_base = body_buffer ;
			iovs[1].iov_len = context.body_remain_len ;
			len = writev( sock , iovs , 2 ) ;
			if( len == -1 )
			{
				(*p_body_len) = head_len + context.body_len - context.head_remain_len - context.body_remain_len ;
				return TDHB_ERROR_SEND_FAILED;
			}
			if( len >= context.head_remain_len )
			{
				context.head_remain_len = 0 ;
				context.body_remain_len -= ( len - head_len ) ;
			}
			else
			{
				context.head_remain_len -= len ;
			}
		}
		else
		{
			len = write( sock , body_buffer+(context.body_len-context.body_remain_len) , context.body_remain_len ) ;
			if( len == -1 )
			{
				(*p_body_len) = head_len + context.body_len - context.head_remain_len - context.body_remain_len ;
				return TDHB_ERROR_SEND_FAILED;
			}
			context.body_remain_len -= len ;
		}
		
		gettimeofday( & end_timestamp , NULL );
		timeout->tv_sec -= ( end_timestamp.tv_sec - begin_timestamp.tv_sec ) ;
		timeout->tv_usec -= ( end_timestamp.tv_usec - begin_timestamp.tv_usec ) ;
		while( timeout->tv_usec < 0 )
		{
			timeout->tv_usec += 1000000 ;
			timeout->tv_sec--;
		}
	}
	
	return 0;
}

/* �̶�ͨѶͷ+ͨѶ��Э�� ͬ��������ʽ�������� */
int TDHBReceiveData( int sock , int head_len , char *body_buffer , int *p_body_bufsize , struct timeval *timeout )
{
	struct TDHBContext	context ;
	fd_set			read_fds ;
	struct timeval		begin_timestamp ;
	struct timeval		end_timestamp ;
	int			len ;
	
	int			nret = 0 ;
	
	memset( & context , 0x00 , sizeof(struct TDHBContext) );
	
	context.head_remain_len = head_len ;
	context.body_remain_len = 0 ;
	while( context.head_remain_len || context.body_remain_len )
	{
		gettimeofday( & begin_timestamp , NULL );
		
		FD_ZERO( & read_fds );
		FD_SET( sock , & read_fds );
		nret = select( sock+1 , & read_fds , NULL , NULL , timeout ) ;
		if( nret == 0 )
		{
			if( context.head_remain_len )
				(*p_body_bufsize) = 0 ;
			else
				(*p_body_bufsize) = context.body_len - context.body_remain_len ;
			return TDHB_ERROR_RECV_TIMEOUT;
		}
		else if( nret == -1 )
		{
			if( context.head_remain_len )
				(*p_body_bufsize) = 0 ;
			else
				(*p_body_bufsize) = context.body_len - context.body_remain_len ;
			return TDHB_ERROR_SELECT_RECV_FAILED;
		}
		
		if( context.head_remain_len )
		{
			len = read( sock , context.head_buffer+(head_len-context.head_remain_len) , context.head_remain_len ) ;
			if( len == -1 )
			{
				(*p_body_bufsize) = 0 ;
				return TDHB_ERROR_RECV_FAILED;
			}
			else if( len == 0 )
			{
				(*p_body_bufsize) = 0 ;
				return TDHB_ERROR_SOCKET_CLOSED;
			}
			
			if( len == context.head_remain_len )
			{
				context.head_remain_len = 0 ;
				context.head_buffer[head_len] = '\0' ;
				context.body_len = atoi(context.head_buffer) ;
				if( context.body_len > (*p_body_bufsize) )
				{
					(*p_body_bufsize) = 0 ;
					return TDHB_ERROR_RECV_BUFFER_OVERFLOW;
				}
				context.body_remain_len = context.body_len - ( len-head_len ) ;
			}
			else
			{
				context.head_remain_len -= len ;
			}
		}
		else
		{
			len = read( sock , body_buffer+(context.body_len-context.body_remain_len) , context.body_remain_len ) ;
			if( len == -1 )
			{
				(*p_body_bufsize) = context.body_len - context.body_remain_len ;
				return TDHB_ERROR_RECV_FAILED;
			}
			context.body_remain_len -= len ;
		}
		
		gettimeofday( & end_timestamp , NULL );
		timeout->tv_sec -= ( end_timestamp.tv_sec - begin_timestamp.tv_sec ) ;
		timeout->tv_usec -= ( end_timestamp.tv_usec - begin_timestamp.tv_usec ) ;
		while( timeout->tv_usec < 0 )
		{
			timeout->tv_usec += 1000000 ;
			timeout->tv_sec--;
		}
	}
	
	(*p_body_bufsize) = context.body_len ;
	
	return 0;
}

/* �̶�ͨѶͷ+ͨѶ��Э�� ���ӷ����� */
int TDHBConnect( char *ip , int port )
{
	int			sock ;
	struct sockaddr_in	addr ;
	
	int			nret = 0 ;
	
	/* ���ӷ���� */
	sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( sock == -1 )
	{
		return TDHB_ERROR_SOCKET_FAILED;
	}
	
	{
		int	onoff = 1 ;
		setsockopt( sock , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) );
	}
	
	memset( & addr , 0x00 , sizeof(struct sockaddr_in) );
	addr.sin_family = AF_INET ;
	if( ip[0] == '\0' )
		addr.sin_addr.s_addr = INADDR_ANY ;
	else
		addr.sin_addr.s_addr = inet_addr(ip) ;
	addr.sin_port = htons( (unsigned short)(port) );
	nret = connect( sock , (struct sockaddr *) & addr , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		close( sock );
		return TDHB_ERROR_CONNECT_FAILED;
	}
	
	return sock;
}

/* �̶�ͨѶͷ+ͨѶ��Э�� ���ͽ������� */
int TDHBSendAndReceiveData( int sock , int head_len , char *body_buffer , int *p_body_len , int *p_body_bufsize , struct timeval *timeout )
{
	int		nret = 0 ;
	
	nret = TDHBSendData( sock , head_len , body_buffer , p_body_len , timeout ) ;
	if( nret )
		return nret;
	
	nret = TDHBReceiveData( sock , head_len , body_buffer , p_body_bufsize , timeout ) ;
	if( nret )
		return nret;
	
	return 0;
}

/* �̶�ͨѶͷ+ͨѶ��Э�� �Ͽ������� */
void TDHBDisconnect( int sock )
{
	/* �Ͽ������ */
	close( sock );
	
	return;
}

/* �̶�ͨѶͷ+ͨѶ��Э�� ���ӷ����������ͽ������ݣ��Ͽ������� */
int TDHBCall( char *ip , int port , int head_len , char *body_buffer , int *p_body_len , int *p_body_bufsize , struct timeval *timeout )
{
	int		sock ;
	
	int		nret = 0 ;
	
	/* ���ӷ����� */
	sock = TDHBConnect( ip , port ) ;
	if( sock < 0 )
		return sock;
	
	/* ���ͽ������� */
	nret = TDHBSendAndReceiveData( sock , head_len , body_buffer , p_body_len , p_body_bufsize , timeout ) ;
	if( nret )
	{
		close( sock );
		return nret;
	}
	
	/* �Ͽ������� */
	TDHBDisconnect( sock );
	
	return 0;
}

/* �̶�ͨѶͷ+ͨѶ��Э�� �Ƕ�����ʽ�������� */
int TDHBSendDataWithNonblock( int sock , int head_len , char *body_buffer , int *p_body_len , struct TDHBContext *p_context )
{
	struct iovec		iovs[ 2 ] ;
	int			len ;
	
	if( head_len > sizeof(p_context->head_buffer)-1 )
		return TDHB_ERROR_HEAD_TOO_LONG;
	
	if( p_context->head_remain_len == 0 && p_context->body_remain_len == 0 )
	{
		p_context->body_len = (*p_body_len) ;
		sprintf( p_context->head_buffer , "%0*d" , head_len , p_context->body_len );
		p_context->head_remain_len = head_len ;
		p_context->body_remain_len = p_context->body_len ;
	}
	
	while(1)
	{
		if( p_context->head_remain_len )
		{
			iovs[0].iov_base = p_context->head_buffer+(head_len-p_context->head_remain_len) ;
			iovs[0].iov_len = p_context->head_remain_len ;
			iovs[1].iov_base = body_buffer ;
			iovs[1].iov_len = p_context->body_remain_len ;
			len = writev( sock , iovs , 2 ) ;
			if( len == -1 )
			{
				if( errno == EAGAIN )
					return TCPMAIN_RETURN_WAITINGFOR_NEXT;
				(*p_body_len) = head_len + p_context->body_len - p_context->head_remain_len - p_context->body_remain_len ;
				return TDHB_ERROR_SEND_FAILED;
			}
			if( len >= p_context->head_remain_len )
			{
				DEBUGHEXLOG( p_context->head_buffer , head_len , "SEND HEAD [%d]BYTES" , head_len );
				
				p_context->head_remain_len = 0 ;
				p_context->body_remain_len -= ( len - head_len ) ;
				if( p_context->body_remain_len == 0 )
				{
					DEBUGHEXLOG( body_buffer , (*p_body_len) , "SEND BODY [%d]BYTES" , (*p_body_len) );
					return TCPMAIN_RETURN_WAITINGFOR_RECEIVING;
				}
			}
			else
			{
				p_context->head_remain_len -= len ;
				if( p_context->body_remain_len == 0 )
					return TCPMAIN_RETURN_WAITINGFOR_RECEIVING;
			}
		}
		else
		{
			len = write( sock , body_buffer+(p_context->body_len-p_context->body_remain_len) , p_context->body_remain_len ) ;
			if( len == -1 )
			{
				if( errno == EAGAIN )
					return TCPMAIN_RETURN_WAITINGFOR_NEXT;
				(*p_body_len) = head_len + p_context->body_len - p_context->head_remain_len - p_context->body_remain_len ;
				return TDHB_ERROR_SEND_FAILED;
			}
			p_context->body_remain_len -= len ;
			if( p_context->body_remain_len == 0 )
			{
				DEBUGHEXLOG( body_buffer , (*p_body_len) , "SEND BODY [%d]BYTES" , (*p_body_len) );
				return TCPMAIN_RETURN_WAITINGFOR_RECEIVING;
			}
		}
	}
}

/* �̶�ͨѶͷ+ͨѶ��Э�� �Ƕ�����ʽ�������� */
int TDHBReceiveDataWithNonblock( int sock , int head_len , char *body_buffer , int *p_body_bufsize , struct TDHBContext *p_context )
{
	int			len ;
	
	if( p_context->head_remain_len == 0 && p_context->body_remain_len == 0 )
	{
		p_context->body_len = 0 ;
		memset( p_context->head_buffer , 0x00 , sizeof(p_context->head_buffer) );
		p_context->head_remain_len = head_len ;
		p_context->body_remain_len = 0 ;
	}
	
	while(1)
	{
		if( p_context->head_remain_len )
		{
			len = read( sock , p_context->head_buffer+(head_len-p_context->head_remain_len) , p_context->head_remain_len ) ;
			if( len == -1 )
			{
				if( errno == EAGAIN )
					return TCPMAIN_RETURN_WAITINGFOR_NEXT;
				(*p_body_bufsize) = 0 ;
				return TDHB_ERROR_RECV_FAILED;
			}
			else if( len == 0 )
			{
				(*p_body_bufsize) = head_len-p_context->head_remain_len ;
				return TDHB_ERROR_SOCKET_CLOSED;
			}
			
			if( len == p_context->head_remain_len )
			{
				DEBUGHEXLOG( p_context->head_buffer , head_len , "RECV HEAD [%d]BYTES" , head_len );
				
				p_context->head_remain_len = 0 ;
				p_context->head_buffer[head_len] = '\0' ;
				p_context->body_len = atoi(p_context->head_buffer) ;
				if( p_context->body_len > (*p_body_bufsize) )
					return TDHB_ERROR_RECV_BUFFER_OVERFLOW;
				p_context->body_remain_len = p_context->body_len - ( len-head_len ) ;
				if( p_context->body_remain_len == 0 )
				{
					(*p_body_bufsize) = 0 ;
					break;
				}
			}
			else
			{
				p_context->head_remain_len -= len ;
			}
		}
		else
		{
			len = read( sock , body_buffer+(p_context->body_len-p_context->body_remain_len) , p_context->body_remain_len ) ;
			if( len == -1 )
			{
				if( errno == EAGAIN )
					return TCPMAIN_RETURN_WAITINGFOR_NEXT;
				(*p_body_bufsize) = p_context->body_len - p_context->body_remain_len ;
				return TDHB_ERROR_RECV_FAILED;
			}
			else if( len == 0 )
			{
				(*p_body_bufsize) = head_len+p_context->body_len-p_context->body_remain_len ;
				return TDHB_ERROR_SOCKET_CLOSED;
			}
			
			p_context->body_remain_len -= len ;
			if( p_context->body_remain_len == 0 )
			{
				DEBUGHEXLOG( body_buffer , p_context->body_len , "RECV BODY [%d]BYTES" , p_context->body_len );
				(*p_body_bufsize) = p_context->body_len ;
				break;
			}
		}
	}
	
	return TCPMAIN_RETURN_WAITINGFOR_SENDING;
}

#endif

