tcpLab: tcp_ip_tester.c ./includes/rsock_recv.c ./includes/tcpHelper.c ./includes/rsockHelper.c ./includes/spz.c ./includes/post.c ./includes/packet.c -lm
	gcc -o $@ -Wall $^ -lpthread

tcpLabDebug: tcp_ip_tester.c ./includes/rsock_recv.c ./includes/tcpHelper.c ./includes/rsockHelper.c ./includes/spz.c ./includes/post.c ./includes/packet.c -lm
	gcc -o $@ $^ -g -lpthread
