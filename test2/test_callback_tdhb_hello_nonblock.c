#include "tcpdaemon.h"

#include "IDL_hello_world.dsc.h"

#define PREHEAD_LEN		4

struct AcceptedSession
{
	int			sock ; /* socket������ */
	struct sockaddr		addr ; /* socket��ַ */
	
	char			tdhb_buffer[ 9999 + 1 ] ; /* �շ������� */
	int			tdhb_len ;
	struct TDHBContext	context ; /* �̶�ͨѶͷ+ͨѶ�� �շ������� */
} ;

_WINDLL_FUNC int tcpmain( struct TcpdaemonServerEnvironment *p_env , int sock , void *p_addr )
{
	struct AcceptedSession	*p_accepted_session = NULL ;
	hello_world		st ;
	int			unpack_len ;
	int			nret = 0 ;
	
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
			p_accepted_session->tdhb_len = sizeof(p_accepted_session->tdhb_buffer) ;
			nret = TDHBReceiveDataWithNonblock( p_accepted_session->sock , PREHEAD_LEN , p_accepted_session->tdhb_buffer , & (p_accepted_session->tdhb_len) , & (p_accepted_session->context) ) ;
			if( nret == TDHB_ERROR_SOCKET_CLOSED )
			{
				if( p_accepted_session->tdhb_len == 0 )
					return TCPMAIN_RETURN_CLOSE;
				else
					return TCPMAIN_RETURN_ERROR;
			}
			else if( nret == TCPMAIN_RETURN_WAITINGFOR_NEXT )
				return nret;
			else if( nret == TCPMAIN_RETURN_WAITINGFOR_SENDING )
				;
			else if( nret )
				return nret;
			
			/* �����л����� */
			unpack_len = 0 ;
			memset( & st , 0x00 , sizeof(hello_world) );
			nret = DSCDESERIALIZE_COMPRESS_hello_world( p_accepted_session->tdhb_buffer , & unpack_len , & st ) ;
			if( nret )
				return TCPMAIN_RETURN_ERROR;
			
			/* ���л����� */
			memset( p_accepted_session->tdhb_buffer , 0x00 , sizeof(p_accepted_session->tdhb_buffer) );
			p_accepted_session->tdhb_len = sizeof(p_accepted_session->tdhb_buffer) ;
			nret = DSCSERIALIZE_COMPRESS_hello_world( & st , p_accepted_session->tdhb_buffer , & (p_accepted_session->tdhb_len) ) ;
			if( nret )
				return TCPMAIN_RETURN_ERROR;
			
			return TCPMAIN_RETURN_WAITINGFOR_SENDING;
			
		/* ͨѶ�����¼� */
		case IOMP_ON_SENDING_SOCKET :
			p_accepted_session = (struct AcceptedSession *) p_addr ;
			
			/* �Ƕ�������ͨѶ���� */
			nret = TDHBSendDataWithNonblock( p_accepted_session->sock , PREHEAD_LEN , p_accepted_session->tdhb_buffer , & (p_accepted_session->tdhb_len) , & (p_accepted_session->context) ) ;
			if( nret == TCPMAIN_RETURN_WAITINGFOR_NEXT )
				return nret;
			else if( nret == TCPMAIN_RETURN_WAITINGFOR_RECEIVING )
				/*
				 * ������в���
				 */
				/* ... */
				return nret;
			else
				return nret;
			
		default :
			return TCPMAIN_RETURN_ERROR;
	}
}

