tcpdaemon
=========

# 0.���ٿ�ʼ #
��ʦ����С��һ����������ʵ��һ��TCP����������������������ڻ����κν��յ���ͨѶ���ݡ�С�����������ڿ�Դ�й���Ŀ�����ѵ��˿�Դ��tcpdaemon���������������������������װ��tcpdaemon��Ȼ��д��һ��C�����ļ�test_callback_echo.c

    $ vi test_callback_echo.c
	#include "tcpdaemon.h"
	
	_WINDLL_FUNC int tcpmain( struct TcpdaemonServerEnvironment *p_env , int sock , void *p_addr )
	{
		char	buffer[ 4096 ] ;
		int	len ;
		
		len = recv( sock , buffer , sizeof(buffer) , 0 ) ;
		if( len == 0 )
			return TCPMAIN_RETURN_CLOSE;
		else if( len < 0 )
			return TCPMAIN_RETURN_ERROR;
        
		len = send( sock , buffer , len , 0 ) ;
		if( len < 0 )
			return TCPMAIN_RETURN_ERROR;
		
		return TCPMAIN_RETURN_WAITINGFOR_NEXT;
	}

���������ӳɶ�̬��test_callback_echo.so�������tcpdaemonֱ�ӹҽ�ִ��

    $ tcpdaemon -m IF -l 0:9527 -s test_callback_echo.so -c 10 --tcp-nodelay --logfile $HOME/log/test_callback_echo.log

OK���ܹ���������ӣ�Բ�������ʦ��ҵ����ʦ˵���̫���ˣ�С������Ҹĳ���Apache�����Leader-Follow�����ģ�ͣ�С��˵û���⣬���������������`-m IF`�ĳ���`-m LF`���ٴ�ִ�У������ʦҪ���ܹ����������ӡ���ʦ������ô��ô��͸ĺ��ˣ�С��˵ȫ����Դ��Ŀtcpdaemon���˴�æ�� ^_^

# 1.���� #
tcpdaemon��һ��TCPͨѶ�����ƽ̨/�⣬����װ���ڶೣ������˽���/�̹߳����TCP���ӹ���ģ�ͣ�Forking��Leader-Follow��IO-Multiplex��WindowsThreads Leader-Follow����ʹ����ֻ�����TCPͨѶ�����շ���Ӧ���߼�������ܿ��ٹ�����������TCPӦ�÷�������

| ����ģ�� | ģ��˵�� |
| ---- | ---- |
| Forking | ���������ػ���ÿ��һ��TCP�����ӽ����󣬽���֮�������ӽ��̽���ص�����tcpmain����֮��һ�����Ӷ�Ӧһ���ӽ��̣����������ڣ� |
| Leader-Follow | �����̹�����̣�Ԥ�ȴ���һ���ӽ��̣����������ڣ���������쳣�������ӽ��̵ȴ�ѭ������TCP�����ӵ��ûص�����tcpmain����֮ |
| IO-Multiplex | ���������ػ���IO��·���õȴ�TCP�����ӽ����¼���TCP���ݵ����¼���TCP���ݿ�д�¼������ûص�����tcpmain����֮ |
| WindowsThreads Leader-Follow | ͬLeader-Follow����������Ԥ�ȴ���һ�����̶߳����ӽ��� |

tcpdaemon�ṩ��������ʹ���ߴ���Խӷ�ʽ��(ע�⣺.exeֻ��Ϊ��˵���Լ��ǿ�ִ���ļ�����UNIX/Linux�п�ִ���ļ�һ��û����չ��)

| ����ģʽ | ���ӹ�ϵ | ˵�� |
| ---- | ----- | -- |
| �ص�ģʽ | tcpdaemon.exe+user.so(tcpmain) | ��ִ�г���tcpdaemonͨ�����������в����ҽ��û���̬�⣬��ö�̬���к���tcpmainָ�롣������TCP���Ӻ� �� IO��·����ģʽ�µ��ɶ���д�¼�����ʱ ���ûص�����tcpmain |
| ����ģʽ | user.exe(main,tcpmain)+libtcpdaemon.a(tcpdaemon) | �û���ִ�г���user.exe��ʽ���ӿ�libtcpdaemon.a���û�����main(user.exe)��ʼ��tcpdaemon�����ṹ�壬�����ûص�����tcpmain�����ú���tcpdaemon(libtcpdaemon.so)��������TCP���Ӻ� �� IO��·����ģʽ�µ��ɶ���д�¼�����ʱ ���ûص�����tcpmain |
| ����+�ص�ģʽ | user.exe(main)+libtcpdaemon.a(tcpdaemon) + user.so(tcpmain) | ͬ�ϣ����������û�����main��ֱ�����ûص�����tcpmain������user.so�ļ���������tcpdaemon����ҽӶ�̬��user.so����ú���tcpmainָ�� |

