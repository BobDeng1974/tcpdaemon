#include "tcpdaemon_in.h"

/*
 * tcpdaemon - TCP���ӹ����ػ�
 * author      : calvin
 * email       : calvinwilliams@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

/* �汾�ַ��� */
char		__TCPDAEMON_VERSION_1_1_0[] = "1.1.0" ;
char		*__TCPDAEMON_VERSION = __TCPDAEMON_VERSION_1_1_0 ;

static struct TcpdaemonServerEnvirment	*g_p_env = NULL ;
static int				g_EXIT_flag = 0 ;

#if ( defined _WIN32 )
static CRITICAL_SECTION		accept_critical_section; /* accept�ٽ��� */
#endif

#define MAX_INT(_a_,_b_)	((_a_)>(_b_)?(_a_):(_b_))

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
			ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker poll size[%ld] invalid" , p_para->process_count );
			return -1;
		}
		
		return 0;
	}
#elif ( defined _WIN32 )
	if( STRCMP( p_para->server_model , == , "WIN-TLF" ) )
	{
		if( p_para->process_count <= 0 )
		{
			ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker poll size[%ld] invalid" , p_para->process_count );
			return -1;
		}
		
		return 0;
	}
#endif
	
	ErrorLog( __FILE__ , __LINE__ , "server mode[%s] invalid" , p_para->server_model );
	return -2;
}

#if ( defined __unix ) || ( defined __linux__ )
static unsigned int tcpdaemon_LF_worker( void *pv )
#elif ( defined _WIN32 )
static unsigned int WINAPI tcpdaemon_LF_worker( void *pv )
#endif
{
	struct TcpdaemonServerEnvirment	*p_env = (struct TcpdaemonServerEnvirment *)pv ;
#if ( defined __linux__ ) || ( defined __unix )
	struct sembuf		sb ;
#endif
	fd_set			readfds ;
	
	struct sockaddr		accept_addr ;
	SOCKLEN_T		accept_addrlen ;
	int			accept_sock ;
	
	int			nret = 0 ;
	
	/* ������־���� */
	SetLogFile( p_env->p_para->log_pathfilename );
	SetLogLevel( p_env->p_para->log_level );
	
	DebugLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | begin" );
	
	while(1)
	{
		DebugLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | waiting for entering accept mutex" , p_env->index );
		
#if ( defined __linux__ ) || ( defined __unix )
		/* �����ٽ��� */
		memset( & sb , 0x00 , sizeof(struct sembuf) );
		sb.sem_num = 0 ;
		sb.sem_op = -1 ;
		sb.sem_flg = SEM_UNDO ;
		nret = semop( p_env->accept_mutex , & sb , 1 ) ;
		if( nret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | enter accept mutex failed , errno[%d]" , p_env->index , ERRNO );
			return 1;
		}
#elif ( defined _WIN32 )
		EnterCriticalSection( & accept_critical_section );
#endif
		DebugLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | enter accept mutex ok" , p_env->index );
		
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
			ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | select failed , errno[%d]" , p_env->index , ERRNO );
			break;
		}
		
#if ( defined __linux__ ) || ( defined __unix )
		if( FD_ISSET( p_env->alive_pipes[p_env->index].fd[0] , & readfds ) )
		{
			DebugLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | alive_pipe received quit command" , p_env->index );
			break;
		}
#endif
		
		/* �����¿ͻ������� */
		accept_addrlen = sizeof(struct sockaddr) ;
		memset( & accept_addr , 0x00 , accept_addrlen );
		accept_sock = accept( p_env->listen_sock , & accept_addr , & accept_addrlen ) ;
		if( accept_sock == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | accept failed , errno[%d]" , p_env->index , ERRNO );
			break;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | accept ok , [%d]accept[%d]" , p_env->index , p_env->listen_sock , accept_sock );
		}
		
		if( p_env->p_para->tcp_nodelay > 0 )
		{
			setsockopt( accept_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & (p_env->p_para->tcp_nodelay) , sizeof(int) );
		}
		
		if( p_env->p_para->tcp_linger > 0 )
		{
			struct linger	lg ;
			lg.l_onoff = 1 ;
			lg.l_linger = p_env->p_para->tcp_linger - 1 ;
			setsockopt( accept_sock , SOL_SOCKET , SO_LINGER , (void *) & lg , sizeof(struct linger) );
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
			ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | leave accept mutex failed , errno[%d]" , p_env->index , ERRNO );
			return 1;
		}
#elif ( defined _WIN32 )
		LeaveCriticalSection( & accept_critical_section );
#endif
		DebugLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | leave accept mutex ok" , p_env->index );
		
		/* ����ͨѶ����Э�鼰Ӧ�ô���ص����� */
		InfoLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | call tcpmain sock[%d]" , p_env->index , accept_sock );
		nret = p_env->pfunc_tcpmain( p_env , accept_sock , & accept_addr ) ;
		if( nret < 0 )
		{
			ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | tcpmain return[%d]" , p_env->index , nret );
			return 1;
		}
		else if( nret > 0 )
		{
			WarnLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | tcpmain return[%d]" , p_env->index , nret );
		}
		else
		{
			InfoLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | tcpmain return[%d]" , p_env->index , nret );
		}
		
		/* �رտͻ������� */
		CLOSESOCKET( accept_sock );
		DebugLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | close[%d]" , p_env->index , accept_sock );
		
		/* ��鹤�����̴������� */
		p_env->requests_per_process++;
		if( p_env->p_para->max_requests_per_process != 0 && p_env->requests_per_process >= p_env->p_para->max_requests_per_process )
		{
			InfoLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | maximum number of processing[%ld][%ld] , ending" , p_env->index , p_env->requests_per_process , p_env->p_para->max_requests_per_process );
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
		InfoLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | leave accept mutex finally failed , errno[%d]" , p_env->index , ERRNO );
		return 1;
	}
#elif ( defined _WIN32 )
	LeaveCriticalSection( & accept_critical_section );
#endif
	DebugLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | leave accept mutex finally ok" , p_env->index );
	
	DebugLog( __FILE__ , __LINE__ , "tcpdaemon_LF_worker(%ld) | end" );
	
#if ( defined __linux__ ) || ( defined __unix )
	return 0;
#elif ( defined _WIN32 )
	free( p_env );
	_endthreadex(0);
	return 0;
#endif
}

