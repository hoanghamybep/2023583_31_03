#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

#define MAX_CLIENTS 64
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char client_id[64];
    int registered;
} ClientInfo;

int main() {
    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == -1) {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    if (listen(server, 5)) {
        perror("listen() failed");
        return 1;
    }

    printf("Server is listening on port 8080...\n");

    ClientInfo clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].registered = 0;
    }

    fd_set fdread;
    char buf[BUFFER_SIZE];
    char send_buf[BUFFER_SIZE + 128];

    while (1) {
        FD_ZERO(&fdread);
        FD_SET(server, &fdread);
        int max_fd = server;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1) {
                FD_SET(clients[i].fd, &fdread);
                if (clients[i].fd > max_fd) max_fd = clients[i].fd;
            }
        }

        int ret = select(max_fd + 1, &fdread, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select() failed");
            break;
        }

        // Chấp nhận kết nối mới
        if (FD_ISSET(server, &fdread)) {
            int client = accept(server, NULL, NULL);
            int i;
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = client;
                    clients[i].registered = 0;
                    break;
                }
            }
            if (i == MAX_CLIENTS) {
                close(client);
            } else {
                char *msg = "Vui long dang ky theo cu phap: <id>: <name>\n";
                send(client, msg, strlen(msg), 0);
            }
        }

        // Kiểm tra dữ liệu từ các client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &fdread)) {
                ret = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) {
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    clients[i].registered = 0;
                    continue;
                }

                buf[ret] = 0;
                buf[strcspn(buf, "\r\n")] = 0;

                if (clients[i].registered == 0) {
                    char id[64], name[64], extra[64];
                    int count = sscanf(buf, "%[^:]: %s %s", id, name, extra);

                    if (count == 2) {
                        strcpy(clients[i].client_id, id);
                        clients[i].registered = 1;
                        char *msg = "Dang ky thanh cong!\n";
                        send(clients[i].fd, msg, strlen(msg), 0);
                    } else if (count > 2) {
                        // Sai: Name chứa khoảng trắng
                        char *msg = "Loi: client_name phai viet lien (khong co dau cach). Nhap lai:\n";
                        send(clients[i].fd, msg, strlen(msg), 0);
                    } else {
                        // Sai: Không đúng định dạng id: name
                        char *msg = "Sai cu phap! Yeu cau: <id>: <name>. Nhap lai:\n";
                        send(clients[i].fd, msg, strlen(msg), 0);
                    }
                } else {
                    snprintf(send_buf, sizeof(send_buf), "%s: %s\n", clients[i].client_id, buf);

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd != -1 && clients[j].registered && i != j) {
                            send(clients[j].fd, send_buf, strlen(send_buf), 0);
                        }
                    }
                }
            }
        }
    }

    close(server);
    return 0;
}