һ�������£�ʹ���߲��ûص�ģʽ���ɣ�ֻҪ��дһ����̬��user.so���ں��ص�����tcpmain������ִ�г���tcpdaemon�ҽ���ȥ���С����ʹ�����붩��һЩ�Զ��崦�����ʼ�����������Բ�������ģʽ��ʵ�ֺ���main����Զ���������ݸ�tcpdaemon��͸��tcpmain�������ʵ������ʱѡ��ص�����tcpmain����Բ�������+�ص�ģʽ��

# 2.���밲װ #
��Linux����ϵͳΪ�������ص�����Դ�밲װ��tcpdaemon-x.y.z.tar.gz��ĳĿ¼����ѹ֮

    $ tar xvzf tcpdaemon-x.y.z.tar.gz
    ...
    $ cd tcpdaemon
    $ cd src
    $ make -f makefile.Linux install
    rm -f LOGC.o
    rm -f tcpdaemon_lib.o
    rm -f tcpdaemon_main.o
    rm -f tcpdaemon
    rm -f libtcpdaemon.a
    gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include  -c tcpdaemon_lib.c
    gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include  -c LOGC.c
    ar rv libtcpdaemon.a tcpdaemon_lib.o LOGC.o
    ar: ���ڴ��� libtcpdaemon.a
    a - tcpdaemon_lib.o
    a - LOGC.o
    gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include  -c tcpdaemon_main.c
    gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing  -o tcpdaemon tcpdaemon_main.o tcpdaemon_lib.o LOGC.o -L. -L/home/calvin/lib -lpthread -ldl 
    cp -rf tcpdaemon /home/calvin/bin/
    cp -rf libtcpdaemon.a /home/calvin/lib/
    cp -rf tcpdaemon.h /home/calvin/include/tcpdaemon/

���Կ�����ִ�г���tcpdaemon��װ��$HOME/bin����̬��libtcpdaemon.a��װ��$HOME/lib�������õ�ͷ�ļ���װ��$HOME/include/tcpdaemon��

# 3.ʹ��ʾ�� #
## 3.1.����ģ��Forking������ģʽ1������HTTP������Ȼ����HTTP��Ӧ���� ##
ʹ����ֻ���дһ������tcpmain��ʵ��ͬ���Ľ���HTTP������Ȼ����HTTP��Ӧ���Ļ�ȥ

    $ vi test_callback_http_echo.c
    #include "tcpdaemon.h"
    
    _WINDLL_FUNC int tcpmain( struct TcpdaemonServerEnvironment *p_env , int sock , void *p_addr )
    {
    	char	http_buffer[ 4096 + 1 ] ;
    	long	http_len ;
    	long	len ;
    	
    	/* ����HTTP���� */
    	memset( http_buffer , 0x00 , sizeof(http_buffer) );
    	http_len = 0 ;
    	while( sizeof(http_buffer)-1 - http_len > 0 )
    	{
    		len = RECV( sock , http_buffer + http_len , sizeof(http_buffer)-1 - http_len , 0 ) ;
    		if( len == 0 )
    			return TCPMAIN_RETURN_CLOSE;
    		if( len == -1 )
    			return TCPMAIN_RETURN_ERROR;
    		if( strstr( http_buffer , "\r\n\r\n" ) )
    			break;
    		http_len += len ;
    	}
    	if( sizeof(http_buffer)-1 - http_len <= 0 )
    	{
    		return TCPMAIN_RETURN_ERROR;
    	}
    	
    	/* ����HTTP��Ӧ */
    	memset( http_buffer , 0x00 , sizeof(http_buffer) );
    	http_len = 0 ;
    	http_len = sprintf( http_buffer , "HTTP/1.0 200 OK\r\nContent-length: 17\r\n\r\nHello Tcpdaemon\r\n" ) ;
    	SEND( sock , http_buffer , http_len , 0 );
    	
    	return TCPMAIN_RETURN_CLOSE;
    }

