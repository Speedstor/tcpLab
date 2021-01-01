tcpLab: main.c ./includes/rsockHelper.c ./control.c ./includes/spz.c -lm
	gcc -o $@ -Wall $^ -lpthread