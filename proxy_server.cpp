//
// Created by Yousof on 2/1/2024 AD.
//

#include "proxy_server.h"
proxy_server::proxy_server(QObject* parent): QObject(parent) {
    proxy_list = new bm_type();
    ser = new QTcpServer();
    proxy = new QNetworkProxy();
    proxy->setType(QNetworkProxy::NoProxy);
    connect(ser , SIGNAL(newConnection()) , this , SLOT(service()));

}
bool proxy_server::start_proxy_server(QHostAddress address , quint64 port) {
    if(ser->listen(address, port)){
        return  true;
    }
    return false;
}
proxy_server::~proxy_server() {
    delete ser;
    delete proxy;
    for(auto index  = proxy_list->begin(); index != proxy_list->end();index++){
        index->left->deleteLater();
        index->right->deleteLater();
    }
}

void proxy_server::service(){
    if(ser->hasPendingConnections()){
        QTcpSocket *my_soc = ser->nextPendingConnection();
        connect(my_soc , SIGNAL(readyRead()) , this , SLOT(readFromSocket()));
        connect(my_soc , SIGNAL(error(QAbstractSocket::SocketError)) , this , SLOT(socket_err(QAbstractSocket::SocketError)));
    }
}


void proxy_server::readFromSocket(){
    //reading data
    //find if it is a CONNECT http method
    QTcpSocket* incomming_socket = reinterpret_cast<QTcpSocket*>(sender());
    QByteArray answer = incomming_socket->readAll();

    if (this->proxy_list->left.find(incomming_socket) != this->proxy_list->left.end()) {
        if(this->proxy_list->left.at(incomming_socket)->state() == QAbstractSocket::ConnectedState){
            this->proxy_list->left.at(incomming_socket)->write(answer);
        }
        return;
    }


    if (this->proxy_list->right.find(incomming_socket) != this->proxy_list->right.end()) {
        if(this->proxy_list->right.at(incomming_socket)->state() == QAbstractSocket::ConnectedState){
            this->proxy_list->right.at(incomming_socket)->write(answer);
        }
        return;
    }


    QStringList req_list = QString(answer).split(CRLF);


    //find ip addr of destination
    QString host  = "";
    qint64 port  = 0;
    bool isHttps = false;
    if(req_list.first().contains(CONNECT_METHOD)){
        isHttps = true;
        host = req_list.first().split(" ")[1].split(":")[0];
        port = req_list.first().split(" ")[1].split(":")[1].toInt();

    }else if(req_list.first().contains(GET_METHOD) || req_list.first().contains(POST_METHOD) || req_list.first().contains(PUT_METHOD)
               || req_list.first().contains(DELETE_METHOD)){
        QRegularExpression hostRegex(HOST_REGEX, QRegularExpression::MultilineOption);
        QRegularExpressionMatch match = hostRegex.match(req_list[1]);
        if (match.hasMatch()) {
            isHttps = false;
            host = match.captured(1);
            port = 80;
        }
    }

    if(host.isEmpty() || port == 0){
        incomming_socket->close();
        incomming_socket->deleteLater();
        incomming_socket = nullptr;
        return;
    }

    QTcpSocket* remote_socket = new QTcpSocket();
    remote_socket->setProxy(*this->proxy);
    connect(remote_socket , SIGNAL(error(QAbstractSocket::SocketError)) , this , SLOT(socket_err(QAbstractSocket::SocketError)));
    connect(remote_socket , SIGNAL(readyRead()) , this , SLOT(readFromSocket()));
    this->proxy_list->insert(bm_type::value_type(incomming_socket, remote_socket));
    QObject::connect(remote_socket , &QTcpSocket::connected , this ,[this ,isHttps,answer, incomming_socket]() {
        if(this->proxy_list->left.find(incomming_socket) == this->proxy_list->left.end()){
            return;
        }

        if(isHttps){
            if(incomming_socket != nullptr){
            if(incomming_socket->state() == QAbstractSocket::ConnectedState){
                incomming_socket->write(OK);
            }
        }
        }else{
            this->proxy_list->left.at(incomming_socket)->write(answer);
        }


    });
    remote_socket->connectToHost(host , port);
    return;
}



void proxy_server::socket_err(QAbstractSocket::SocketError) {
    QTcpSocket *sock = reinterpret_cast<QTcpSocket *>(sender());
    if (this->proxy_list->right.find(sock) != this->proxy_list->right.end()) {
        if(this->proxy_list->right.at(sock) != nullptr){
            this->proxy_list->right.at(sock)->close();
            this->proxy_list->right.at(sock)->deleteLater();
        }
        this->proxy_list->right.erase(sock);
        sock->close();
        sock->deleteLater();
        sock = nullptr;
        return;
    }else if (this->proxy_list->left.find(sock) != this->proxy_list->left.end()) {
        if(this->proxy_list->left.at(sock) != nullptr){
            this->proxy_list->left.at(sock)->close();
            this->proxy_list->left.at(sock)->deleteLater();
        }
        this->proxy_list->left.erase(sock);
        sock->close();
        sock->deleteLater();
        sock = nullptr;
        return;
    }
    sock->close();
    sock->deleteLater();
    sock = nullptr;

}
