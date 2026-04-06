#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

typedef struct {
    int fd;
    int state;
    char name[128];
    char mssv[20];
}UserInput;


// Hàm hỗ trợ tách tên và tạo email
void generate_email(char *name, char *mssv, char *email) {
    char *words[15];
    int count = 0;
    char temp_name[128];
    strcpy(temp_name, name);

    char *space = strtok(temp_name, " ");
    while (space != NULL && count < 15){
        words[count++] = space;
        space = strtok(NULL, " ");
    }
    
    if (count < 1) return;

    char first_name[64];
    strcpy(first_name, words[count - 1]);
    for(int i = 0; first_name[i]; i++) first_name[i] = tolower(first_name[i]);
    
    char middle_last_name[15];
    int i;
    for(i = 0; i < count - 1; i++){
        middle_last_name[i] = (char)tolower(words[i][0]);
    }
    middle_last_name[i] = '\0';

    char *final_mssv = mssv + 2;

    sprintf(email, "%s.%s%s@sis.hust.edu.vn", first_name, middle_last_name, final_mssv);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    printf("Email Server is running on port 8080...\n");

    UserInput clients[64];
    int numberclients = 0;
    char buf[256], email[256];

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client != -1) {
            printf("New connection: %d\n", client);
            ul = 1;
            ioctl(client, FIONBIO, &ul);
            clients[numberclients].fd = client;
            clients[numberclients].state = 0;
            numberclients++;
            send(client, "1. Nhap Ho va Ten (khong dau): ", 31, 0);
            
            
        }

        for (int i = 0; i < numberclients; i++) {
            int len = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
            if (len > 0) {
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0;

                if (clients[i].state == 0) {
                    // Lưu tên và chuyển sang đợi MSSV
                    strcpy(clients[i].name, buf);
                    clients[i].state = 1;
                    send(clients[i].fd, "2. Nhap MSSV: ", 14, 0);
                }

                else if (clients[i].state == 1) {
                    strcpy(clients[i].mssv, buf);
                    memset(email, 0, sizeof(email));
                    generate_email(clients[i].name, clients[i].mssv, email);
                    
                    char final_msg[512];
                    sprintf(final_msg, "=> Email cua ban: %s\nNhap ten moi de lam lai: ", email);
                    send(clients[i].fd, final_msg, strlen(final_msg), 0);
                    
                    clients[i].state = 0;
                }
            }
        }
        
    }
    close(listener);
    return 0;
}