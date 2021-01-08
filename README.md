# tcpLab 
A `C` program that send and receives tcp packets from raw sockets. It can be used for a reference for making custom network protocols given the structure. In addition, the main purpose of this program is to have detailed time recordings of individual packets w/ `-r`.

"On a side note", this program only runs on **linux** and linux only, :( I'm sorry ;(.
<br/><br/><br/><br/>

## Before you start :hand:
removing automatic reset packets is needed. To do that on ubuntu, just type this into the terminal.
```bash
$ sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP
```
<br/><br/><br/>


## Quick Start
```bash
$ sudo ./bin/tcpLab -i \<network-interface\> -\<S / R\>
```
<br/><br/><br/>


### Set Network Interface
The default network interface (*if*) is `wlp1s0`. And if your network interface is different, you have to specify it yourself. To check your *if*, you can do `ifconfig`. And if you don't know which one it is, you can do `iwconfig` instead, but you would need **net-tools**.
```bash
$ ifconfig
```
> **wlp1s0**: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500<br/>
> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;inet <your.ip.is.here>  netmask 255.255.255.0  broadcast 192.168.1.255

~or~

```bash
$ iwconfig
```
> **wlp1s0**  IEEE 802.11  ESSID:"WIFI-NAME"  
> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Mode:Managed  Frequency:2.452 GHz  Access Point: 50:6A:03:A5:6B:8C   

<br/><br/><br/>

### Options
> all optional (except sometimes `-i`)
```
 -i     network interface card that is going to be used
 -s     source ip IPv4 address
 -d     destination ip IPv4 address (default: 127.0.0.1)
 -P     destination port number (default: 8900)
 -S/R/B select send or receive mode, B is for both (default: -s)
 -r     if record packets to database
 -M     multithread: unifed receive (have performance gain) [not implemented]
 -m     message, payload of the data that going to be sent
 -p     transport network protocol to send data
 -v     verbose, choose if there is progress bars
 -j     json format for recording, choose between json and csv 
 -A/D   animation/dull, choose if there's spinning loading chars
 -h/?   show help
```

<br/><br/><br/>

### Install / Uninstall
I don't know why you would want to install this, but you are free to do whatever you want :)
```bash
$ sudo make install

$ sudo make uninstall
```
<br/>

###### Made by [speedstor](https://speedstor.net)