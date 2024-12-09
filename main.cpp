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
#include <QProcess>
#include <QObject>
#include <cstdlib>
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
        qDebug()<<INVALID_ARGUMENTS;
        exit(0);
    }


    if(QString(argv[1]) != INBOUND){
        qDebug()<<INVALID_ARGUMENTS;
        exit(0);
    }

    QStringList lst_inbound = QString(argv[2]).split(":");
    if(lst_inbound.size()<2){
        qDebug()<<INVALID_ARGUMENTS;
        exit(0);
    }
    in_host = lst_inbound[0];
    in_port = lst_inbound[1].toULongLong();

    if(in_host.isEmpty() || in_port <= 0){
        qDebug()<<INVALID_IP_PORT;
        exit(0);
    }

    if(argc > 3){

        if(QString(argv[3]) != OUTBOUND){
            qDebug()<<INVALID_ARGUMENTS;
            exit(0);
        }



        QStringList lst_outbound = QString(argv[4]).split(":");
        if(lst_outbound.size()<2){
            qDebug()<<INVALID_ARGUMENTS;
            exit(0);
        }

        out_host = lst_outbound[0];
        out_port = lst_outbound[1].toULongLong();

        if(out_host.isEmpty() || out_port <= 0){
            qDebug()<<INVALID_IP_PORT;
            exit(0);
        }

    }
}

void* create_proxy_ser_array_shared_memory(int cpu_nums){

    int shm_fd = shm_open(PROXY_SER_ARR_SHARED_NAME , O_CREAT | O_RDWR , 0666);
    if(shm_fd == -1){
        perror(SHARED_MEMORY_OPEN);
        exit(1);
    }
    size_t array_size = cpu_nums*sizeof(proxy_server);
    if(ftruncate(shm_fd , array_size) == -1){
        perror(F_TRUNCATE);
        exit(1);
    }

    void* pointer = mmap(0 , array_size , PROT_READ | PROT_WRITE , MAP_SHARED , shm_fd , 0);
    if(pointer == MAP_FAILED){
        perror(MEMORY_MAP);
        exit(1);
    }
    return pointer;
}

void create_proxy_ser_lck_shared_memeory(){

    sem_t* sem = sem_open(LOCK_SHARED_NAME , O_CREAT , 0666 , 1);
    if(sem == SEM_FAILED){
        perror(SHARED_MEMORY_OPEN);
        exit(1);
    }
}
void* create_proxy_shared_atomic_var(){

    int shm_fd = shm_open(PROXY_SER_ATOMIC_SHARED_NAME , O_CREAT | O_RDWR , 0666);
    if(shm_fd == -1){
        perror(SHARED_MEMORY_OPEN);
        exit(1);
    }
    size_t array_size = sizeof(std::atomic<bool>);
    if(ftruncate(shm_fd , array_size) == -1){
        perror(F_TRUNCATE);
        exit(1);
    }

    void* pointer = mmap(0 , array_size , PROT_READ | PROT_WRITE , MAP_SHARED , shm_fd , 0);
    if(pointer == MAP_FAILED){
        perror(MEMORY_MAP);
        exit(1);
    }
    return pointer;
}
void* proxy_ser_shard_circular_queue(){

    int shm_fd = shm_open(PROXY_SER_CIRCULAR_QUEUE_SHARED_NAME , O_CREAT | O_RDWR , 0666);
    if(shm_fd == -1){
        perror(SHARED_MEMORY_OPEN);
        exit(1);
    }
    size_t array_size = sizeof(CircularQueue);
    if(ftruncate(shm_fd , array_size) == -1){
        perror(F_TRUNCATE);
        exit(1);
    }

    void* pointer = mmap(0 , array_size , PROT_READ | PROT_WRITE , MAP_SHARED , shm_fd , 0);
    if(pointer == MAP_FAILED){
        perror(MEMORY_MAP);
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


    create_proxy_ser_lck_shared_memeory();

    quint64 number = 0;
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    QTcpServer* server = new QTcpServer();


    CircularQueue* qu = reinterpret_cast<CircularQueue*>(proxy_ser_shard_circular_queue());
    new(qu)CircularQueue();

    std::atomic<bool>* pointer_at = reinterpret_cast<std::atomic<bool>*>(create_proxy_shared_atomic_var());
    new(pointer_at)std::atomic<bool>();
    pointer_at->store(false);

    proxy_server* pointer = reinterpret_cast<proxy_server*>(create_proxy_ser_array_shared_memory(num_cpus));

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