static unsigned int tcpdaemon_MPIO_worker( void *pv )
{
	struct TcpdaemonServerEnvirment	*p_env = (struct TcpdaemonServerEnvirment *)pv ;
	int				quit_flag ;
	struct epoll_event		event ;
	struct epoll_event		events[ MAX_EPOLL_EVENTS ] ;
	int				epoll_nfds ;
	int				i ;
	struct epoll_event		*p_event = NULL ;
	
	int				nret = 0 ;
	
	/* ������־���� */
	SetLogFile( p_env->p_para->log_pathfilename );
	SetLogLevel( p_env->p_para->log_level );
	
	DebugLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | begin" , p_env->index );
	
	/* ���������ɶ��¼���epoll */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = & (p_env->alive_pipes[p_env->index].fd[0]) ;
	nret = epoll_ctl( p_env->epoll_array[p_env->index] , EPOLL_CTL_ADD , p_env->alive_pipes[p_env->index].fd[0] , & event ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | epoll_ctl[%d] add pipe_session[%d] failed , errno[%d]" , p_env->index , p_env->epoll_array[p_env->index] , p_env->alive_pipes[p_env->index].fd[0] , errno );
		return 1;
	}
	else
	{
		InfoLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | epoll_ctl[%d] add pipe_session[%d] ok" , p_env->index , p_env->epoll_array[p_env->index] , p_env->alive_pipes[p_env->index].fd[0] );
	}
	
	/* �¼���ѭ�� */
	quit_flag = 0 ;
	while( ! quit_flag )
	{
		/* �ȴ�epoll�¼�������1�볬ʱ */
		InfoLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | epoll_wait[%d] ..." , p_env->index , p_env->epoll_array[p_env->index] );
		memset( events , 0x00 , sizeof(events) );
		epoll_nfds = epoll_wait( p_env->epoll_array[p_env->index] , events , MAX_EPOLL_EVENTS , 1000 ) ;
		if( epoll_nfds == -1 )
		{
			if( errno == EINTR )
			{
				InfoLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | epoll_wait[%d] interrupted" , p_env->index , p_env->epoll_array[p_env->index] );
				continue;
			}
			else
			{
				ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | epoll_wait[%d] failed , errno[%d]" , p_env->index , p_env->epoll_array[p_env->index] , ERRNO );
			}
			
			return 1;
		}
		else
		{
			InfoLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | epoll_wait[%d] return[%d]events" , p_env->index , p_env->epoll_array[p_env->index] , epoll_nfds );
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
					int			accept_sock ;
					struct sockaddr		accept_addr ;
					SOCKLEN_T		accept_addr_len ;
					
					while(1)
					{
						/* ���������� */
						memset( & accept_addr , 0x00 , sizeof(struct sockaddr_in) );
						accept_addr_len = sizeof(struct sockaddr) ;
						accept_sock = accept( p_env->listen_sock , (struct sockaddr *) & accept_addr , & accept_addr_len ) ;
						if( accept_sock == -1 )
						{
							if( errno == EAGAIN )
								break;
							ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | accept failed , errno[%d]" , p_env->index , errno );
							return 1;
						}
						
						/* ���ý���ͨѶ���ӻص����� */
						InfoLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | call tcpmain on OnAcceptingSocket , sock[%d]" , p_env->index , accept_sock );
						nret = p_env->pfunc_tcpmain( p_env , accept_sock , & accept_addr ) ;
						if( nret < 0 )
						{
							ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | tcpmain return[%d]" , p_env->index , nret );
							return 1;
						}
						else if( nret > 0 )
						{
							WarnLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | tcpmain return[%d]" , p_env->index , nret );
						}
						else
						{
							InfoLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | tcpmain return[%d]" , p_env->index , nret );
						}
					}
					
					if( p_env->process_count > 1 )
					{
						int		 j ;
						
						/* ת�������ɶ��¼�����һ��epoll */
						j = p_env->index ;
						if( j >= p_env->process_count )
							j = 0 ;
						
						epoll_ctl( p_env->epoll_array[p_env->index] , EPOLL_CTL_DEL , p_env->listen_sock , NULL );
						DebugLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | epoll_ctl[%d] remove listen sock[%d] ok" , p_env->index , p_env->epoll_array[p_env->index] , p_env->listen_sock );
						
						memset( & event , 0x00 , sizeof(struct epoll_event) );
						event.events = EPOLLIN | EPOLLERR ;
						event.data.ptr = & (p_env->listen_sock) ;
						nret = epoll_ctl( p_env->epoll_array[j] , EPOLL_CTL_ADD , p_env->listen_sock , & event ) ;
						if( nret == -1 )
						{
							ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | epoll_ctl[%d] add listen sock failed , errno[%d]" , p_env->index , p_env->epoll_array[p_env->index] , errno );
							return 1;
						}
						else
						{
							DebugLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | epoll_ctl[%d] add listen sock[%d] ok" , p_env->index , p_env->epoll_array[p_env->index] , p_env->listen_sock );
						}
					}
				}
				/* �����¼� */
				else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
				{
					FatalLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | listen session err or hup event[0x%X]" , p_env->index , p_event->events );
					return 1;
				}
				/* �����¼� */
				else
				{
					FatalLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | Unknow listen session event[0x%X]" , p_env->index , p_event->events );
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
				/* �ɶ��¼� */
				if( p_event->events & EPOLLIN )
				{
					/* ���ý���ͨѶ���ݻص����� */
					InfoLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | call tcpmain on OnReceivingSocket ..." , p_env->index );
					nret = p_env->pfunc_tcpmain( p_env , p_event->events , p_event->data.ptr ) ;
					if( nret < 0 )
					{
						ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | call tcpmain on OnReceivingSocket return[%d]" , p_env->index , nret );
						return 1;
					}
					else if( nret > 0 )
					{
						InfoLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | call tcpmain on OnReceivingSocket return[%d]" , p_env->index , nret );
						/* ���ùر����ӻص����� */
						p_env->pfunc_tcpmain( p_env , 0 , p_event->data.ptr );
					}
					else
					{
						DebugLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | call tcpmain on OnReceivingSocket return[%d]" , p_env->index , nret );
					}
				}
				/* ��д�¼� */
				else if( p_event->events & EPOLLOUT )
				{
					/* ���÷���ͨѶ���ݻص����� */
					InfoLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | call tcpmain on OnSendingSocket ..." , p_env->index );
					nret = p_env->pfunc_tcpmain( p_env , p_event->events , p_event->data.ptr ) ;
					if( nret < 0 )
					{
						ErrorLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | call tcpmain on OnSendingSocket return[%d]" , p_env->index , nret );
						return 1;
					}
					else if( nret > 0 )
					{
						InfoLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | call tcpmain on OnSendingSocket return[%d]" , p_env->index , nret );
						/* ���ùر����ӻص����� */
						p_env->pfunc_tcpmain( p_env , 0 , p_event->data.ptr );
					}
					else
					{
						DebugLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | call tcpmain on OnSendingSocket return[%d]" , p_env->index , nret );
					}
				}
				/* �����¼� */
				else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
				{
					FatalLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | accepted session err or hup event[0x%X]" , p_env->index , p_event->events );
					/* ���ùر����ӻص����� */
					p_env->pfunc_tcpmain( p_env , 0 , p_event->data.ptr );
				}
				/* �����¼� */
				else
				{
					FatalLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | Unknow accepted session event[0x%X]" , p_env->index , p_event->events );
					return 1;
				}
			}
		}
	}
	
	DebugLog( __FILE__ , __LINE__ , "tcpdaemon_MPIO_worker(%ld) | end" , p_env->index );
	
	return 0;
}