�������ӳ�test_callback_http_echo.so

    $ gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include/tcpdaemon  -c test_callback_http_echo.c
    $ gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -shared -o test_callback_http_echo.so test_callback_http_echo.o -L. -L/home/calvin/lib -lpthread -ldl 
    
��tcpdaemonֱ�ӹҽӼ���

    $ tcpdaemon -m IF -l 0:9527 -s test_callback_http_echo.so -c 10 --tcp-nodelay --logfile $HOME/log/test_callback_http_echo.log --loglevel-debug

��ִ�г���tcpdaemon���������в������Բ���������ִ�ж��õ�

    $ tcpdaemon
    USAGE : tcpdaemon -m IF -l ip:port -s so_pathfilename [ -c max_process_count ]
                      -m LF -l ip:port -s so_pathfilename -c process_count [ -n max_requests_per_process ]
            other options :
                      -v
                      [ --daemon-level ]
                      [ --work-path work_path ]
                      [ --work-user work_user ]

ִ�к����$HOME/log�¿��Կ���tcpdaemon����־��

ͨ��curl����������

    $ curl "http://localhost:9527/"
    Hello Tcpdaemon

���Գɹ���

**���д�����Դ�밲װ����testĿ¼���¿����ҵ�**

## 3.2.����ģ��IO-Multiplex������ģʽ2���Ƕ����Ľ���HTTP������Ȼ����HTTP��Ӧ���� ##
ʹ���߱�дһ������main

    $ test_main_IOMP.c
    #include "tcpdaemon.h"
    
    extern int tcpmain( struct TcpdaemonServerEnvironment *p_env , int sock , void *p_addr );
    
    int main()
    {
    	struct TcpdaemonEntryParameter	ep ;
    	
    	memset( & ep , 0x00 , sizeof(struct TcpdaemonEntryParameter) );
    	
    	snprintf( ep.log_pathfilename , sizeof(ep.log_pathfilename) , "%s/log/test_main_IOMP.log" , getenv("HOME") );
    	ep.log_level = LOGLEVEL_DEBUG ;
    	
    	strcpy( ep.server_model , "IOMP" );
        ep.timeout_seconds = 60 ;
    	strcpy( ep.ip , "0" );
    	ep.port = 9527 ;
        ep.tcp_nodelay = 1 ;
    	
    	ep.process_count = 1 ;
    	
    	ep.pfunc_tcpmain = & tcpmain ;
    	ep.param_tcpmain = NULL ;
    	
    	return -tcpdaemon( & ep );
    }

�ṹ��TcpdaemonEntryParameter���г�Ա˵����tcpdaemon.h������ҵ�

    struct TcpdaemonEntryParameter
    {
    	int		daemon_level ;	/* �Ƿ�ת��Ϊ�ػ����� 1:ת�� 0:��ת����ȱʡ�� */
    	
    	char		log_pathfilename[ 256 + 1 ] ;	/* ��־����ļ��������������������׼����� */
    	int		log_level ;	/* ��־�ȼ� */
    	
    	char		server_model[ 10 + 1 ] ;	/* TCP���ӹ���ģ��
    							LF:�쵼��-׷����Ԥ�������̳�ģ�� for UNIX,Linux
    							IF:��ʱ��������ģ�� for UNIX,Linux
    							WIN-TLF:�쵼��-׷����Ԥ�����̳߳�ģ�� for win32
    							*/
    	int		process_count ;	/* ��Ϊ�쵼��-׷����Ԥ�������̳�ģ��ʱΪ�������̳ؽ�����������Ϊ��ʱ��������ģ��ʱΪ����ӽ�����������ΪIO��·����ģ��ʱΪ�������̳ؽ������� */
    	int		max_requests_per_process ;	/* ��Ϊ�쵼��-׷����Ԥ�������̳�ģ��ʱΪ�����������������Ӧ�ô��� */
    	char		ip[ 20 + 1 ] ;	/* ��������IP */
    	int		port ;	/* ��������PORT */
    	char		so_pathfilename[ 256 + 1 ] ;	/* �þ���·�������·������Ӧ�ö�̬���ļ��� */
    	
    	char		work_user[ 64 + 1 ] ;	/* �л�Ϊ�����û����С���ѡ */
    	char		work_path[ 256 + 1 ] ;	/* �л���ָ��Ŀ¼���С���ѡ */
    	
    	func_tcpmain	*pfunc_tcpmain ;	/* ����������ģʽʱ��ָ���TCP���ӽ���Ӧ����ں���ָ�� */
    	void		*param_tcpmain ;	/* ����������ģʽʱ��ָ���TCP���ӽ���Ӧ����ں����Ĳ���ָ�롣�ر�ע�⣺�Լ���֤�̰߳�ȫ */
    	
    	int		tcp_nodelay ;	/* ����TCP_NODELAYѡ�� 1:���� 0:�����ã�ȱʡ������ѡ */
    	int		tcp_linger ;	/* ����TCP_LINGERѡ�� >=1:���ò����óɲ���ֵ 0:�����ã�ȱʡ������ѡ */
    	
        int     timeout_seconds ; /* ��ʱʱ�䣬��λ���룻Ŀǰֻ��IO-Multiplexģ����Ч */
        
    	/* ����Ϊ�ڲ�ʹ�� */
    	int		install_winservice ;
    	int		uninstall_winservice ;
    } ;


