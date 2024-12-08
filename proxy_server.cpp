//
// Created by Yousof on 2/1/2024 AD.
//

#include "proxy_server.h"
proxy_server::proxy_server(QString outboundHost,quint64 outboundPort, QObject *parent): QObject(parent) {
    proxy_list = new bm_type();
    proxy = new QNetworkProxy();
    proxy->setType(QNetworkProxy::NoProxy);
    this->qu = (CircularQueue*)shared_mem_open_qu();
    this->at = (std::atomic<bool>*)shared_mem_open_atomic_bool();
    this->lck = shared_mem_open_lock();
    this->outboundHost = outboundHost;
    this->outboundPort = outboundPort;
    connect(this , SIGNAL(startConnection(QTcpSocket*)) , this , SLOT(service(QTcpSocket*)));
    connect(this , SIGNAL(sendLog(QString)) , this , SLOT(writeLog(QString)) , Qt::QueuedConnection);
}

void* proxy_server::shared_mem_open_atomic_bool(){
    int shm_fd = shm_open("proxy_ser_atomic" ,  O_RDWR , 0666);
    if(shm_fd == -1){
        perror("shm_open");
        exit(1);
    }
    size_t array_size = sizeof(std::atomic<bool>);
    if(ftruncate(shm_fd , array_size) == -1){
        perror("ftruncate");
        exit(1);
    }

    void* pointer = mmap(0 , array_size , PROT_READ | PROT_WRITE , MAP_SHARED , shm_fd , 0);
    if(pointer == MAP_FAILED){
        perror("mmap");
        exit(1);
    }
    return pointer;
}

sem_t* proxy_server::shared_mem_open_lock(){
    sem_t* sem = sem_open("proxy_ser_lck" , O_RDWR , 0666);
    if(sem == SEM_FAILED){
        perror("sem_open");
        exit(1);
    }
    return sem;


}
void* proxy_server::shared_mem_open_qu(){
    int shm_fd = shm_open("proxy_ser_circ" ,  O_RDWR , 0666);
    if(shm_fd == -1){
        perror("shm_open");
        exit(1);
    }
    size_t array_size = sizeof(CircularQueue);
    if(ftruncate(shm_fd , array_size) == -1){
        perror("ftruncate");
        exit(1);
    }

    void* pointer = mmap(0 , array_size , PROT_READ | PROT_WRITE , MAP_SHARED , shm_fd , 0);
    if(pointer == MAP_FAILED){
        perror("mmap");
        exit(1);
    }
    return pointer;
}

void proxy_server::writeLog(QString mssg){
    if(!this->at->load()){
        sem_wait(this->lck);
        this->qu->enqueue(mssg.toStdString().c_str());
        sem_post(this->lck);
    }

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

    QString host  = "";
    quint64 port  = 0;
    QString path = "";
    bool isHttps = false;
    HTTPRequestParser(answer , path , host , port , isHttps);



    if(host.isEmpty() || port <= 0){
        incomming_socket->close();
        incomming_socket->deleteLater();
        incomming_socket = nullptr;
        return;
    }


    //check if this is the log path
    if(path == LOG_PATH){
        this->at->store(true);
        QString answer = "TIMESTAMP     CLIENT IP    PROCC_ID\n";
        int semVal;
        sem_getvalue(this->lck , &semVal);
        sem_wait(this->lck);
        sem_getvalue(this->lck , &semVal);
        while(!this->qu->isEmpty()){
        answer += QString(this->qu->dequeue());
        answer += LF;
       }
        if(incomming_socket->state() == QAbstractSocket::ConnectedState){
        incomming_socket->write(answer.toStdString().c_str());
        incomming_socket->flush();
        incomming_socket->close();
        }
        incomming_socket->deleteLater();
        incomming_socket = nullptr;
        sem_post(this->lck);
        this->at->store(false);
        return;
    }


    if(!this->outboundHost.isEmpty() && this->outboundPort > 0){
        if(!host.contains(this->outboundHost) || this->outboundPort != port){
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
    if(remote_socket != nullptr){
    remote_socket->connectToHost(host , port);
    }
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



void proxy_server::HTTPRequestParser(const QString &request, QString &path, QString &host, quint64 &port, bool& isHttps){

    QStringList req = request.split(CRLF);
    if(req.size() == 0){
        return;
    }
    if(req.first().contains(CONNECT_METHOD)){
        isHttps = true;
    }else if(req.first().contains(GET_METHOD) || req.first().contains(POST_METHOD) || req.first().contains(PUT_METHOD)
               || req.first().contains(DELETE_METHOD)){
        isHttps = false;
    }

    //time to find host
    bool flag = false;
    for(auto index : req){
        if(index.contains("Host:")){
            QStringList hostList = index.split("Host:");
            if(hostList.size()<2){
                return;
            }
            QStringList host_port_finder = hostList[1].split(":");
            if(host_port_finder.isEmpty()){
                return;
            }
            if(host_port_finder.size() >= 2){
                port = host_port_finder[1].toULongLong();
            }else{
                if(isHttps){
                    port = HTTPS_PORT;
                }else{
                    port = HTTP_PORT;
                }
            }
            host = host_port_finder[0];
            flag = true;
            break;
        }

    }
    if(!flag){
        return;
    }


    // Define the regex pattern
    QRegularExpression regex(HOST_REGEX,
                             QRegularExpression::UseUnicodePropertiesOption);

    // Match the URL against the regex
    QStringList path_finder = req[0].split(" ");
    if(path_finder.size() >= 2){

    QRegularExpressionMatch match = regex.match(path_finder[1]);

    // Extract groups if a match is found
    if (match.hasMatch()) {
        path = match.captured(PATH);
        }
    }

    path.replace(" " , "");
    host.replace(" " , "");

}





