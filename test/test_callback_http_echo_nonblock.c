#include "tcpdaemon.h"

struct AcceptedSession
{
	int		sock ; /* socket������ */
	struct sockaddr	addr ; /* socket��ַ */
	
	char		http_buffer[ 4096 + 1 ] ; /* HTTP�շ������� */
	int		read_len ; /* ���˶����ֽ� */
	int		write_len ; /* ��Ҫд�����ֽ� */
	int		wrote_len ; /* д�˶����ֽ� */
} ;

_WINDLL_FUNC int tcpmain( struct TcpdaemonServerEnvironment *p_env , int sock , void *p_addr )
{
	struct AcceptedSession	*p_accepted_session = NULL ;
	int			len ;
	
	switch( TDGetIoMultiplexEvent(p_env) )
	{
		/* �����������¼� */
		case IOMP_ON_ACCEPTING_SOCKET :
			/* �����ڴ��Դ�������ӻỰ */
			p_accepted_session = (struct AcceptedSession *)malloc( sizeof(struct AcceptedSession) ) ;
			if( p_accepted_session == NULL )
				return TCPMAIN_RETURN_ERROR;
			memset( p_accepted_session , 0x00 , sizeof(struct AcceptedSession) );
			
			p_accepted_session->sock = sock ;
			memcpy( & (p_accepted_session->addr) , p_addr , sizeof(struct sockaddr) );
			
			/* ���������ӻỰ���ݽṹ */
			TDSetIoMultiplexDataPtr( p_env , p_accepted_session );
			
			/* �ȴ����¼� */
			return TCPMAIN_RETURN_WAITINGFOR_RECEIVING;
			
		/* �ر������¼� */
		case IOMP_ON_CLOSING_SOCKET :
			/* �ͷ������ӻỰ */
			p_accepted_session = (struct AcceptedSession *) p_addr ;
			free( p_accepted_session );
			
			/* �ȴ���һ�����¼� */
			return TCPMAIN_RETURN_WAITINGFOR_NEXT;
			
		/* ͨѶ�����¼� */
		case IOMP_ON_RECEIVING_SOCKET :
			p_accepted_session = (struct AcceptedSession *) p_addr ;
			
			/* �Ƕ�������ͨѶ���� */
			len = RECV( p_accepted_session->sock , p_accepted_session->http_buffer+p_accepted_session->read_len , sizeof(p_accepted_session->http_buffer)-1-p_accepted_session->read_len , 0 ) ;
			if( len == 0 )
				return TCPMAIN_RETURN_CLOSE;
			else if( len == -1 )
				return TCPMAIN_RETURN_ERROR;
			
			/* �ѽ������ݳ����ۼ� */
			p_accepted_session->read_len += len ;
			
			/* ��������� */
			if( strstr( p_accepted_session->http_buffer , "\r\n\r\n" ) )
			{
				/* ��֯��Ӧ���� */
				p_accepted_session->write_len = sprintf( p_accepted_session->http_buffer , "HTTP/1.0 200 OK\r\n"
											"Content-length: 17\r\n"
											"\r\n"
											"Hello Tcpdaemon\r\n" ) ;
				return TCPMAIN_RETURN_WAITINGFOR_SENDING;
			}
			
			/* ��������������˻�û���� */
			if( p_accepted_session->read_len == sizeof(p_accepted_session->http_buffer)-1 )
				return TCPMAIN_RETURN_ERROR;
			
			/* �ȴ���һ�����¼� */
			return TCPMAIN_RETURN_WAITINGFOR_NEXT;
			
		/* ͨѶ�����¼� */
		case IOMP_ON_SENDING_SOCKET :
			p_accepted_session = (struct AcceptedSession *) p_addr ;
			
			/* �Ƕ�������ͨѶ���� */
			len = SEND( p_accepted_session->sock , p_accepted_session->http_buffer+p_accepted_session->wrote_len , p_accepted_session->write_len-p_accepted_session->wrote_len , 0 ) ;
			if( len == -1 )
				return TCPMAIN_RETURN_ERROR;
			
			/* �ѷ������ݳ����ۼ� */
			p_accepted_session->wrote_len += len ;
			
			/* ����ѷ��� */
			if( p_accepted_session->wrote_len == p_accepted_session->write_len )
				return TCPMAIN_RETURN_CLOSE;
			
			/* �ȴ���һ�����¼� */
			return TCPMAIN_RETURN_WAITINGFOR_NEXT;
			
		default :
			return TCPMAIN_RETURN_ERROR;
	}
}