struct TcpdaemonServerEnvirment *DuplicateServerEnv( struct TcpdaemonServerEnvirment *p_env )
{
	struct TcpdaemonServerEnvirment	*pse_new = NULL ;

	pse_new = (struct TcpdaemonServerEnvirment *)malloc( sizeof(struct TcpdaemonServerEnvirment) );
	if( pse_new == NULL )
		return NULL;
	
	memcpy( pse_new , p_env , sizeof(struct TcpdaemonServerEnvirment) );
	
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
static int InitDaemonEnv( struct TcpdaemonServerEnvirment *p_env )
{
	int		nret = 0 ;
	
	p_env->listen_sock = -1 ;
	
	/* �õ�ͨѶ����Э�鼰Ӧ�ô���ص�����ָ�� */
	if( p_env->p_para->call_mode == TCPDAEMON_CALLMODE_CALLBACK )
	{
#if ( defined __linux__ ) || ( defined __unix )
		p_env->so_handle = dlopen( p_env->p_para->so_pathfilename , RTLD_LAZY ) ;
#elif ( defined _WIN32 )
		p_env->so_handle = LoadLibrary( p_env->p_para->so_pathfilename ) ;
#endif
		if( p_env->so_handle == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "dlopen[%s]failed[%s]" , p_env->p_para->so_pathfilename , DLERROR );
			return -1;
		}
		
#if ( defined __linux__ ) || ( defined __unix )
		p_env->pfunc_tcpmain = (func_tcpmain*)dlsym( p_env->so_handle , TCPMAIN ) ;
#elif ( defined _WIN32 )
		p_env->pfunc_tcpmain = (func_tcpmain*)GetProcAddress( p_env->so_handle , TCPMAIN ) ;
#endif
		if( p_env->pfunc_tcpmain == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "dlsym[%s]failed[%s]" , TCPMAIN , DLERROR );
			return -1;
		}
	}
	else if( p_env->p_para->call_mode == TCPDAEMON_CALLMODE_MAIN )
	{
		p_env->pfunc_tcpmain = p_env->p_para->pfunc_tcpmain ;
		p_env->param_tcpmain = p_env->p_para->param_tcpmain ;
	}
	else
	{
		ErrorLog( __FILE__ , __LINE__ , "call_mode[%d]invalid" , p_env->p_para->call_mode );
		return -1;
	}
	
	/* ��������socket */
	p_env->listen_sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( p_env->listen_sock == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "socket failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	
	/* ���������˿ڵ�ַ���� */
	{
		int	on = 1 ;
		setsockopt( p_env->listen_sock , SOL_SOCKET , SO_REUSEADDR , (void *) & on , sizeof(on) );
	}
	
	/* ���������˿ڹر�nagle�㷨 */
	{
		int	onoff = 1 ;
		setsockopt( p_env->listen_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) );
	}
	
	{
	memset( & (p_env->listen_addr) , 0x00 , sizeof(struct sockaddr_in) );
	p_env->listen_addr.sin_family = AF_INET ;
	p_env->listen_addr.sin_addr.s_addr = inet_addr( p_env->p_para->ip ) ;
	p_env->listen_addr.sin_port = htons( (unsigned short)p_env->p_para->port ) ;
	nret = bind( p_env->listen_sock , (struct sockaddr *) & (p_env->listen_addr), sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "bind[%s:%ld]failed , ERRNO[%d]" , p_env->p_para->ip , p_env->p_para->port , ERRNO );
		return -1;
	}
	}
	
	nret = listen( p_env->listen_sock , p_env->p_para->process_count * 2 ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "listen failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	
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
	
	if( STRCMP( p_env->p_para->work_path , != , "" ) )
	{
		CHDIR( p_env->p_para->work_path );
	}
	
	return 0;
}

