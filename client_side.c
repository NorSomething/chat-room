#include<netinet/in.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h> //for close()

int main(int argc, char **argv) {

    struct sockaddr_in server_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //localhost, hardcoding ipv4

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect error");
        exit(1);
    }


    fd_set read_fds;
    int max_fd = (sockfd > 0) ? sockfd : 0;

    char buffer[1024];

    while(1) {

        memset(buffer, 0, sizeof(buffer));

        //dont need working set and all cuz we know we only have two fixed items in the set
        //no dynamic number of FDs stuff in here.

        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds); //0 is STDIN
        FD_SET(sockfd, &read_fds);

        if (select(max_fd+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select error");
            exit(1);
        }

        if (FD_ISSET(0, &read_fds)) {
            //STDIN has activity
            fgets(buffer, sizeof(buffer), stdin);
            send(sockfd, buffer, strlen(buffer), 0);
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            //SOCKFD has activity
            int n = recv(sockfd, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                //connection closed;
                break;
            }
            printf("%s", buffer);
        }

    }

    close(sockfd);



    return 0;
}