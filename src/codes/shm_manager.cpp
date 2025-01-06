#include "shm_manager.hpp"
#include "util.hpp"

shm_manager::shm_manager() {
    /* get share memory */
    shm_key[shm_manager::user_data] = (key_t)(1453);  
    shm_id[shm_manager::user_data] = shmget(shm_key[shm_manager::user_data], SHM_SEG_SIZE*MAX_USER, PERMS|IPC_CREAT);
    if (shm_id[shm_manager::user_data] < 0) {
        err_sys(this, __func__, "shmget");
    }

    /* reset all */
    reset(shm_manager::user_data, 0, SHM_SEG_SIZE*MAX_USER);

    /* reset client num */
    set_num_data(shm_manager::user_data, 0);

    /* initialize clients */
    client_pid temp;
    for (int i = 0; i < MAX_USER-10; i++) {
        temp.set_id(i);
        set_user_info(temp);
    }

    /* get user broadcast memory */
    shm_key[shm_manager::broadcast] = (key_t)(1453+shm_manager::broadcast);
    shm_id[shm_manager::broadcast] = shmget(shm_key[shm_manager::broadcast], MSG_MAX, PERMS|IPC_CREAT);
    if (shm_id[shm_manager::broadcast] < 0) {
        err_sys(this, __func__, "shmget fail");
    }
    
    /* reset message num */
    reset(shm_manager::broadcast, 0, MSG_MAX);
    set_num_data(shm_manager::broadcast, 0);
}

std::unique_ptr<void, SharedMemoryDeleter> shm_manager::get_addr(int idx) {
/* 
    Do: Get & check the start address of shared memory indexd by idx
    Return: the start address of shared memory indexd by idx
*/
    char *addr = (char*)shmat(shm_id[idx], 0, 0);
    if (addr == NULL) {
        err_sys(this, __func__, "shmat fail");
    }
    return std::unique_ptr<void, SharedMemoryDeleter>(addr);
}
void shm_manager::read(int idx, int offset, char* msg, int len) {
/*
Do: Read data from shared memory indexed by idx, with offset
Return: None (read data will be stored in the input arguments msg)
*/
    auto addr = get_addr(idx);
    memcpy(msg, addr.get() + offset, len);
}
void shm_manager::write(int idx, int offset, char* msg, int len) {
/*
    Do: Write specific message with given msg, len to given shared memory with offset
    Return: None
*/
    auto addr = get_addr(idx);

    memcpy(addr.get() + offset, msg, len);
}

void shm_manager::reset(int idx, int offset, int len) {
/* 
    Do: Reset shared memory.
    Return: None
*/
    auto addr = get_addr(idx);
    memset(addr.get() + offset, '\0', len);
}
void shm_manager::set_num_data(int idx, int num) {
/*
    Do: Set current client number in the shared memory with specified index to given number
    Return: None
*/  
    char n[NUM_DATA_LEN] = {0};    
    sprintf(n, "%d", num);

    write(idx, 0, n, NUM_DATA_LEN);
}
int shm_manager::get_num_data(int idx) {
/* 
    Do: Get current client number in the shared memory with specified index
    Return: Current number of client corresponds to the given indexed shared memory
*/
    auto addr = get_addr(idx);

    char n[NUM_DATA_LEN] = {0};
    memcpy(n, addr.get(), NUM_DATA_LEN);
    
    return atoi(n);
}
void shm_manager::set_user_info(client_pid c) {
/*
    Do: Append user info to share memroy
    Return: None
*/
    reset(shm_manager::user_data, c.get_id()*SHM_SEG_SIZE + SHM_SEG_INTERVAL, SHM_SEG_SIZE);
    
    char now[SHM_SEG_SIZE];
    memset(now, '\0', SHM_SEG_SIZE);
    sprintf(now, "%d:%s:%d:%s:%d", c.get_connfd(), c.get_name(), c.get_pid(), c.get_addr(), c.get_port()); 
    write(shm_manager::user_data, c.get_id()*SHM_SEG_SIZE + SHM_SEG_INTERVAL, now, strlen(now));
}
std::optional<client_pid> shm_manager::read_user_info(int id) {
/*
    Do: Read user info indexed by id
    Return: The corresponding user info (null if not found)
*/
    char user_info[SHM_SEG_SIZE];
    if (id >=0) {
        read(shm_manager::user_data, id*SHM_SEG_SIZE + SHM_SEG_INTERVAL, user_info, SHM_SEG_SIZE);
        
        /* parse user info*/
        client_pid c;
        char *ptr = user_info;
        c.set_connfd(atoi(strtok_r(ptr, ":", &ptr)));

        if (c.get_connfd() < 0) { return std::nullopt; }

        c.set_name(strtok_r(ptr, ":", &ptr));
        c.set_pid(atoi(strtok_r(ptr, ":", &ptr)));
        c.set_addr(strtok_r(ptr, ":", &ptr));
        c.set_port(atoi(strtok_r(ptr, "\0", &ptr)));

        return c;
    } else { return std::nullopt; }
}

void shm_manager::clear() {
    for (int i = 0; i < 3; i++) {
        if (shmctl(shm_id[i], IPC_RMID, 0) < 0) {
            err_sys(this, __func__, "shmctl remove id fail");
        }
    }
}