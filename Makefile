tcpLab: ./src/main/main.c ./src/socket/socketCore.c ./src/common/control.c ./src/common/spz.c ./src/socket/tcp/tcp.c ./src/socket/packetCore.c ./src/common/record.c -lm
	gcc -o $@ -Wall $^ -lpthread