/* �����ػ����� */
static int CleanDaemonEnv( struct TcpdaemonServerEnvirment *p_env )
{
	if( p_env->p_para->call_mode == TCPDAEMON_CALLMODE_CALLBACK )
	{
		if( p_env->so_handle )
		{
#if ( defined __linux__ ) || ( defined __unix )
			dlclose( p_env->so_handle );
#elif ( defined _WIN32 )
			FreeLibrary( p_env->so_handle );
#endif
		}
	}
	
	if( p_env->listen_sock != -1 )
	{
		CLOSE( p_env->listen_sock );
	}
	
	return 0;
}

#if ( defined __linux__ ) || ( defined __unix )

/* Instance-Fork����ģ�� ��ʼ���ػ����� */
static int InitDaemonEnv_IF( struct TcpdaemonServerEnvirment *p_env )
{
	return InitDaemonEnv( p_env );
}

/* Instance-Fork����ģ�� �����ػ����� */
static int CleanDaemonEnv_IF( struct TcpdaemonServerEnvirment *p_env )
{
	return CleanDaemonEnv( p_env );
}

/* Instance-Fork����ģ�� ��ں��� */
int tcpdaemon_IF( struct TcpdaemonServerEnvirment *p_env )
{
	struct sigaction	act , oldact ;
	
	fd_set			readfds ;
	struct timeval		tv ;
	
	struct sockaddr		accept_addr ;
	socklen_t		accept_addrlen ;
	int			accept_sock ;
	
	PID_T			pid ;
	int			status ;
	
	int			nret = 0 ;
	
	/* ��ʼ���ػ����� */
	nret = InitDaemonEnv_IF( p_env ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "init IF failed[%d]" , nret );
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
	
	InfoLog( __FILE__ , __LINE__ , "parent listen starting" );
	
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
				ErrorLog( __FILE__ , __LINE__ , "select failed , ERRNO[%d]" , ERRNO );
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
		accept_addrlen = sizeof(struct sockaddr) ;
		memset( & accept_addr , 0x00 , accept_addrlen );
		accept_sock = accept( p_env->listen_sock , & accept_addr , & accept_addrlen ) ;
		if( accept_sock == -1 )
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
				ErrorLog( __FILE__ , __LINE__ , "accept failed , ERRNO[%d]" , ERRNO );
				break;
			}
		}
		else
		{
			
			DebugLog( __FILE__ , __LINE__ , "accept ok , [%d]accept[%d]" , p_env->listen_sock , accept_sock );
		}
		
		if( p_env->p_para->process_count != 0 && p_env->process_count + 1 > p_env->p_para->process_count )
		{
			ErrorLog( __FILE__ , __LINE__ , "too many sockets" );
			CLOSE( accept_sock );
			continue;
		}
		
		if( p_env->p_para->tcp_nodelay > 0 )
		{
			setsockopt( accept_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & (p_env->p_para->tcp_nodelay) , sizeof(int) );
		}
		
		if( p_env->p_para->tcp_linger > 0 )
		{
			struct linger	lg ;
			lg.l_onoff = 1 ;
			lg.l_linger = p_env->p_para->tcp_linger - 1 ;
			setsockopt( accept_sock , SOL_SOCKET , SO_LINGER , (void *) & lg , sizeof(struct linger) );
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
				ErrorLog( __FILE__ , __LINE__ , "fork failed , ERRNO[%d]" , ERRNO );
				CLOSE( accept_sock );
				continue;
			}
		}
		else if( pid == 0 )
		{
			signal( SIGTERM , SIG_DFL );
			signal( SIGCHLD , SIG_DFL );
			
			CLOSESOCKET( p_env->listen_sock );
			
			/* ����ͨѶ����Э�鼰Ӧ�ô���ص����� */
			InfoLog( __FILE__ , __LINE__ , "call tcpmain sock[%d]" , accept_sock );
			nret = p_env->pfunc_tcpmain( p_env , accept_sock , & accept_addr ) ;
			if( nret < 0 )
			{
				ErrorLog( __FILE__ , __LINE__ , "tcpmain return[%d]" , nret );
			}
			else if( nret > 0 )
			{
				WarnLog( __FILE__ , __LINE__ , "tcpmain return[%d]" , nret );
			}
			else
			{
				InfoLog( __FILE__ , __LINE__ , "tcpmain return[%d]" , nret );
			}
			
			/* �رտͻ������� */
			CLOSESOCKET( accept_sock );
			DebugLog( __FILE__ , __LINE__ , "CLOSE[%d]" , accept_sock );
			
			/* �ӽ����˳� */
			DebugLog( __FILE__ , __LINE__ , "child exit" );
			exit(0);
		}
		else
		{
			CLOSESOCKET( accept_sock );
			p_env->process_count++;
		}
	}
	
	sigaction( SIGTERM , & oldact , NULL );
	
	InfoLog( __FILE__ , __LINE__ , "parent listen ended" );
	
	InfoLog( __FILE__ , __LINE__ , "waiting for all children exit starting" );
	
	while( p_env->process_count > 0 )
	{
		waitpid( -1 , & status , 0 );
	}
	
	InfoLog( __FILE__ , __LINE__ , "waiting for all chdilren exit ended" );
	
	/* �����ػ����� */
	CleanDaemonEnv_IF( p_env );
	
	return 0;
}

