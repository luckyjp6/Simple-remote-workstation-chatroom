#include "worm_server.hpp"

int main(int argc, char **argv) {
	if (argc > 2) {
        printf("Usage: ./worm_server\n");
        return -1;
    }

    /* prepare socket */
    worm_server w_server;
    w_server.run();

    return 0;
}