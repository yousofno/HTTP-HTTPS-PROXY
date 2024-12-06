#include <QCoreApplication>
#include "proxy_server.h"
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <unistd.h>
#include <sched.h>
#include <iostream>
#include <queue>
#include "robust_lck.h"
void bind_to_cpu(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    pid_t pid = getpid();
    if (sched_setaffinity(pid, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_setaffinity");
    }
}
void args(int argc , char** argv , QString& in_host , quint64& in_port , QString& out_host , quint64& out_port){

    //checking the switch cases
    if(argc < 3){
        qDebug()<<"Invalid arguments";
        exit(0);
    }
    if(QString(argv[1]) != INBOUND){
        qDebug()<<"Invalid arguments";
        exit(0);
    }

    QRegularExpression inbound(IP_PORT_REGEX);
    QRegularExpressionMatch match = inbound.match(QString(argv[2]));
    if (match.hasMatch()) {
        in_host = match.captured(1);
        in_port = match.captured(2).toLongLong();
    }

    if(in_host.isEmpty() || in_port <= 0){
        qDebug()<<"Invalid inbound Port or IP";
        exit(0);
    }

    if(argc > 3){

        if(QString(argv[3]) != OUTBOUND){
            qDebug()<<"Invalid arguments";
            exit(0);
        }
        QRegularExpression inbound(HOST_REGEX);
        QRegularExpressionMatch match = inbound.match(QString(argv[4]));
        if (match.hasMatch()) {
            out_host = match.captured(1);
            out_port = match.captured(2).toLongLong();
        }


        if(out_host.isEmpty() || out_port <= 0){
            qDebug()<<"Invalid outbound Port or IP";
            exit(0);
        }

    }


}
int main(int argc , char** argv) {


    QString inbound_host = "";
    quint64 inbound_port  = 0;
    QString outbound_host = "";
    quint64 outbound_port = 0;
    args(argc , argv , inbound_host , inbound_port , outbound_host , outbound_port);


    QCoreApplication app(argc, argv);
    shm::concurrent::robust_ipc_mutex* lock = new shm::concurrent::robust_ipc_mutex();
    quint64 number = 0;
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    QTcpServer* server = new QTcpServer();
    proxy_server* child_server[num_cpus];
    std::queue<QString>* qu = new std::queue<QString>();


    QObject::connect(server , &QTcpServer::newConnection , [server , &child_server , &number , num_cpus](){
        int index = (number) % num_cpus;
        number +=1;
        emit child_server[index]->startConnection(server->nextPendingConnection());
    });

    if (!server->listen(QHostAddress(inbound_host), inbound_port)) {
        std::cerr << "Failed to start server: " << server->errorString().toStdString() << std::endl;
        return 1;
    }else{
        qDebug()<<QString("we are listening on ip %1 and port %2").arg(inbound_host).arg(inbound_port);
    }


    for (int i = 0; i <= num_cpus-1; i++) {
        pid_t pid = fork();
        if (pid > 0) {
            // Child process
            bind_to_cpu(i); // Bind to specific CPU
            child_server[i] = new proxy_server(lock , qu , outbound_host , outbound_port);
        }
    }

    return app.exec();
}
