#include <QCoreApplication>
#include "proxy_server.h"
int main(int argc , char** argv) {
    QCoreApplication a(argc , argv);
    proxy_server* proxy = new proxy_server(&a);
    if(!proxy->start_proxy_server(QHostAddress::AnyIPv4 , 1234)){
        qDebug()<<"An error accured...";
        exit(1);
    }
    qDebug()<<"Listening on :::1234";
    return a.exec();

}
