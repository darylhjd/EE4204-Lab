#include "headsock.h"

// Get the difference in timing in ms between an end and start time.
double get_interval(struct timeval *end, struct timeval *start) {
    if ((end->tv_usec -= start->tv_usec) < 0) {
        --end->tv_sec;
        end->tv_usec += 1000000;
    }
    end->tv_sec -= start->tv_sec;
    return (end->tv_sec) * 1000.0 + (end->tv_usec) / 1000.0;
}

// Send all data in a file to the specified destination.
double send_all(FILE *fp, int *data_size, int num_du, int du_size, int client_socket, struct sockaddr *destination, int addr_len) {
    // We read the whole file into memory.
    // Pointer gymnastics to get the size of the file.
    long file_size;
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
    //printf("The size of the file is %ld bytes.\n", file_size);

    // Show what length will the size of one packet be.
    //printf("The size of a data unit is %d bytes.\n", du_size);

    // Create a char array for storing the whole file.
    *data_size = file_size;
    char *to_send = malloc(file_size);
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
    //printf("Starting data transfer...\n");
    long offset = 0;
    while (offset <= file_size) {
        int test;
        // We send num_du units of data together, before waiting for acknowledgement.
        for (int i = 0; i < num_du; i++) {
            // Get the size of the data to be sent.
            int size;
            if ((file_size + 1 - offset) <= du_size) {
                size = file_size + 1 - offset;
            } else {
                size = du_size;
            }

            // If there is no more data to send, we do an early break.
            if (size == 0) {
                break;
            }

            // Copy the data to be sent in this particular data unit.
            char *du = malloc(size);
            memcpy(du, (to_send + offset), size);
            // Send the data
            if ((test = sendto(client_socket, du, size, 0, destination, addr_len)) == 999) {
                printf("There was an error sending a data unit! Exiting...\n");
                exit(1);
            }
            free(du);
            offset += size;
            //printf("Sent DU %d\n", i+1);
        }

        // Wait to receive an acknowledgement.
        //printf("Waiting for acknowledgement...");
        struct ACK ack;
        if ((test = recvfrom(client_socket, &ack, sizeof(struct ACK), 0, destination, (socklen_t *) &addr_len)) == -1) {
            printf("Error receiving data! Exiting...\n");
            exit(1);
        } else if (ack.num != 1 || ack.len != 0) {
            printf("Error in transmission of ACK.\n");
        } else {
            //printf("ACK received from server!\n");
        }
    }
    free(to_send);
    //printf("\t\tSent %ld bytes.\n", offset);

    // Get the end timing.
    gettimeofday(&end_time, NULL);
    return get_interval(&end_time, &start_time);
}

// Do all the sending.
double do_send(int client_socket, struct sockaddr *destination) {
    // Open the file and read the stuff to send.
    FILE *fp;
    if ((fp = fopen("myfile.txt", "r+t")) == NULL) {
        printf("File doesn't exist!\n");
        exit(1);
    }

    // Create output file.
    FILE *out = fopen("results.csv", "w");
    fputs("du_size,batch_size,transfer_time,throughput\n", out);

    // For each data unit size and each batch size, we repeat a number of times.
    for (int s = 0; s < sizeof(DU_SIZES) / sizeof(int); s++) {
        int du_size = DU_SIZES[s];
        printf("Testing %d DU size.\n", du_size);
        for (int i = 0; i < sizeof(DU_BATCH_SIZES) / sizeof(int); i++) {
            int num_du = DU_BATCH_SIZES[i];
            printf("\tTesting %d DU batch size.\n", num_du);
            for (int r = 0; r < REPEATS; r++) {
                // Send all the data.
                int data_size;
                double duration = send_all(fp, &data_size, num_du, du_size, client_socket, destination, sizeof(struct sockaddr_in));
                double data_rate = (data_size / duration);
                //printf("Time(ms): %.3f, Data sent (bytes): %d\nData rate: %.3f (Kbytes/s)\n", duration, data_size, data_rate);
                fprintf(out, "%d,%d,%.3f,%.3f\n", du_size, num_du, duration, data_rate);
            }
        }
    }

    // Close remaining stuff.
    close(client_socket);
    fclose(fp);
    return 0;
}

// Program starts here.
int main(int argc, char const *argv[]) {
    // Check correct number of arguments
    if (argc != 2) {
        printf("Wrong number of arguments specified! Exiting...\n");
        exit(1);
    }

    // Get hostname
    struct hostent *h = gethostbyname(argv[1]);
    if (h == NULL) {
        printf("Error getting hostname! Exiting...\n");
        exit(1);
    }
    printf("Canonical name: %s\n", h->h_name);
    // Print all aliases.
    for (char **p = h->h_aliases; *p != NULL; p++) {
        printf("Alias: %s\n", *p);
    }

    // Print address type.
    if (h->h_addrtype == AF_INET) {
        printf("Using AF_INET...\n");
    } else {
        printf("Using unknown address type!\n");
    }

    // Set up the client socket.
    struct in_addr **addrs = (struct in_addr **) h->h_addr_list;
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);  // create the socket
    if (client_socket < 0) {
        printf("\rError creating client socket!\n");
        exit(1);
    }
    struct sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(MYUDP_PORT);
    memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
    bzero(&(ser_addr.sin_zero), 8);

    // Do the sending.
    do_send(client_socket, (struct sockaddr *) &ser_addr);
}
