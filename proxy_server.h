//
// Created by Yousof Nouri on 8/19/2023 AD.
//
#ifndef UNTITLED17_PROXY_SERVER_H
#define UNTITLED17_PROXY_SERVER_H
#include <QDnsLookup>
#include <QEventLoop>
#include <QHostAddress>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/set_of.hpp>
#include <QRegularExpression>
#include "defines.h"
#include <QPointer>
#include <queue>
#include "circularqueue.h"
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <new>
#include <semaphore.h>
typedef boost::bimap<QTcpSocket *, QTcpSocket* > bm_type;
class proxy_server : public QObject {
  Q_OBJECT
public:
  proxy_server(QString outboundHost = "" ,quint64 outboundPort = 0 , QObject *parent = 0);
  ~proxy_server();
private slots:
  void service(QTcpSocket* scket);
  void socket_err(QAbstractSocket::SocketError error);
  void readFromSocket();
  void writeLog(QString mssg);
  void HTTPRequestParser(const QString& request , QString& path , QString& host , quint64& port , bool& isHttps);
  void* shared_mem_open_qu();
  void* shared_mem_open_atomic_bool();
  sem_t* shared_mem_open_lock();
  void reg(QString& url , QString& host , QString& path , quint64& port);
  signals:
  void startConnection(QTcpSocket* socket);
      void sendLog(QString msg);

private:
  // customer req_port->socket_clinet,socket_remote_server
  bm_type *proxy_list;
  QNetworkProxy *proxy;
CircularQueue* qu;
 quint64 outboundPort;
 QString outboundHost;
  sem_t* lck;
 std::atomic<bool>* at;
};

#endif // UNTITLED17_PROXY_SERVER_H
