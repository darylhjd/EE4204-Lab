#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MYUDP_PORT 5350

#define BUFSIZE 60000

#define REPEATS 100
int DU_BATCH_SIZES[] = {1, 2, 4};
int DU_SIZES[] = {50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900};

struct ACK {
    uint8_t num;
    uint8_t len;
};
