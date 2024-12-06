#ifndef DEFINES_H
#define DEFINES_H
#define HTTP_PORT 80
#define HTTPS_PORT 443
#define TIMEOUT 500
static const char CONNECT_METHOD [] = "CONNECT";
static const char GET_METHOD [] = "GET";
static const char POST_METHOD [] = "POST";
static const char PUT_METHOD [] = "PUT";
static const char DELETE_METHOD [] = "DELETE";
static const char BAD_GATEWAY [] = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
static const char CRLF [] = "\r\n";
static const char LF [] = "\n";
static const char INBOUND [] = "--inbound";
static const char OUTBOUND [] = "--outbound";
static const char OK [] = "HTTP/1.1 200 OK\r\n\r\n";
static const char HOST_REGEX [] = R"rx(^(?:(?:https?:\/\/)?(?<host>[a-zA-Z0-9_\-\.]+))(?:\:(?<port>[0-9]+))?(?<path>\/[^\?\s]*)?)rx";
static const char IP_PORT_REGEX [] = R"(([\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}):([\d]{1,5}))";
static const char LOG_PATH [] = "/.svc/collect_logs";
static const char HOST [] = "host";
static const char PORT [] = "port";
static const char PATH [] = "path";
#endif // DEFINES_H


