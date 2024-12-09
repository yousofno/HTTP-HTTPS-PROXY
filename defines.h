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
static const char CLEANUP [] = "--cleanUp";
static const char INBOUND [] = "--inbound";
static const char OUTBOUND [] = "--outbound";
static const char OK [] = "HTTP/1.1 200 OK\r\n\r\n";
static const char HOST_REGEX [] = R"rx(^(?:(?:https?:\/\/)?(?<host>[a-zA-Z0-9_\-\.]+))?(?:\:(?<port>[0-9]+))?(?<path>\/[^\?\s]*)?)rx";
static const char LOG_PATH [] = "/.svc/collect_logs";
static const char HOST [] = "host";
static const char PORT [] = "port";
static const char PATH [] = "path";
static const char LOCK_SHARED_NAME [] = "proxy_ser_lck";
static const char PROXY_SER_ARR_SHARED_NAME [] = "proxy_ser_arr";
static const char PROXY_SER_ATOMIC_SHARED_NAME [] = "proxy_ser_atomic";
static const char PROXY_SER_CIRCULAR_QUEUE_SHARED_NAME [] = "proxy_ser_circ";
static const char SHARED_MEMORY_OPEN [] = "shm_open";
static const char F_TRUNCATE [] = "ftruncate";
static const char MEMORY_MAP [] = "mmap";
static const char INVALID_ARGUMENTS [] = "Invalid arguments";
static const char INVALID_IP_PORT [] = "Invalid outbound Port or IP";
#endif // DEFINES_H


