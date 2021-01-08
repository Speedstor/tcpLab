# tcpLab 
A `C` program that send and receives tcp packets from raw sockets. It can be used for a reference for making custom network protocols given the structure. In addition, the main purpose of this program is to have detailed time recordings of individual packets w/ `-r`.

"On a side note", this program only runs on **linux** and linux only, :( I'm sorry ;(.
<br/><br/>

## Before you start :hand:
removing automatic reset packets is needed. To do that on ubuntu, just type this into the terminal.
```bash
$ sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP
```
## Quick Start
> $ sudo ./bin/tcpLab -i \<network-interface\> -\<S / R\>
### Set Network Interface
The default network interface (*if*) is `wlp1s0`. And if your network interface is different, you have to specify it yourself. To check your *if*, you can do `ifconfig`. And if you don't know which one it is, you can do `iwconfig` instead, but you would need **net-tools**.
```bash
$ ifconfig
```

> enp0s31f6: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500 <br/>
> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;ether 48:2a:e3:26:90:63  txqueuelen 1000  (Ethernet)
>
> lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536 <br/>
> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;inet 127.0.0.1  netmask 255.0.0.0
> 
> **wlp1s0**: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500<br/>
> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;inet <your.ip.is.here>  netmask 255.255.255.0  broadcast 192.168.1.255

~or~

```bash
$ iwconfig
```
> lo        no wireless extensions.
> 
> **wlp1s0**  IEEE 802.11  ESSID:"WIFI-NAME"  
> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Mode:Managed  Frequency:2.452 GHz  Access Point: 50:6A:03:A5:6B:8C   
> 
> enp0s31f6  no wireless extensions.
<br/><br/>

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
### Install / Uninstall
I don't know why you would want to install this, but you are free to do whatever you want :)
```bash
$ sudo make install

$ sudo make uninstall
```
##### (There are no dependencies)
<br/><br/>

###### Made by [speedstor](https://speedstor.net)