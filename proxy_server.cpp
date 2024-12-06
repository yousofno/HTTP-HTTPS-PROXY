//
// Created by Yousof on 2/1/2024 AD.
//

#include "proxy_server.h"
proxy_server::proxy_server(shm::concurrent::robust_ipc_mutex* lck , std::queue<QString> * qu ,QString outboundHost,quint64 outboundPort, QObject *parent): QObject(parent) {
    this->lck = lck;
    proxy_list = new bm_type();
    proxy = new QNetworkProxy();
    proxy->setType(QNetworkProxy::NoProxy);
    this->qu = qu;
    this->outboundHost = outboundHost;
    this->outboundPort = outboundPort;
    connect(this , SIGNAL(startConnection(QTcpSocket*)) , this , SLOT(service(QTcpSocket*)));
    connect(this , SIGNAL(sendLog(QString)) , this , SLOT(writeLog(QString)));
}

void proxy_server::writeLog(QString mssg){
         this->lck->lock();
        this->qu->push(mssg);
        this->lck->unlock();
        return;

}
proxy_server::~proxy_server() {
    delete proxy;
    for(auto index  = proxy_list->begin(); index != proxy_list->end();index++){
        index->left->deleteLater();
        index->right->deleteLater();
    }
}

void proxy_server::service(QTcpSocket* socket){
    if(socket != nullptr){
        connect(socket , SIGNAL(readyRead()) , this , SLOT(readFromSocket()));
        connect(socket , SIGNAL(error(QAbstractSocket::SocketError)) , this , SLOT(socket_err(QAbstractSocket::SocketError)));
    }
}



void proxy_server::reg(QString& url , QString& host , QString& path , quint64& port){
    // Define the regex pattern
    QRegularExpression regex(HOST_REGEX,
                             QRegularExpression::UseUnicodePropertiesOption);

    // Match the URL against the regex
    QRegularExpressionMatch match = regex.match(url);

    // Extract groups if a match is found
    if (match.hasMatch()) {
        host = match.captured(HOST);
        port = match.captured(PORT).toULongLong();
        path = match.captured(PATH);
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
            this->proxy_list->left.at(incomming_socket)->flush();
            emit writeLog(QString(QString::number(QDateTime::currentMSecsSinceEpoch())+" " + incomming_socket->peerAddress().toString() + " " + QString::number(getpid())));
        }
        return;
    }


    if (this->proxy_list->right.find(incomming_socket) != this->proxy_list->right.end()) {
        if(this->proxy_list->right.at(incomming_socket)->state() == QAbstractSocket::ConnectedState){
            this->proxy_list->right.at(incomming_socket)->write(answer);
            this->proxy_list->right.at(incomming_socket)->flush();
        }
        return;
    }
    QStringList req_list = QString(answer).split(CRLF);
    QString host  = "";
    quint64 port  = 0;
    QString path = "";
    QString url;
    bool isHttps = false;
    if(req_list.isEmpty()){
        incomming_socket->close();
        incomming_socket->deleteLater();
        incomming_socket = nullptr;
        return;
    }
    if(req_list.first().isEmpty()){
        incomming_socket->close();
        incomming_socket->deleteLater();
        incomming_socket = nullptr;
        return;
    }
    if(req_list.first().split(" ").size() < 2){
        incomming_socket->close();
        incomming_socket->deleteLater();
        incomming_socket = nullptr;
        return;
    }
    url = req_list.first().split(" ")[1];
    reg( url, host , path , port);

    if(req_list.first().contains(CONNECT_METHOD)){
        isHttps = true;
        if(port == 0){
            port = HTTPS_PORT;
        }
    }else if(req_list.first().contains(GET_METHOD) || req_list.first().contains(POST_METHOD) || req_list.first().contains(PUT_METHOD)
               || req_list.first().contains(DELETE_METHOD)){
        isHttps = false;
        if(port == 0){

            port = HTTP_PORT;
        }
    }else{

        incomming_socket->close();
        incomming_socket->deleteLater();
        incomming_socket = nullptr;
        return;
    }

    if(host.isEmpty() || port == 0){
        incomming_socket->close();
        incomming_socket->deleteLater();
        incomming_socket = nullptr;
        return;
    }

    //check if this is the log path
    if(path == LOG_PATH){
        this->lck->lock();
        QString answer = "TIMESTAMP     CLIENT IP    PROCC_ID\n";
        while(!this->qu->empty()){
            answer += QString(this->qu->front());
            this->qu->pop();
            answer += LF;
        }
        if(incomming_socket->state() == QAbstractSocket::ConnectedState){
        incomming_socket->write(answer.toStdString().c_str());
        incomming_socket->flush();
        incomming_socket->close();
        }
        incomming_socket->deleteLater();
        this->lck->unlock();
        return;
    }



    if(!this->outboundHost.isEmpty() && this->outboundPort > 0){
        if(this->outboundHost != host || this->outboundPort != port){
            incomming_socket->close();
            incomming_socket->deleteLater();
            incomming_socket = nullptr;
            return;
        }
    }


    QTcpSocket* remote_socket = new QTcpSocket();
    remote_socket->setProxy(*this->proxy);
    connect(remote_socket , SIGNAL(error(QAbstractSocket::SocketError)) , this , SLOT(socket_err(QAbstractSocket::SocketError)));
    connect(remote_socket , SIGNAL(readyRead()) , this , SLOT(readFromSocket()));
    this->proxy_list->insert(bm_type::value_type(incomming_socket, remote_socket));
    QObject::connect(remote_socket , &QTcpSocket::connected , this ,[this ,isHttps,answer, incomming_socket,path]() {
        if(this->proxy_list->left.find(incomming_socket) == this->proxy_list->left.end()){
            return;
        }
        if(isHttps){
            if(incomming_socket != nullptr){
            if(incomming_socket->state() == QAbstractSocket::ConnectedState){
                incomming_socket->write(OK);
                incomming_socket->flush();

            }
        }
        }else{
            this->proxy_list->left.at(incomming_socket)->write(answer.toStdString().c_str());
            this->proxy_list->left.at(incomming_socket)->flush();
        }

    });
    remote_socket->connectToHost(host , port);
    return;
}



void proxy_server::socket_err(QAbstractSocket::SocketError error) {
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





