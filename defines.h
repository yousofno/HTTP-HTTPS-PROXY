#ifndef DEFINES_H
#define DEFINES_H

static const char CONNECT_METHOD [] = "CONNECT";
static const char GET_METHOD [] = "GET";
static const char POST_METHOD [] = "POST";
static const char PUT_METHOD [] = "PUT";
static const char DELETE_METHOD [] = "DELETE";
static const char BAD_GATEWAY [] = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
static const char CRLF [] = "\r\n";
static const char OK [] = "HTTP/1.1 200 OK\r\n\r\n";
static const char HOST_REGEX [] = R"rx(/^(?:(?'scheme'http[s]?):\/\/)?(?'host'[a-z0-9_\-.]+)(?:\:(?'port'[0-9]+))?(?'path'\/[^\?\s]*)?(?:\?(?'query'\S+))?)rx";
static const char IP_PORT_REGEX [] = R"(([\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}):([\d]{1,5}))";
static const char LOG_PATH [] = "/.svc/collectLogs";

#endif // DEFINES_H