/* Leader-Follower���̳�ģ�� ��ʼ���ػ����� */
static int InitDaemonEnv_LF( struct TcpdaemonServerEnvirment *p_env )
{
	int		nret = 0 ;
	
	nret = InitDaemonEnv( p_env ) ;
	if( nret )
		return nret;
	
	/* ����pid������Ϣ���� */
	p_env->pids = (pid_t*)malloc( sizeof(pid_t) * (p_env->p_para->process_count) ) ;
	if( p_env->pids == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->pids , 0x00 , sizeof(pid_t) * (p_env->p_para->process_count) );
	
	/* �������ܵ���Ϣ���� */
	p_env->alive_pipes = (PIPE_T*)malloc( sizeof(PIPE_T) * (p_env->p_para->process_count) ) ;
	if( p_env->alive_pipes == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->alive_pipes , 0x00 , sizeof(PIPE_T) * (p_env->p_para->process_count) );
	
	/* ����accept�ٽ��� */
	p_env->accept_mutex = semget( IPC_PRIVATE , 1 , IPC_CREAT | 00777 ) ;
	if( p_env->accept_mutex == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "create mutex failed" );
		return -1;
	}
	
	{
	union semun	semopts ;
	semopts.val = 1 ;
	nret = semctl( p_env->accept_mutex , 0 , SETVAL , semopts ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "set mutex failed" );
		return -1;
	}
	}
	
	return 0;
}

/* Leader-Follower���̳�ģ�� �����ػ����� */
static int CleanDaemonEnv_LF( struct TcpdaemonServerEnvirment *p_env )
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
int tcpdaemon_LF( struct TcpdaemonServerEnvirment *p_env )
{
	PID_T			pid ;
	int			status ;
	
	struct sigaction	act , oldact ;
	
	int			nret = 0 ;
	
	/* ��ʼ���ػ����� */
	nret = InitDaemonEnv_LF( p_env ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "init LF failed[%d]" , nret );
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
	InfoLog( __FILE__ , __LINE__ , "create tcpdaemon_LF_worker pool starting" );
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		nret = pipe( p_env->alive_pipes[p_env->index].fd ) ;
		if( nret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_LF( p_env );
			return -11;
		}
		
		p_env->pids[p_env->index] = fork() ;
		if( p_env->pids[p_env->index] == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "fork failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_LF( p_env );
			return -11;
		}
		else if( p_env->pids[p_env->index] == 0 )
		{
			signal( SIGTERM , SIG_DFL );
			
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
	
	InfoLog( __FILE__ , __LINE__ , "create tcpdaemon_LF_worker pool ended" );
	
	/* ��ع������̳� */
	InfoLog( __FILE__ , __LINE__ , "monitoring all children starting" );
	
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
				
				InfoLog( __FILE__ , __LINE__ , "detecting child[%ld]pid[%ld] exit , rebooting" , p_env->index , (long)pid );
				
				SLEEP(1);
				
				p_env->requests_per_process = 0 ;
				
				nret = pipe( p_env->alive_pipes[p_env->index].fd ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "pipe failed , ERRNO[%d]" , ERRNO );
					CleanDaemonEnv_LF( p_env );
					return -11;
				}
				
				p_env->pids[p_env->index] = fork() ;
				if( p_env->pids[p_env->index] == -1 )
				{
					ErrorLog( __FILE__ , __LINE__ , "fork failed , ERRNO[%d]" , ERRNO );
					CleanDaemonEnv_LF( p_env );
					return -11;
				}
				else if( p_env->pids[p_env->index] == 0 )
				{
					signal( SIGTERM , SIG_DFL );
					
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
			InfoLog( __FILE__ , __LINE__ , "detecting unknow child pid[%ld] exit" , (long)pid );
			continue;
		}
	}
	
	InfoLog( __FILE__ , __LINE__ , "monitoring all children ended" );
	
	sigaction( SIGTERM , & oldact , NULL );
	
	/* ���ٽ��̳� */
	InfoLog( __FILE__ , __LINE__ , "destroy tcpdaemon_LF_worker poll starting" );
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		write( p_env->alive_pipes[p_env->index].fd[1] , "\0" , 1 );
	}
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		waitpid( -1 , & status , 0 );
		CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
	}
	
	InfoLog( __FILE__ , __LINE__ , "destroy tcpdaemon_LF_worker poll ended" );
	
	/* �����ػ����� */
	CleanDaemonEnv_LF( p_env );
	
	return 0;
}