ʹ�����ٱ�дһ������tcpmain��ʵ�ַǶ����Ľ���HTTP������Ȼ����HTTP��Ӧ���Ļ�ȥ

    $ vi test_callback_http_echo_nonblock.c
    #include "tcpdaemon.h"
    
    struct AcceptedSession
    {
    	int		sock ; /* socket������ */
    	struct sockaddr	addr ; /* socket��ַ */
    	
    	char	http_buffer[ 4096 + 1 ] ; /* HTTP�շ������� */
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
    
�����test_main_IOMP

    gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include/tcpdaemon  -c test_main_IOMP.c
    gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include/tcpdaemon  -c test_callback_http_echo_nonblock.c
    gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing  -o test_main_IOMP test_main_IOMP.o test_callback_http_echo_nonblock.o -L. -L/home/calvin/lib -lpthread -ldl  -ltcpdaemon

ִ��test_main_IOMP

    $ ./test_main_IOMP

ִ�к����$HOME/log�¿��Կ���test_main_IOMP����־��

ͨ��curl����������

    $ curl "http://localhost:9527/"
    Hello Tcpdaemon

���Գɹ���

**���д�����Դ�밲װ����testĿ¼���¿����ҵ�**

# 4.�ο� #
## 4.1.�ص�����tcpmain ##
tcpdaemon���ں���ʱ���ص��û���ָ������tcpmain��

������ģ��ΪForking��Leader-Follow��WindowsThreads Leader-Followʱ���ص�����tcpmain�����ӱ����ܺ���á�����TCPMAIN_RETURN_CLOSE��TCPMAIN_RETURN_ERROR�����ر�ͨѶ���ӡ�

������ģ��ΪIO-Multiplexʱ���ص�����tcpmain��ͨѶ���ӱ�����ʱ��ͨѶ���ӱ��ر�ʱ��ͨѶ���ݿɽ���/����ʱ���ã���Ϊ�ǷǶ�����������TCPMAIN_RETURN_CLOSE��TCPMAIN_RETURN_ERROR��ر�ͨѶ���ӣ�����TCPMAIN_RETURN_WAITINGFOR_RECEIVING���л��ȴ��ɶ��¼�������TCPMAIN_RETURN_WAITINGFOR_SENDING���л��ȴ���д�¼�������TCPMAIN_RETURN_WAITINGFOR_NEXT������ȴ������¼�����ע�⣺ͨѶ���ӱ�����ʱ��tcpmain��������TDSetIoMultiplexDataPtr����ͨѶ���ݻỰ�ṹ��

## 4.2.TcpdaemonServerEnvironment���������� ##

| ������ | TDGetTcpmainParameter |
| --:|:-- |
| ����ԭ�� | void *TDGetTcpmainParameter( struct TcpdaemonServerEnvironment *p_env ); |
| ������� | struct TcpdaemonServerEnvironment *p_env tcpdaemon�����ṹָ�� |
| ����ֵ | ����tcpmainʱ����TcpdaemonEntryParameter��param_tcpmain��ַ |

| ������ | TDGetListenSocket |
| --:|:-- |
| ����ԭ�� | int TDGetListenSocket( struct TcpdaemonServerEnvironment *p_env ); |
| ������� | struct TcpdaemonServerEnvironment *p_env tcpdaemon�����ṹָ�� |
| ����ֵ | �����˿������� |

| ������ | TDGetListenSocketPtr |
| --:|:-- |
| ����ԭ�� | int *TDGetListenSocketPtr( struct TcpdaemonServerEnvironment *p_env ); |
| ������� | struct TcpdaemonServerEnvironment *p_env tcpdaemon�����ṹָ�� |
| ����ֵ | �����˿������ֵĵ�ַ |

| ������ | TDGetListenAddress |
| --:|:-- |
| ����ԭ�� | struct sockaddr_in TDGetListenAddress( struct TcpdaemonServerEnvironment *p_env ); |
| ������� | struct TcpdaemonServerEnvironment *p_env tcpdaemon�����ṹָ�� |
| ����ֵ | �����˿������ַ |

| ������ | TDGetListenAddressPtr |
| --:|:-- |
| ����ԭ�� | struct sockaddr_in *TDGetListenAddressPtr( struct TcpdaemonServerEnvironment *p_env ); |
| ������� | struct TcpdaemonServerEnvironment *p_env tcpdaemon�����ṹָ�� |
| ����ֵ | �����˿������ַ�ĵ�ַ |

| ������ | TDGetProcessCount |
| --:|:-- |
| ����ԭ�� | int TDGetProcessCount( struct TcpdaemonServerEnvironment *p_env ); |
| ������� | struct TcpdaemonServerEnvironment *p_env tcpdaemon�����ṹָ�� |
| ����ֵ | ������󲢷��Ȼ�̬���̳ؽ������� |

| ������ | TDGetEpollArrayBase |
| --:|:-- |
| ����ԭ�� | int *TDGetEpollArrayBase( struct TcpdaemonServerEnvironment *p_env ); |
| ������� | struct TcpdaemonServerEnvironment *p_env tcpdaemon�����ṹָ�� |
| ����ֵ | IO��·����epoll�����һ��Ԫ�صĵ�ַ |

| ������ | TDGetThisEpoll |
| --:|:-- |
| ����ԭ�� | int TDGetThisEpoll( struct TcpdaemonServerEnvironment *p_env ); |
| ������� | struct TcpdaemonServerEnvironment *p_env tcpdaemon�����ṹָ�� |
| ����ֵ | ��ǰIO��·����epoll������ |

| ������ | TDGetIoMultiplexEvent |
| --:|:-- |
| ����ԭ�� | int TDGetIoMultiplexEvent( struct TcpdaemonServerEnvironment *p_env ); |
| ������� | struct TcpdaemonServerEnvironment *p_env tcpdaemon�����ṹָ�� |
| ����ֵ | ��ǰIO��·����epoll�¼� IOMP_ON_ACCEPTING_SOCKET:�����������¼� IOMP_ON_CLOSING_SOCKET:�ر������¼� IOMP_ON_RECEIVING_SOCKET:����ͨѶ�����¼� IOMP_ON_SENDING_SOCKET:����ͨѶ�����¼� |

# 5.�ܽ� #

tcpdaemon�ṩ�˶��ַ���ģ�ͺ�����ģʽ��ּ��Э��ʹ���߿��ٹ���TCPӦ�÷��������������ʹ�ñ��˵���һ����Դ��Ŀ [HTTP������fasterhttp](http://git.oschina.net/calvinwilliams/fasterhttp) �԰������ڴ��빹����һ��������Web������������һ��������Ӧ�ð����ɲ��ı��˵���һ����Դ��Ŀ [�ֲ�ʽ������](http://git.oschina.net/calvinwilliams/coconut) ������tcpdaemon�����Ӧ�ô���������һ�롣

tcpdaemonԴ���й��� [��Դ�й�����](http://git.oschina.net/calvinwilliams/tcpdaemon)����Ҳ����ͨ�� [����](calvinwilliams@163.com) ��ϵ������

�ܰ����������ҵ����� ^_^
