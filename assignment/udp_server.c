#include "headsock.h"

void receive_all(int server_socket, struct sockaddr *from, int addrlen) {
    // Create a char array to store received data.
    char received[BUFSIZE];

    int received_bytes = 0;
    int group_size = 0;
    // Keep receiving data until end (\0) received.
    while (1) {
        // Create a temporary buffer to store this iteration's received data.
        char rec[DUSIZE];

        // Receive data.
        int num_read = recvfrom(server_socket, &rec, DUSIZE, 0, from, (socklen_t *) &addrlen);
        if (num_read == -1) {
            printf("Error receiving data!\n");
            exit(1);
        }
        // Copy received data into received buffer.
        memcpy((received + received_bytes), rec, num_read);
        received_bytes += num_read;
        group_size++;
        printf("Read %d bytes!\n", num_read);
        // If we received NUMDU number of data units, then we send one acknowledgement.
        if (group_size == NUMDU || rec[num_read - 1] == '\0') {
            printf("Sending ACK...");
            struct ack_so ack;
            ack.num = 1;
            ack.len = 0;
            if (sendto(server_socket, &ack, sizeof(struct ack_so), 0, from, addrlen) == -1) {
                printf("Error sending ACK!\n");
                exit(1);
            }
            group_size %= NUMDU;
            printf("Done!");
            if (rec[num_read - 1] == '\0') {
                printf(" End of file detected, break!\n");
                received_bytes--; // Do not write the null character at the end.
                break;
            }
            printf("\n");
        }
    }

    // Write received data into file.
    printf("The file we received is %d bytes!\n", received_bytes);
    printf("Writing received data into file...");
    FILE *fp;
    if ((fp = fopen ("received_text.txt","wt")) == NULL) {
		printf("File doesn't exist\n");
		exit(1);
	}
	fwrite(received, 1, received_bytes, fp);
	fclose(fp);
    printf("Done!\n");
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
    while (1) {
        printf("Waiting for data...\n");
        receive_all(server_socket, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_in));
    }

    // Close.
    close(server_socket);
    exit(0);
}
