tcpLab: ./src/main/main.c ./src/common/definitions.c ./src/socket/socketCore.c ./src/common/control.c ./src/common/spz.c ./src/socket/tcp/tcp.c ./src/socket/packetCore.c ./src/socket/serverThread.c ./src/common/record.c -lm
	gcc -o ./bin/$@ -Wall $^ -lpthread



# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

.PHONY: install
install: tcpLab
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp ./bin/$< $(DESTDIR)$(PREFIX)/bin/

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/tcpLab

