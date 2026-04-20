#include<netinet/in.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h> //for close()

int main(int argc, char *argv) {

    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof hints);
    int sockfd;

    //for select()
    fd_set read_fds; //our master set
    fd_set working_set;
    int max_fd;

    //master_set  = all sockets I care about
    //working_set = sockets that are READY right now (after select)

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0; //i think?
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, "8080", &hints, &res);

    //now lets move through the linked list till one socket connects
    for (p = res; p != NULL; p = p->ai_next) {
        
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        int yes = 1; //this is just for like bypassing the OS time limit in between succsive like failed connections to localhost
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            return -1; 
        }

        if ((bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
            close(sockfd); //close cuz bind failed 
            perror("server: bind");
            continue;
        }

        break; //found working struct -> bind succeeded.


    }

    if (p == NULL) {
        //nothign worked so we exit
        printf("Nothing in res worked, exiting...");
        return -1;
    }

    listen(sockfd, 10); //backlog queue len is 10
    //listen just turns the server on


    struct sockaddr_storage their_addr; //sockaddr_storage so we can fit ipv6 stuff too
    socklen_t their_socklen = sizeof(their_addr); //can be int also but this is better i think

    char message[] = "hi from nirmal!\nEnter some thing... (q to exit)\n";

    //adding select logic
    //imp: select() modifies your fd_set!!

    //clear the set
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    max_fd = sockfd;
    

    // Only monitoring read_fds
    // this inside while loop
    
    char reply[1024];
    char buffer[1024];

    //we gon multipelx on this shit
    while(1) {

        //clearing ze buffers)
        memset(buffer, 0, sizeof(buffer));

        working_set = read_fds;
        int activity = select(max_fd + 1, &working_set, NULL, NULL, NULL);
        //select wakes up when a client connects, we set its FD as max_fd+1?

        if (activity > 0) {
            
            for (int i = 0; i <= max_fd; i++) {

                if (FD_ISSET(i, &working_set)) {

                    if (i == sockfd) {

                        //new connection has joined
                        int new_sock_fd = accept(sockfd, (struct sockaddr*)&their_addr, &their_socklen); //creates brand new FD
                        FD_SET(new_sock_fd, &read_fds);
                        send(new_sock_fd, message, sizeof(message), 0); 

                        if (new_sock_fd > max_fd)
                            max_fd = new_sock_fd;
                    }
                    
                    else {

                        //existing connection
                        int n = recv(i, buffer, sizeof(buffer), 0);

                        if (n <= 0) {
                            //error so remove that client
                            close(i);
                            FD_CLR(i, &read_fds);

                        }

                        snprintf(reply, sizeof(reply), "Client said : %s", buffer);

                        //i is sender, rest all -> j loop is recivier

                        for (int j = 0; j <= max_fd; j++) {
                            if (FD_ISSET(j, &read_fds)) {
                                if (j != sockfd && j != i) {
                                    send(j, reply, strlen(reply), 0);
                                }

                            }
        
                        }

                    }

                }

            }

        }

    }
    
    close(sockfd);
    freeaddrinfo(res); //all done with addr cuz we got the socket? 


    return 0;
}