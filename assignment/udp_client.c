#include "headsock.h"

double get_interval(struct timeval *end, struct timeval *start) {
    struct timeval diff;
    diff.tv_sec = end->tv_sec;
    diff.tv_usec = end->tv_usec;

    if ((end->tv_usec -= start->tv_usec) < 0) {
        --diff.tv_sec;
        diff.tv_usec += 1000000;
    }
    diff.tv_sec -= start->tv_sec;
    return (diff.tv_sec) * 1000.0 + (diff.tv_usec) / 1000.0;
}

void send_all(FILE *fp, int client_socket) {
    // We read the whole file into memory.
    // Pointer gymnastics to get the size of the file.
    long file_size;
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
    printf("The size of the file is %ld bytes.\n", file_size);

    // Show what length will the size of one packet be.
    printf("The size of a packet is %d bytes.\n", DUSIZE);

    // Create a char array for storing the whole file.
    long send_size = file_size + 1;
    char *to_send = malloc(send_size);
    if (to_send == NULL) {
        printf("Error creating buffer to store file contents! Exiting...\n");
        exit(1);
    }
    // Copy file contents into the bugger
    fread(to_send, 1, file_size, fp);
    // Set the last byte as NULL character.
    to_send[file_size] = '\0';

    // Also keep track of the transfer time.
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Start sending all data!
    long offset = 0;
    while (offset <= send_size) {
        int test;
        // We send NUMDU units of data together, before waiting for acknowledgement.
        for (int i = 0; i < NUMDU; i++) {
            // Get the size of the data to be sent.
            int size;
            if ((send_size + 1 - offset) < DUSIZE) {
                size = send_size + 1 - offset;
            } else {
                size = DUSIZE;
            }
            // Copy the data to be sent in this particular data unit.
            char *du = malloc(size);
            memcpy(du, (to_send + offset), size);
            // Send the data
            if ((test = send(client_socket, du, size, 0)) == -1) {
                printf("There was an error sending a data unit! Exiting...\n");
                exit(1);
            }
            free(du);
            offset += size;
        }

        // Wait to receive an acknowledgement.
        struct ack_so ack;
        if ((test = recv(client_socket, &ack, 2, 0)) == -1) {
            printf("Error receiving data! Exiting...\n");
            exit(1);
        } else if (ack.num != 1 || ack.len != 0) {
            printf("Error in transmission of ACK.\n");
        } else {
            printf("ACK received from server!\n");
        }
    }
    free(to_send);

    // Get the end timing.
    gettimeofday(&end_time, NULL);
    return get_interval(&end_time, &start_time);
}

int main(int argc, char const *argv[]) {
    // Check correct number of arguments
    if (argc != 2) {
        printf("Wrong number of arguments specified! Exiting...\n");
        exit(1);
    }

    // Get hostname
    struct hostent *host = gethostbyname(argv[1]);
    if (host == NULL) {
        printf("Error getting hostname! Exiting...\n");
        exit(1);
    }
    printf("Canonical name: %s\n", host->h_name);
    // Print all aliases.
    for (char **p = host->h_aliases; *p != NULL; p++) {
        printf("Alias: %s\n", *p);
    }

    // Print address type.
    if (host->h_addrtype == AF_INET) {
        printf("Using AF_INET...\n");
    } else {
        printf("Using unknown address type!\n");
    }

    // Set up the client socket.
    struct in_addr **addrs = host->h_addr_list;
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);  //create the socket
    while (client_socket < 0) {
        printf("\rError creating socket!\n");
        exit(1);
    }
    struct sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(MYUDP_PORT);
    memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
    bzero(&(ser_addr.sin_zero), 8);


    FILE *fp;
    if ((fp = fopen("myfile.txt", "r+t")) == NULL) {
        printf("File doesn't exist!\n");
        exit(1);
    }

    // Send all the data.
    send_all(fp, client_socket);
}