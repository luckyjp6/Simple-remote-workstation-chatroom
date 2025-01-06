#ifndef SHM_MANAGER_HPP
#define SHM_MANAGER_HPP

#include <iostream>
#include <optional>

/* shared memory */
#include <sys/shm.h>
#include <memory>

#include "worm_server.hpp"

#define PERMS 0666
#define NUM_DATA_LEN 4
#define SHM_SEG_SIZE 100
#define SHM_SEG_INTERVAL 5

/* Self-defined deleter for shared memory*/
struct SharedMemoryDeleter {
    void operator()(void* addr) const {
        if (shmdt(addr) == -1) {
            std::cerr << "shmdt failed: " << strerror(errno) << std::endl;
        }
    }
};


/* shared memory manager*/
class shm_manager {
private:
    key_t shm_key[3];   // user data, broadcast
    int shm_id[3];      // 0: user data, 2: broadcast
    int server_gid = -1;
    std::unique_ptr<void, SharedMemoryDeleter> get_addr(int idx); 
    void read(int idx, int offset, char* msg, int len);
    void write(int idx, int offset, char* msg, int len);
public:
    enum SHM_id {
        user_data,
        none,
        broadcast
    };
    
    shm_manager(); 
    void reset(int idx, int offset, int len); 
    
    void set_num_data(int idx, int num);
    int get_num_data(int idx);
    // void alter_num_user(int amount);
    
    void set_user_info(client_pid c);
    std::optional<client_pid> read_user_info(int id);

    void clear();    
};

#endif