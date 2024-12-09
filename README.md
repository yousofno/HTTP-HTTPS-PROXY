
# HTTP/HTTPS Proxy Server

Simple HTTP/HTTPS Proxy Server fully written in Qt



## Usage/Examples
For Using it as a regular proxy server just run the program with --inbound switch with port and ip that this proxy server should listens to.
```bash
./proxy_server --inbound 192.168.136.100:80
```
For sending traffic to a specific host add --outbound switch.
```bash
./proxy_server --inbound 192.168.136.100:80 --outbound google.com:443
```
For terminating all proccess Use
```bash
killall proxy_server
```



## Tech Stack

**Server:** C++, Qt


## Author

- [@yousofno](https://www.github.com/yousofno)


## Appendix

https://github.com/yousofno/HTTP-HTTPS-PROXY/

