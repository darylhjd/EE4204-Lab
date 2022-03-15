#include "headsock.h"

void receive_all(int num_du, int du_size, int server_socket, struct sockaddr *from, int addrlen) {
    // Create a char array to store received data.
    char received[BUFSIZE];

    int received_bytes = 0;
    int group_size = 0;
    // Keep receiving data until end (\0) received.
    while (1) {
        // Create a temporary buffer to store this iteration's received data.
        char rec[du_size];

        // Receive data.
        int num_read = recvfrom(server_socket, &rec, du_size, 0, from, (socklen_t *) &addrlen);
        if (num_read == -1) {
            printf("Error receiving data!\n");
            exit(1);
        }
        // Copy received data into received buffer.
        memcpy((received + received_bytes), rec, num_read);
        received_bytes += num_read;
        group_size++;
        //printf("Read %d bytes!\n", num_read);
        // If we received NUMDU number of data units, then we send one acknowledgement.
        if (group_size == num_du || rec[num_read - 1] == '\0') {
            //printf("Sending ACK...");
            struct ACK ack;
            ack.num = 1;
            ack.len = 0;
            if (sendto(server_socket, &ack, sizeof(struct ACK), 0, from, addrlen) == -1) {
                printf("Error sending ACK!\n");
                exit(1);
            }
            group_size %= num_du;
            //printf("Done!");
            if (rec[num_read - 1] == '\0') {
                //printf(" End of file detected, break!\n");
                received_bytes--; // Do not write the null character at the end.
                break;
            }
            //printf("\n");
        }
    }

    // Write received data into file.
    //printf("\t\tThe file we received is %d bytes!\n", received_bytes);
    //printf("Writing received data into file...");
    FILE *fp;
    if ((fp = fopen ("received_text.txt","wt")) == NULL) {
		printf("File doesn't exist\n");
		exit(1);
	}
	fwrite(received, 1, received_bytes, fp);
	fclose(fp);
    //printf("Done!\n");
}

int main(int argc, char *argv[]) {
    // Set up server.
    struct sockaddr_in client_addr;
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {  // create socket
        printf("Error creating server socket!\n");
        exit(1);
    }
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(MYUDP_PORT);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(client_addr.sin_zero), 8);

    // Bind to client.
    if (bind(server_socket, (struct sockaddr *) &client_addr, sizeof(struct sockaddr)) == -1) {  // bind socket
        printf("Error binding to client!\n");
        exit(1);
    }

    // Allow server to keep receiving.
    for (int s = 0; s < sizeof(DU_SIZES) / sizeof(int); s++) {
        int du_size = DU_SIZES[s];
        printf("Testing %d DU size.\n", du_size);
        for (int i = 0; i < sizeof(DU_BATCH_SIZES) / sizeof(int); i++) {
            int num_du = DU_BATCH_SIZES[i];
            printf("\tTesting %d DU batch size.\n", num_du);
            for (int r = 0; r < REPEATS; r++) {
                //printf("Waiting for data...\n");
                receive_all(num_du, du_size, server_socket, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_in));
            }
        }
    }

    // Close.
    close(server_socket);
    exit(0);
}
