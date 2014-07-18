#include "tcpdaemon.h"

/* ���ػ�ģʽ���Գ��� */

_WINDLL_FUNC func_tcpmain tcpmain ;
int tcpmain( void *param_tcpmain , int sock , struct sockaddr *addr )
{
#if 0
	write( sock , "hello tcpdaemon\n" , 16 );
	return 0;
#endif
#if 1
	char	http_buffer[ 4096 + 1 ] ;
	long	http_len ;
	long	len ;
	
	memset( http_buffer , 0x00 , sizeof(http_buffer) );
	http_len = 0 ;
	
	while( sizeof(http_buffer)-1 - http_len > 0 )
	{
		len = RECV( sock , http_buffer + http_len , sizeof(http_buffer)-1 - http_len , 0 ) ;
		if( len == -1 || len == 0 )
			return 0;
		if( strstr( http_buffer , "\r\n\r\n" ) )
			break;
		http_len += len ;
	}
	if( sizeof(http_buffer)-1 - http_len <= 0 )
	{
		return -1;
	}
	
	memset( http_buffer , 0x00 , sizeof(http_buffer) );
	http_len = 0 ;
	
	http_len = sprintf( http_buffer , "HTTP/1.0 200 OK\r\n\r\n" ) ;
	SEND( sock , http_buffer , http_len , 0 );
	
	return 0;
#endif
}