/* MultiplexIO���̳�ģ�� ��ʼ���ػ����� */
static int InitDaemonEnv_MPIO( struct TcpdaemonServerEnvirment *p_env )
{
	struct epoll_event	event ;
	int			i ;
	
	int			nret = 0 ;
	
	nret = InitDaemonEnv( p_env ) ;
	if( nret )
		return nret;
	
	/* ���������˿ڷǶ��� */
	{
		int	opts;
		opts = fcntl( p_env->listen_sock , F_GETFL );
		opts |= O_NONBLOCK ;
		fcntl( p_env->listen_sock , F_SETFL , opts );
	}
	
	/* ����pid������Ϣ���� */
	p_env->pids = (pid_t*)malloc( sizeof(pid_t) * (p_env->p_para->process_count) ) ;
	if( p_env->pids == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->pids , 0x00 , sizeof(pid_t) * (p_env->p_para->process_count) );
	
	/* �������ܵ���Ϣ���� */
	p_env->alive_pipes = (PIPE_T*)malloc( sizeof(PIPE_T) * (p_env->p_para->process_count) ) ;
	if( p_env->alive_pipes == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->alive_pipes , 0x00 , sizeof(PIPE_T) * (p_env->p_para->process_count) );
	
	/* ����epoll���� */
	p_env->epoll_array = (int*)malloc( sizeof(int) * (p_env->p_para->process_count) ) ;
	if( p_env->epoll_array == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->epoll_array , 0x00 , sizeof(int) * (p_env->p_para->process_count) );
	
	for( i = 0 ; i < p_env->p_para->process_count ; i++ )
	{
		p_env->epoll_array[i] = epoll_create( 1024 ) ;
		if( p_env->epoll_array[i] == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "epoll_create failed , errno[%d]" , errno );
			return -1;
		}
		else
		{
			InfoLog( __FILE__ , __LINE__ , "epoll_create ok" );
		}
	}
	
	/* ���������ɶ��¼���epoll */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = & (p_env->listen_sock) ;
	nret = epoll_ctl( p_env->epoll_array[0] , EPOLL_CTL_ADD , p_env->listen_sock , & event ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "epoll_ctl[%d] add listen_session failed , errno[%d]" , p_env->epoll_array[0] , errno );
		return -1;
	}
	else
	{
		InfoLog( __FILE__ , __LINE__ , "epoll_ctl[%d] add listen_session[%d] ok" , p_env->epoll_array[0] , p_env->listen_sock );
	}
	
	return 0;
}

/* MultiplexIO���̳�ģ�� �����ػ����� */
static int CleanDaemonEnv_MPIO( struct TcpdaemonServerEnvirment *p_env )
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
	
	for( i = 0 ; i < p_env->p_para->process_count ; i++ )
	{
		close( p_env->epoll_array[i] );
	}
	
	return CleanDaemonEnv( p_env );
}

