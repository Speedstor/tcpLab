tcpLab: tcp_ip_tester.c ./includes/rsock_recv.c ./includes/tcpHelper.c ./includes/rsockHelper.c ./includes/spz.c ./includes/post.c -lcurl -lm
	gcc -o $@ -Wall $^ -lpthread

tcpLabDebug: tcp_ip_tester.c ./includes/rsock_recv.c ./includes/tcpHelper.c ./includes/rsockHelper.c ./includes/spz.c ./includes/post.c -lcurl -lm
	gcc -o $@ $^ -lpthread

tcpLabRecieve: tcp_ip_tester.c ./includes/rsock_recv.c ./includes/tcpHelper.c ./includes/rsockHelper.c ./includes/spz.c ./includes/post.c -lcurl -lm
	gcc -o $@ $^ -lpthread