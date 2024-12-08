#include <QCoreApplication>
#include "proxy_server.h"
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <unistd.h>
#include <sched.h>
#include <iostream>
#include <queue>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <new>
#include <semaphore.h>
#include "circularqueue.h"
void bind_to_cpu(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    pid_t pid = getpid();
    if (sched_setaffinity(pid, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_setaffinity");
    }else{
        qDebug()<<"proccess id "<<pid<<" is now bind to cpu number "<<cpu_id;
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

    QStringList lst_inbound = QString(argv[2]).split(":");
    if(lst_inbound.size()<2){
        qDebug()<<"Invalid arguments";
        exit(0);
    }
    in_host = lst_inbound[0];
    in_port = lst_inbound[1].toULongLong();

    if(in_host.isEmpty() || in_port <= 0){
        qDebug()<<"Invalid outbound Port or IP";
        exit(0);
    }

    if(argc > 3){

        if(QString(argv[3]) != OUTBOUND){
            qDebug()<<"Invalid arguments";
            exit(0);
        }



        QStringList lst_outbound = QString(argv[4]).split(":");
        if(lst_outbound.size()<2){
            qDebug()<<"Invalid arguments";
            exit(0);
        }

        out_host = lst_outbound[0];
        out_port = lst_outbound[1].toULongLong();

        if(out_host.isEmpty() || out_port <= 0){
            qDebug()<<"Invalid outbound Port or IP";
            exit(0);
        }

    }
}

void* shared_memory(int cpu_nums){

    int shm_fd = shm_open("proxy_ser" , O_CREAT | O_RDWR , 0666);
    if(shm_fd == -1){
        perror("shm_open");
        exit(1);
    }
    size_t array_size = cpu_nums*sizeof(proxy_server);
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

sem_t* shared_memory3(){

    sem_t* sem = sem_open("proxy_ser_lck" , O_CREAT , 0666 , 1);
    if(sem == SEM_FAILED){
        perror("sem_open");
        exit(1);
    }
    return sem;
}
void* shared_memory4(){

    int shm_fd = shm_open("proxy_ser_atomic" , O_CREAT | O_RDWR , 0666);
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
void* shared_memory5(){

    int shm_fd = shm_open("proxy_ser_circ" , O_CREAT | O_RDWR , 0666);
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




int main(int argc , char** argv) {

    QString inbound_host = "";
    quint64 inbound_port  = 0;
    QString outbound_host = "";
    quint64 outbound_port = 0;
    args(argc , argv , inbound_host , inbound_port , outbound_host , outbound_port);


    QCoreApplication app(argc, argv);
    sem_unlink("proxy_ser_lck");
    shm_unlink("proxy_ser");
    shm_unlink("proxy_ser_atomic");
    shm_unlink("proxy_ser_circ");

     sem_t* lock = (shared_memory3());

    quint64 number = 0;
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    QTcpServer* server = new QTcpServer();



    CircularQueue* qu = reinterpret_cast<CircularQueue*>(shared_memory5());
    new(qu)CircularQueue();

    std::atomic<bool>* pointer_at = reinterpret_cast<std::atomic<bool>*>(shared_memory4());
    new(pointer_at)std::atomic<bool>();
    pointer_at->store(false);

    proxy_server* pointer = reinterpret_cast<proxy_server*>(shared_memory(num_cpus));

    QObject::connect(server , &QTcpServer::newConnection , [server , pointer , &number , num_cpus](){
        int index = (number) % num_cpus;
        number +=1;
        emit pointer[index].startConnection(server->nextPendingConnection());
    });

    if (!server->listen(QHostAddress(inbound_host), inbound_port)) {
        std::cerr << "Failed to start server: " << server->errorString().toStdString() << std::endl;
        return 1;
    }else{
        qDebug()<<QString("we are listening on ip %1 and port %2").arg(inbound_host).arg(inbound_port);
    }

    int maxParentId = 0;
    for (int i = 0; i <= num_cpus-1; i++) {
        maxParentId = getpid();
        pid_t pid = fork();
        if (pid > 0) {
            // Child process
            bind_to_cpu(i); // Bind to specific CPU
            new (&pointer[i])proxy_server(outbound_host , outbound_port);
        }
        if(getpid() == maxParentId){
            break;
        }
    }

    return app.exec();
}