/* MultiplexIO���̳�ģ�� ��ں��� */
int tcpdaemon_MPIO( struct TcpdaemonServerEnvirment *p_env )
{
	PID_T			pid ;
	int			status ;
	
	struct sigaction	act , oldact ;
	
	int			nret = 0 ;
	
	/* ��ʼ���ػ����� */
	nret = InitDaemonEnv_MPIO( p_env ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "init LF failed[%d]" , nret );
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
	InfoLog( __FILE__ , __LINE__ , "create tcpdaemon_LF_worker pool starting" );
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		nret = pipe( p_env->alive_pipes[p_env->index].fd ) ;
		if( nret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_LF( p_env );
			return -11;
		}
		
		p_env->pids[p_env->index] = fork() ;
		if( p_env->pids[p_env->index] == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "fork failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_MPIO( p_env );
			return -11;
		}
		else if( p_env->pids[p_env->index] == 0 )
		{
			signal( SIGTERM , SIG_DFL );
			
			CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
			tcpdaemon_MPIO_worker( p_env );
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
	
	InfoLog( __FILE__ , __LINE__ , "create tcpdaemon_MPIO_worker pool ended" );
	
	/* ��ع������̳� */
	InfoLog( __FILE__ , __LINE__ , "monitoring all children starting" );
	
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
				
				InfoLog( __FILE__ , __LINE__ , "detecting child[%ld]pid[%ld] exit , rebooting" , p_env->index , (long)pid );
				
				SLEEP(1);
				
				p_env->requests_per_process = 0 ;
				
				nret = pipe( p_env->alive_pipes[p_env->index].fd ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "pipe failed , ERRNO[%d]" , ERRNO );
					CleanDaemonEnv_LF( p_env );
					return -11;
				}
				
				p_env->pids[p_env->index] = fork() ;
				if( p_env->pids[p_env->index] == -1 )
				{
					ErrorLog( __FILE__ , __LINE__ , "fork failed , ERRNO[%d]" , ERRNO );
					CleanDaemonEnv_MPIO( p_env );
					return -11;
				}
				else if( p_env->pids[p_env->index] == 0 )
				{
					signal( SIGTERM , SIG_DFL );
					
					CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
					tcpdaemon_MPIO_worker( p_env );
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
			InfoLog( __FILE__ , __LINE__ , "detecting unknow child pid[%ld] exit" , (long)pid );
			continue;
		}
	}
	
	InfoLog( __FILE__ , __LINE__ , "monitoring all children ended" );
	
	sigaction( SIGTERM , & oldact , NULL );
	
	/* ���ٽ��̳� */
	InfoLog( __FILE__ , __LINE__ , "destroy tcpdaemon_LF_worker poll starting" );
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		write( p_env->alive_pipes[p_env->index].fd[1] , "\0" , 1 );
	}
	
	for( p_env->index = 0 ; p_env->index < p_env->p_para->process_count ; p_env->index++ )
	{
		waitpid( -1 , & status , 0 );
		CLOSE( p_env->alive_pipes[p_env->index].fd[1] );
	}
	
	InfoLog( __FILE__ , __LINE__ , "destroy tcpdaemon_LF_worker poll ended" );
	
	/* �����ػ����� */
	CleanDaemonEnv_MPIO( p_env );
	
	return 0;
}

#endif

#if ( defined _WIN32 )

/* Leader-Follower�̳߳�ģ�� ��ʼ���ػ����� */
static int InitDaemonEnv_WIN_TLF( struct TcpdaemonServerEnvirment *p_env )
{
	int		nret = 0 ;
	
	nret = InitDaemonEnv( p_env ) ;
	if( nret )
		return nret;
	
	/* ��ʼ���ٽ��� */
	InitializeCriticalSection( & accept_critical_section ); 
	
	/* ����tid������Ϣ���� */
	p_env->thandles = (THANDLE_T*)malloc( sizeof(THANDLE_T) * (p_env->p_para->process_count+1) ) ;
	if( p_env->thandles == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->thandles , 0x00 , sizeof(THANDLE_T) * (p_env->p_para->process_count+1) );
	
	p_env->tids = (TID_T*)malloc( sizeof(TID_T) * (p_env->p_para->process_count+1) ) ;
	if( p_env->tids == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "alloc failed , ERRNO[%d]" , ERRNO );
		return -1;
	}
	memset( p_env->tids , 0x00 , sizeof(TID_T) * (p_env->p_para->process_count+1) );
	
	return 0;
}

