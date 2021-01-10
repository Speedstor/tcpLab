# tcpLab 
A `C` program that send and receives tcp packets from raw sockets. It can be used for a reference for making custom network protocols given the structure. In addition, the main purpose of this program is to have detailed **time recordings** of individual packets w/ `-r`.

*"On a side note"*, this program only runs on **linux** and linux only, I'm sorry :(.
###### One of the simplest tcp client/server in raw sockets
<br/><br/><br/>

## :stop_sign: Before you start
removing automatic reset packets is needed. To do that on ubuntu, just type this into the terminal.
```bash
$ sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP
```
<br/>

## :green_circle: Quick Start
```bash
$ sudo ./bin/tcpLab -i <network-interface> -<S/R>
```

### Set Network Interface
The default network interface (*if*) is `wlp1s0`. And if your network interface is different, you have to specify it yourself. To check your *if*, you can do `ifconfig`. And if you don't know which one it is, you can do `iwconfig` instead, but you would need **net-tools**.
```bash
$ ifconfig

~or~

$ iwconfig
```

### Options
> all optional (except sometimes `-i`)
```
 -i     network interface card that is going to be used
 -s     source ip IPv4 address
 -d     destination ip IPv4 address (default: 127.0.0.1)
 -P     destination port number (default: 8900)
 -S/R/B select send or receive mode, B is for both (default: -s)
 -r     if record packets to database
  -c    checksum type (1: tcp, 2: crc, 3: none), automatically changes protocol! (tcp: 6, crc: 206, none: 216)
 -M     multithread: unifed receive (have performance gain) [not implemented]
 -m     message, payload of the data that going to be sent
 -p     transport network protocol to send data
 -v     verbose, choose if there is progress bars
 -j     json format for recording, choose between json and csv 
 -A/D   animation/dull, choose if there's spinning loading chars
 -h/?   show help
```

<br/>

### Install / Uninstall
I don't know why you would want to install this, but you are free to do whatever you want :smiley:
```bash
$ sudo make install

$ sudo make uninstall
```
<br/>

### Documentation
##### DataBase Record Packets
There are two formats to recording packets, determined by `-j` for **json** and *none* for **csv**. Both format saves into a txt file unfortunately tho :(. The txt files are called *tcpDB_sent.txt* and &*tcpDB_receive.txt*


| tableName      | ifAuto            | seq       | data                      | packet                                          | time                    |
| -------------- | ----------------- | --------- | ------------------------- | ----------------------------------------------- | ----------------------- |
| packet_sent    | *if auto written* | tcp_seq   | (*when available*) in HEX | HE XO FW HO LE PA CK ET                         | 2021-01-07 23:04:48.487 |
| packet_receive | *if auto written* | tcp_seq   | (*when available*) in HEX | HE XO FW HO LE PA CK ET                         | 2021-01-07 23:04:48.487 |
| sample         | true              | 592577640 |                           | 45 00 00 28 0e a4 00 00 0b 06 c7 ac 4c a9 ca cb | 2021-01-07 23:04:48.487 |


<br/>

###### Made by [speedstor](https://speedstor.net)
