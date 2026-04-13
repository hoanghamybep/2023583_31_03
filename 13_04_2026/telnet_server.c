/*******************************************************************************
 * @file    remote_cmd_server.c
 * @brief   Server sử dụng select() thực hiện đăng nhập và thực thi lệnh hệ thống
 * @date    2026-04-13 23:45
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_CLIENTS FD_SETSIZE
#define BUFFER_SIZE 256

/**
 * @brief Kiểm tra thông tin đăng nhập từ file databases.txt
 */
int check_login(char *user, char *pass) {
    FILE *f = fopen("database.txt", "r");
    if (f == NULL) return 0;

    char f_user[32], f_pass[32];
    while (fscanf(f, "%s %s", f_user, f_pass) != EOF) {
        if (strcmp(user, f_user) == 0 && strcmp(pass, f_pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        return 1;
    }

    printf("Server is listening on port 8080...\n");

    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);

    int logged_in[MAX_CLIENTS] = {0};
    char buf[BUFFER_SIZE];

    while (1) {
        fdtest = fdread;
        int ret = select(MAX_CLIENTS, &fdtest, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select() failed");
            break;
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (FD_ISSET(i, &fdtest)) {
                if (i == listener) {
                    // Chấp nhận kết nối mới
                    int client = accept(listener, NULL, NULL);
                    if (client < MAX_CLIENTS) {
                        FD_SET(client, &fdread);
                        char *msg = "Hay gui user pass de dang nhap (format: user pass):\n";
                        send(client, msg, strlen(msg), 0);
                        printf("New client connected: FD %d\n", client);
                    } else {
                        close(client);
                    }
                } else {
                    // Nhận dữ liệu từ client
                    ret = recv(i, buf, sizeof(buf) - 1, 0);
                    if (ret <= 0) {
                        printf("Client FD %d disconnected\n", i);
                        FD_CLR(i, &fdread);
                        close(i);
                        logged_in[i] = 0;
                    } else {
                        buf[ret] = 0;
                        // Xử lý loại bỏ ký tự xuống dòng (\n, \r)
                        buf[strcspn(buf, "\r\n")] = 0;

                        if (logged_in[i] == 0) {
                            // LOGIC ĐĂNG NHẬP
                            char user[32], pass[32];
                            if (sscanf(buf, "%s %s", user, pass) == 2 && check_login(user, pass)) {
                                logged_in[i] = 1;
                                char *msg = "Dang nhap thanh cong. Nhap lenh he thong:\n";
                                send(i, msg, strlen(msg), 0);
                            } else {
                                char *msg = "Sai tai khoan hoac mat khau. Nhap lai:\n";
                                send(i, msg, strlen(msg), 0);
                            }
                        } else {
                            // THỰC THI LỆNH
                            char tmp_file[32];
                            char cmd[512];
                            sprintf(tmp_file, "out_%d.txt", i);
                            
                            // Redirect output của lệnh vào file tạm
                            sprintf(cmd, "%s > %s 2>&1", buf, tmp_file);
                            printf("Executing for FD %d: %s\n", i, buf);
                            system(cmd);

                            // Đọc nội dung file kết quả và gửi trả client
                            FILE *f = fopen(tmp_file, "r");
                            if (f) {
                                char file_buf[1024];
                                while (fgets(file_buf, sizeof(file_buf), f)) {
                                    send(i, file_buf, strlen(file_buf), 0);
                                }
                                fclose(f);
                                // Xóa file tạm sau khi gửi xong
                            }
                            send(i, "\nDone.\n", 7, 0);
                        }
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}