#ifndef ROUBUST_LCK_H
#define ROUBUST_LCK_H

#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <string>
#include <thread>
#include<threads.h>
#include <pthread.h>
#include <atomic>
#include <pthread.h>
#include <cerrno>
#include <iostream>
#include <pthread.h>
#include <sys/stat.h>
#include <chrono>
#include <iostream>
#include <string>
#include<boost/interprocess/sync/interprocess_sharable_mutex.hpp>
namespace shm::concurrent {
inline bool proc_dead(__pid_t proc) {
    if (proc == 0) {
        return false;
    }
    std::string pid_path = "/proc/" + std::to_string(proc);
    struct stat sts {};
    return (stat(pid_path.c_str(), &sts) == -1 && errno == ENOENT);
}
class robust_ipc_mutex {
public:
    robust_ipc_mutex() = default;

    void lock() {
        // in a spin lock until we get access
        while(!mut.try_lock()) {
            if (exclusive_owner != 0) {
                auto ex_proc = exclusive_owner.load(); // atomic load
                if (proc_dead(ex_proc)) {
                    if (exclusive_owner.compare_exchange_strong(ex_proc, 0)) {
                        // ensures that the process which we checked for liveness
                        // is the same as the value we're replacing

                        // if the condition returns false
                        // exclusive_owner was reset by some other process
                        mut.unlock();
                        continue;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(TIMEOUT));
        }
        exclusive_owner = getpid();
    }

    void unlock() {
        auto current_pid = getpid();
        if (exclusive_owner.compare_exchange_strong(current_pid, 0)) {
            mut.unlock();
        }
    }

private:
    boost::interprocess::interprocess_sharable_mutex mut;
    std::atomic<__pid_t> exclusive_owner{0};
};

}
#endif // ROUBUST_LCK_H
