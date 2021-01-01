tcpLab: ./src/main/main.c ./src/socket/socketCore.c ./src/common/control.c ./src/common/spz.c -lm
	gcc -o $@ -Wall $^ -lpthread