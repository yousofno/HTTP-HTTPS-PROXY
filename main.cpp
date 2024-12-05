#include <QCoreApplication>
#include "proxy_server.h"
int main(int argc , char** argv) {
    //checking the switch cases
    if(argc != 3){
        qDebug()<<"Invalid arguments";
        exit(0);
    }
    if(QString(argv[1]) != "--inbound"){
        qDebug()<<"Invalid arguments";
        exit(0);
    }
    QString host = "";
    quint64 port  = 0;
    QRegularExpression hostRegex(IP_PORT_REGEX);
    QRegularExpressionMatch match = hostRegex.match(QString(argv[2]));
    if (match.hasMatch()) {
        host = match.captured(1);
        port = match.captured(2).toLongLong();
    }

    if(host.isEmpty() || port <= 0){
        qDebug()<<"Invalid Port or IP";
        exit(0);
    }

    QCoreApplication a(argc , argv);
    proxy_server* proxy = new proxy_server(&a);
    if(!proxy->start_proxy_server(QHostAddress(host) , port)){
        qDebug()<<"An error accured...";
        exit(1);
    }
    qDebug()<<QString("Listening on %1:%2").arg(host).arg(port);
    return a.exec();

}
