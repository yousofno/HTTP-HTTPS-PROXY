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
#include <Qthread>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/set_of.hpp>
#include <QRegularExpression>
#include "defines.h"
#include <QPointer>
typedef boost::bimap<QTcpSocket *, QTcpSocket* > bm_type;
class proxy_server : public QObject {
  Q_OBJECT
public:
  proxy_server(QObject *parent);
  bool start_proxy_server(QHostAddress address , quint64 port);
  ~proxy_server();
private slots:
  void service();
  void socket_err(QAbstractSocket::SocketError);
  void readFromSocket();

private:
  // customer req_port->socket_clinet,socket_remote_server
  bm_type *proxy_list;
  QTcpServer *ser;
  QNetworkProxy *proxy;
};

#endif // UNTITLED17_PROXY_SERVER_H
