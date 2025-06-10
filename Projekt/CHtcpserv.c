//=========================================================================
/*   SERVER TCP/UDP SELECT zgodny z PDF (praca25.pdf) - obsługa połączenia typu C (TCP) i U (UDP)
*/
//===========================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_CONNECTION 15
#define NAME_SIZE 20
#define SERWER_IP "0.0.0.0"

#define CONNSTATE_IDLE 0
#define CONNSTATE_RECEIVING 1
#define CONNSTATE_SENDING 2
#define CONNSTATE_CLOSING 3

struct conninfo {
    int status;
    int sock;
    unsigned char name[NAME_SIZE+1];
    unsigned int name_pos;
    int port;
    char type;
    int id_client;
    int id_sub_client;
    int pom_sub_id;
    uint32_t full_id;

    unsigned char recv_buffer[1000];
    unsigned char send_buffer[1000];
};

struct conninfo conn[MAX_CONNECTION];
int port_number;
int id_client = 0;
int free_conn = 0;
char* temp_buffer;

int find_free_slot() {
    for (int i = 0; i < MAX_CONNECTION; i++) {
        if (conn[i].status == CONNSTATE_IDLE)
            return i;
    }
    return -1;
}

void save_name(int i, unsigned char* c) {
    int j = 0;
    while (conn[i].name_pos < NAME_SIZE && c[j]) {
        conn[i].name[conn[i].name_pos++] = c[j++];
    }
    conn[i].name[conn[i].name_pos] = '\0';
}

void close_conn(int i) {
    close(conn[i].sock);
    bzero(&conn[i], sizeof(conn[i]));
    conn[i].status = CONNSTATE_IDLE;
}

void add_new_conn(int fd, char* name, int main_id, int sub_id, char type) {
    int i = find_free_slot();
    if (i == -1) return;

    bzero(&conn[i], sizeof(conn[i]));
    conn[i].sock = fd;
    conn[i].status = CONNSTATE_RECEIVING;
    conn[i].name_pos = 0;
    conn[i].id_client = main_id;
    conn[i].id_sub_client = sub_id;
    conn[i].type = type;
    conn[i].full_id = (main_id << 16) | sub_id;

    if (name && strlen(name) > 0) save_name(i, (unsigned char*)name);

    printf("Dodano nowe polaczenie typ %c, id: %d\n", type, conn[i].full_id);
}

void process_command(int i, unsigned char* msg) {
    char *start = strchr((char*)msg, ':');
    char *end = strchr((char*)msg, ';');
    if (!start || !end || end <= start) return;

    size_t len = end - start - 1;
    char *value = malloc(len + 1);
    strncpy(value, start + 1, len);
    value[len] = '\0';

    switch (msg[1]) {
        case 'N': { // #N@NAME:nazwa;!
            int idx = find_free_slot();
            if (idx >= 0) {
                id_client++;
                add_new_conn(conn[i].sock, value, id_client, 0, 'C');
                uint32_t full_id = (id_client << 16);
                char response[64];
                snprintf(response, sizeof(response), "#NOK@CONNECTION:%u;!", full_id);
                send(conn[i].sock, response, strlen(response), MSG_NOSIGNAL);
                printf("Client %s connected.\n", value);
            } else {
                char response[] = "#NER@ERROR:no free connection;!";
                send(conn[i].sock, response, strlen(response), MSG_NOSIGNAL);
                printf("Client %s try to connect but no free connection now.\n", value);
            }
            break;
        }
        case 'U': { // #U@!
            int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (udp_sock < 0) {
                perror("UDP socket error");
                break;
            }
            struct sockaddr_in udp_addr;
            bzero(&udp_addr, sizeof(udp_addr));
            udp_addr.sin_family = AF_INET;
            udp_addr.sin_addr.s_addr = INADDR_ANY;
            udp_addr.sin_port = htons(port_number + 1);

            if (bind(udp_sock, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
                perror("UDP bind error");
                close(udp_sock);
                break;
            }

            conn[i].pom_sub_id++;
            add_new_conn(udp_sock, (char*)conn[i].name, conn[i].id_client, conn[i].pom_sub_id, 'U');
            char response[128];
            snprintf(response, sizeof(response), "#UOK@PORT:%d;CONNECTION:%d;!", port_number + 1, conn[i].full_id);
            send(conn[i].sock, response, strlen(response), MSG_NOSIGNAL);
            break;
        }
        default:
            break;
    }

    free(value);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uzycie: %s <port>\n", argv[0]);
        exit(1);
    }

    port_number = atoi(argv[1]);
    temp_buffer = malloc(1024);
    if (!temp_buffer) exit(2);

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port_number);

    if (bind(listen_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        exit(3);
    }

    if (listen(listen_sock, 5) < 0) {
        perror("listen()");
        exit(4);
    }

    printf("Serwer nasluchuje na porcie %d\n", port_number);

    fd_set readfds;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listen_sock, &readfds);
        int maxfd = listen_sock;

        for (int i = 0; i < MAX_CONNECTION; i++) {
            if (conn[i].status == CONNSTATE_RECEIVING) {
                FD_SET(conn[i].sock, &readfds);
                if (conn[i].sock > maxfd) maxfd = conn[i].sock;
            }
        }

        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select()");
            continue;
        }

        if (FD_ISSET(listen_sock, &readfds)) {
            int newsock = accept(listen_sock, NULL, NULL);
            if (newsock >= 0) {
				int slot = find_free_slot();
				if (slot >= 0) {
				    int recv_len = recv(newsock, temp_buffer, 1024, 0);
				    if (recv_len > 0) {
				        temp_buffer[recv_len] = '\0';
				        add_new_conn(newsock, NULL, 0, 0, 'C');
				        printf("Polaczenie C, komenda: %s\n", temp_buffer);
				        process_command(slot, (unsigned char*)temp_buffer);
				    } else {
				        close(newsock);
				    }
				} else {
				    printf("Brak wolnych slotów na nowe połączenie C\n");
				    close(newsock);
					}
            }
        }

        for (int i = 0; i < MAX_CONNECTION; i++) {
            if (conn[i].status == CONNSTATE_RECEIVING && FD_ISSET(conn[i].sock, &readfds)) {
                int len = recv(conn[i].sock, conn[i].recv_buffer, sizeof(conn[i].recv_buffer), 0);
                if (len > 0) {
                    conn[i].recv_buffer[len] = '\0';
                    process_command(i, conn[i].recv_buffer);
                } else {
                    close_conn(i);
                }
            }
        }
    }

    free(temp_buffer);
    return 0;
}
