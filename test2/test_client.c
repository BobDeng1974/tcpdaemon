#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "IDL_hello_world.dsc.h"

#include "tcpdaemon.h"

#define MAXCNT_EVENTS		1024

#define COMM_HEAD_LEN		4

struct ConnectedSession
{
	int			sock ; /* socket������ */
	struct sockaddr		addr ; /* socket��ַ */
	
	char			tdhb_buffer[ 9999 + 1 ] ; /* �շ������� */
	int			tdhb_len ;
	struct TDHBContext	tdhb_context ; /* �̶�ͨѶͷ+ͨѶ�� �շ������� */
} ;

static int test_client( char *ip , int port , int processor_count , int total_processing_count )
{
	struct ConnectedSession	*p_connected_session_array = NULL ;
	struct ConnectedSession	*p_connected_session = NULL ;
	int			i ;
	hello_world		st ;
	int			epoll_fd ;
	struct epoll_event	event ;
	struct epoll_event	*p_event = NULL ;
	struct epoll_event	events[ MAXCNT_EVENTS ] ;
	int			epoll_nfds ;
	int			unpack_len ;
	
	int			nret = 0 ;
	
	/* �����ڴ��Դ�����ӻỰ���� */
	p_connected_session_array = (struct ConnectedSession *)malloc( sizeof(struct ConnectedSession) * processor_count ) ;
	if( p_connected_session_array == NULL )
	{
		printf( "malloc failed , errno[%d]\n" , errno );
		return -1;
	}
	memset( p_connected_session_array , 0x00 , sizeof(struct ConnectedSession) * processor_count );
	
	/* ����epoll�� */
	epoll_fd = epoll_create( 1024 ) ;
	if( epoll_fd == -1 )
	{
		printf( "epoll_create failed , errno[%d]\n" , errno );
		return -1;
	}
	
	for( i = 0 , p_connected_session = p_connected_session_array ; i < processor_count ; i++ , p_connected_session++ )
	{
		/* ���ӷ���� */
		p_connected_session->sock = TDHBConnect( ip , port ) ;
		if( p_connected_session->sock == -1 )
		{
			printf( "TDHBConnect failed[%d] , errno[%d]\n" , p_connected_session->sock , errno );
			return -1;
		}
		
		/* ��֯���� */
		memset( & st , 0x00 , sizeof(hello_world) );
		strcpy( st.message , "hello world" );
		
		/* ���л����� */
		memset( p_connected_session->tdhb_buffer , 0x00 , sizeof(p_connected_session->tdhb_buffer) );
		p_connected_session->tdhb_len = sizeof(p_connected_session->tdhb_buffer) ;
		nret = DSCSERIALIZE_COMPRESS_hello_world( & st , p_connected_session->tdhb_buffer , & (p_connected_session->tdhb_len) ) ;
		if( nret )
		{
			printf( "DSCSERIALIZE_COMPRESS_hello_world failed , errno[%d]\n" , errno );
			return -1;
		}
		
		/* ����ͻ����׽��ֵ�epoll���� */
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.events = EPOLLOUT | EPOLLERR ;
		event.data.ptr = p_connected_session ;
		nret = epoll_ctl( epoll_fd , EPOLL_CTL_ADD , p_connected_session->sock , & event ) ;
		if( nret == -1 )
		{
			printf( "epoll_ctl EPOLL_CTL_ADD failed , errno[%d]\n" , errno );
			return -1;
		}
	}
	
	/* ֱ���ľ�������������� */
	while( total_processing_count || processor_count )
	{
		/* �ȴ�epoll���¼� */
		memset( events , 0x00 , sizeof(events) );
		epoll_nfds = epoll_wait( epoll_fd , events , MAXCNT_EVENTS , -1 ) ;
		if( epoll_nfds == -1 )
		{
			printf( "epoll_wait failed , errno[%d]\n" , errno );
			return -1;
		}
		
		for( i = 0 , p_event = events ; i < epoll_nfds ; i++ , p_event++ )
		{
			p_connected_session = p_event->data.ptr ;
			
			if( p_event->events & EPOLLOUT )
			{
				nret = TDHBSendDataWithNonblock( p_connected_session->sock , COMM_HEAD_LEN , p_connected_session->tdhb_buffer , & (p_connected_session->tdhb_len) , & (p_connected_session->tdhb_context) ) ;
				if( nret == TCPMAIN_RETURN_WAITINGFOR_NEXT )
				{
					;
				}
				else if( nret == TCPMAIN_RETURN_WAITINGFOR_RECEIVING )
				{
					/* ��ʼ�������� */
					memset( p_connected_session->tdhb_buffer , 0x00 , sizeof(p_connected_session->tdhb_buffer) );
					p_connected_session->tdhb_len = sizeof(p_connected_session->tdhb_buffer) ;
					
					/* ���ÿɶ��¼� */
					memset( & event , 0x00 , sizeof(struct epoll_event) );
					event.events = EPOLLIN | EPOLLERR ;
					event.data.ptr = p_connected_session ;
					nret = epoll_ctl( epoll_fd , EPOLL_CTL_MOD , p_connected_session->sock , & event ) ;
					if( nret == -1 )
					{
						printf( "epoll_ctl EPOLL_CTL_MOD failed , errno[%d]\n" , errno );
						return -1;
					}
				}
				else if( nret )
				{
					printf( "TDHBSendDataWithNonblock failed[%d] , errno[%d]\n" , nret , errno );
					return -1;
				}
			}
			else if( p_event->events & EPOLLIN )
			{
				nret = TDHBReceiveDataWithNonblock( p_connected_session->sock , COMM_HEAD_LEN , p_connected_session->tdhb_buffer , & (p_connected_session->tdhb_len) , & (p_connected_session->tdhb_context) ) ;
				if( nret == TCPMAIN_RETURN_WAITINGFOR_NEXT )
				{
					;
				}
				else if( nret == TCPMAIN_RETURN_WAITINGFOR_SENDING )
				{
					/* �����л����� */
					unpack_len = 0 ;
					memset( & st , 0x00 , sizeof(hello_world) );
					nret = DSCDESERIALIZE_COMPRESS_hello_world( p_connected_session->tdhb_buffer , & unpack_len , & st ) ;
					if( nret )
					{
						printf( "DSCSERIALIZE_COMPRESS_hello_world failed , errno[%d]\n" , errno );
						return -1;
					}
					
					if( total_processing_count )
					{
						/* ���л����� */
						memset( p_connected_session->tdhb_buffer , 0x00 , sizeof(p_connected_session->tdhb_buffer) );
						p_connected_session->tdhb_len = sizeof(p_connected_session->tdhb_buffer) ;
						nret = DSCSERIALIZE_COMPRESS_hello_world( & st , p_connected_session->tdhb_buffer , & (p_connected_session->tdhb_len) ) ;
						if( nret )
						{
							printf( "DSCSERIALIZE_COMPRESS_hello_world failed , errno[%d]\n" , errno );
							return -1;
						}
						
						memset( & (p_connected_session->tdhb_context) , 0x00 , sizeof(struct TDHBContext) );
						
						/* ���ÿ�д�¼� */
						memset( & event , 0x00 , sizeof(struct epoll_event) );
						event.events = EPOLLOUT | EPOLLERR ;
						event.data.ptr = p_connected_session ;
						nret = epoll_ctl( epoll_fd , EPOLL_CTL_MOD , p_connected_session->sock , & event ) ;
						if( nret == -1 )
						{
							printf( "epoll_ctl EPOLL_CTL_MOD failed , errno[%d]\n" , errno );
							return -1;
						}
						
						/* ��������������Լ�һ */
						total_processing_count--;
					}
					else
					{
						/* ��epoll����ɾ���ͻ����׽��� */
						epoll_ctl( epoll_fd , EPOLL_CTL_DEL , p_connected_session->sock , NULL );
						close( p_connected_session->sock );
						
						processor_count--;
					}
				}
				else if( nret )
				{
					printf( "TDHBSendDataWithNonblock failed[%d] , errno[%d]\n" , nret , errno );
					return -1;
				}
			}
			else if( p_event->events & EPOLLERR )
			{
				printf( "EPOLLERR event\n" );
				return -1;
			}
			else
			{
				printf( "unknow event[%d]\n" , p_event->events );
				return -1;
			}
		}
	}
	
	close( epoll_fd );
	
	free( p_connected_session_array );
	
	return 0;
}

static void usage()
{
	printf( "USAGE : test_client ip port processor_count total_processing_count\n" );
	return;
}

int main( int argc , char *argv[] )
{
	if( argc != 1 + 4 )
	{
		usage();
		exit(7);
	}
	
	setbuf( stdout , NULL );
	
	return -test_client( argv[1] , atoi(argv[2]) , atoi(argv[3]) , atoi(argv[4]) );
}

