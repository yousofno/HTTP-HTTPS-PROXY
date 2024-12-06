#include <QCoreApplication>
#include "proxy_server.h"
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <unistd.h>
#include <sched.h>
#include <iostream>
#include <queue>

void bind_to_cpu(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    pid_t pid = getpid();
    if (sched_setaffinity(pid, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_setaffinity");
    } else {
        std::cout << "Process " << pid << " bound to CPU " << cpu_id << std::endl;
    }
}

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



QCoreApplication app(argc, argv);


std::atomic_int number = 0;
int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
QTcpServer* server = new QTcpServer();
proxy_server* child_server[num_cpus];
QMutex* mutex = new QMutex();
std::queue<QString>* qu = new std::queue<QString>();


QObject::connect(server , &QTcpServer::newConnection , [server , &child_server , &number , num_cpus](){
    int index = (number) % num_cpus;
    number +=1;
    emit child_server[index]->startConnection(server->nextPendingConnection());
});

if (!server->listen(QHostAddress(host), port)) {
    std::cerr << "Failed to start server: " << server->errorString().toStdString() << std::endl;
    return 1;
}else{

    qDebug()<<QString("we are listening on ip %1 and port %2").arg(host).arg(port);
}


for (int i = 0; i <= num_cpus-1; i++) {
    pid_t pid = fork();

    if (pid > 0) {
        // Child process
        bind_to_cpu(i); // Bind to specific CPU
        child_server[i] = new proxy_server(mutex , qu);
        std::cout << "Server running in PID " << getpid() << " on CPU " << i << std::endl;
    }
}

return app.exec(); // Keep the parent process running
}
