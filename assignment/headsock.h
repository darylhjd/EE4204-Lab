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
#define MAXSIZE 50
#define DUSIZE 500
#define NUMDU 3

struct ack_so {
    uint8_t num;
    uint8_t len;
};