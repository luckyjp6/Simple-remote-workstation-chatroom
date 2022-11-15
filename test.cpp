#include <cerrno>
#include <iostream>
#include <sys/types.h> 
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <algorithm>

int main(){

    char r[500], w[500];
    sprintf(r, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", "user1", 22, "my_cmd", "user2", 2);
    sprintf(w, "*** Error: the pipe #%d->#%d already exists. ***\n", 1, 2);

    std::string rr(r), ww(w);
    printf("%d ", rr.find("(#2"));
    printf("%d ", rr.find("(#2)"));
    printf("%d", ww.find("just piped"));
    // printf("%d\n", sscanf(r, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", name[0], &id[0], cmd, name[1], &id[1]));
    // printf("%s\n", cmd);
    // printf("%d\n", sscanf(w, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", name[0], &id[0], cmd, name[1], &id[1]));
    // printf("%s\n", cmd);

    return 0;
}