/* Leader-Follower�̳߳�ģ�� �����ػ����� */
static int CleanDaemonEnv_WIN_TLF( struct TcpdaemonServerEnvirment *p_env )
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
int tcpdaemon_WIN_TLF( struct TcpdaemonServerEnvirment *p_env )
{
	struct TcpdaemonServerEnvirment	*pse_new = NULL ;
	unsigned long			index ;
	
	int				nret = 0 ;
	long				lret = 0 ;
	
	/* ��ʼ���ػ����� */
	nret = InitDaemonEnv_WIN_TLF( p_env ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "init WIN-TLF failed[%d]" , nret );
		CleanDaemonEnv_WIN_TLF( p_env );
		return nret;
	}
	
	/* �����������̳� */
	InfoLog( __FILE__ , __LINE__ , "create tcpdaemon_LF_worker pool starting" );
	
	for( p_env->index = 1 ; p_env->index <= p_env->p_para->process_count ; p_env->index++ )
	{
		pse_new = DuplicateServerEnv( p_env ) ;
		if( pse_new == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "DuplicateServerEnv failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_WIN_TLF( p_env );
			return -11;
		}
		
		p_env->thandles[p_env->index] = (THANDLE_T)_beginthreadex( NULL , 0 , tcpdaemon_LF_worker , pse_new , 0 , & (p_env->tids[p_env->index]) ) ;
		if( p_env->thandles[p_env->index] == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "_beginthreadex failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_WIN_TLF( p_env );
			return -12;
		}
		
		SLEEP(1);
	}
	
	InfoLog( __FILE__ , __LINE__ , "create tcpdaemon_LF_worker pool ended" );
	
	/* ��ع����̳߳� */
	InfoLog( __FILE__ , __LINE__ , "monitoring all children starting" );
	
	while(1)
	{
		/* ��ع����߳̽����¼� */
		index = WaitForMultipleObjects( p_env->p_para->process_count , p_env->thandles , FALSE , INFINITE ) ;
		if( index == WAIT_FAILED )
		{
			ErrorLog( __FILE__ , __LINE__ , "WaitForMultipleObjects failed , errno[%d]" , ERRNO );
			break;
		}
		index = index - WAIT_OBJECT_0 + 1 ;
		CloseHandle( p_env->thandles[index-1] );
		
		/* �����߳̽��� */
		InfoLog( __FILE__ , __LINE__ , "detecting child[%ld]tid[%ld] exit , rebooting" , index , p_env->tids[index-1] );
		
		Sleep(1000);
		
		p_env->requests_per_process = 0 ;
		
		pse_new = DuplicateServerEnv( p_env ) ;
		if( pse_new == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "DuplicateServerEnv failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_WIN_TLF( p_env );
			return -11;
		}
		
		p_env->thandles[index-1] = (THANDLE_T)_beginthreadex( NULL , 0 , tcpdaemon_LF_worker , pse_new , 0 , & (p_env->tids[index-1]) ) ;
		if( p_env->thandles[index-1] == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "_beginthreadex failed , ERRNO[%d]" , ERRNO );
			CleanDaemonEnv_WIN_TLF( p_env );
			return -12;
		}
	}
	
	InfoLog( __FILE__ , __LINE__ , "monitoring all children ended" );
	
	/* �����̳߳� */
	InfoLog( __FILE__ , __LINE__ , "destroy tcpdaemon_LF_worker poll starting" );
	
	for( p_env->index = 1 ; p_env->index <= p_env->p_para->process_count ; p_env->index++ )
	{
		lret = WaitForMultipleObjects( p_env->p_para->process_count , p_env->thandles , TRUE , INFINITE ) ;
		if( lret == WAIT_FAILED )
		{
			break;
		}	
	}
	
	InfoLog( __FILE__ , __LINE__ , "destroy tcpdaemon_LF_worker poll ended" );
	
	/* �����ػ����� */
	CleanDaemonEnv_WIN_TLF( p_env );
	
	return 0;
}

#endif

/* ����ں��� */
func_tcpdaemon tcpdaemon ;
int tcpdaemon( struct TcpdaemonEntryParameter *p_para )
{
	struct TcpdaemonServerEnvirment	se ;
	
	int				nret = 0 ;
	
	memset( & se , 0x00 , sizeof(struct TcpdaemonServerEnvirment) );
	se.p_para = p_para ;
	g_p_env = & se ;
	
	/* ������־���� */
	SetLogFile( p_para->log_pathfilename );
	SetLogLevel( p_para->log_level );
	InfoLog( __FILE__ , __LINE__ , "tcpdaemon startup - v%s" , __TCPDAEMON_VERSION );
	
	/* �����ڲ��� */
	nret = CheckCommandParameter( p_para ) ;
	if( nret )
		return nret;
	
#if ( defined __linux__ ) || ( defined __unix )
	if( STRCMP( p_para->server_model , == , "IF" ) )
	{
		/* ����Instance-Fork����ģ�� ��ں��� */
		nret = tcpdaemon_IF( & se ) ;
	}
	if( STRCMP( p_para->server_model , == , "LF" ) )
	{
		/* ����Leader-Follow���̳�ģ�� ��ں��� */
		nret = tcpdaemon_LF( & se ) ;
	}
	if( STRCMP( p_para->server_model , == , "MPIO" ) )
	{
		/* ����MultiplexIO���̳�ģ�� ��ں��� */
		nret = tcpdaemon_MPIO( & se ) ;
	}
#elif ( defined _WIN32 )
	if( STRCMP( p_para->server_model , == , "WIN-TLF" ) )
	{
		/* ����Leader-Follow�̳߳�ģ�� ��ں��� */
		nret = tcpdaemon_WIN_TLF( & se ) ;
	}
#endif
	
	InfoLog( __FILE__ , __LINE__ , "tcpdaemon ended - v%s" , __TCPDAEMON_VERSION );
	
	return nret;
}

void *GetTcpmainParameter( struct TcpdaemonServerEnvirment *p_env )
{
	return p_env->param_tcpmain;
}

int GetListenSocket( struct TcpdaemonServerEnvirment *p_env )
{
	return p_env->listen_sock;
}

int *GetListenSocketPtr( struct TcpdaemonServerEnvirment *p_env )
{
	return &(p_env->listen_sock);
}

struct sockaddr_in GetListenAddress( struct TcpdaemonServerEnvirment *p_env )
{
	return p_env->listen_addr;
}

struct sockaddr_in *GetListenAddressPtr( struct TcpdaemonServerEnvirment *p_env )
{
	return &(p_env->listen_addr);
}

int GetProcessCount( struct TcpdaemonServerEnvirment *p_env )
{
	return p_env->p_para->process_count;
}

int *GetEpollArrayBase( struct TcpdaemonServerEnvirment *p_env )
{
	return p_env->epoll_array;
}
