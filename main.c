#include<netinet/in.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h> //for close()
// #include<pthread.h> //posix threads

//struct for thread function args cuz C is weird?
struct thread_args {
    int sfd;
    char* client_buffer;
};

int max(int a, int b) {
    return (a > b) ? a : b;
}

// void *sock1_reader_thread(void* arg) {

//     // Cast the generic void pointer back to our specific struct pointer
//     struct thread_args *data = (struct thread_args *)arg;

//     recv(data->sfd, data->client_buffer, sizeof(data->client_buffer), 0);
//     char reply[50];
//     snprintf(reply, sizeof(reply), "Client 1 has said %s", data->client_buffer);
//     printf("%s\n", reply);

//     return NULL;
// }

// void *sock2_reader_thread(void *arg) {

//     // Cast the generic void pointer back to our specific struct pointer
//     struct thread_args *data = (struct thread_args *)arg;

//     recv(data->sfd, data->client_buffer, sizeof(data->client_buffer), 0);
//     char reply[50];
//     snprintf(reply, sizeof(reply), "Client 2 has said %s", data->client_buffer);
//     printf("%s\n", reply);

//     return NULL;
// }

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

    int client1_sfd = accept(sockfd, (struct sockaddr*)&their_addr, &their_socklen); //this is one conenction lifecycle

    char message[] = "hi from nirmal!\nEnter some thing... (q to exit)\n";
    send(client1_sfd, message, sizeof(message), 0); //flags = 0 -> nothing fancy yet

    int client2_sfd = accept(sockfd, (struct sockaddr*)&their_addr, &their_socklen);
    send(client2_sfd, message, sizeof(message), 0); 

    //adding select logic
    //imp: select() modifies your fd_set!!

    //clear the set
    FD_ZERO(&read_fds);
    
    //adding my FDs
    FD_SET(client1_sfd, &read_fds);
    FD_SET(client2_sfd, &read_fds);

    max_fd = max(client1_sfd, client2_sfd); //why?

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

        if (activity > 0) {



            //checking if sockfd1 has data
            if (FD_ISSET(client1_sfd, &working_set)) {
                int n = recv(client1_sfd, buffer, sizeof(buffer), 0);

                if (n <= 0) {
                    //error
                    break;
                }

                snprintf(reply, sizeof(reply), "Client 2 said : %s", buffer);

                send(client2_sfd, reply, n+16, 0); // n + 16 so account for "Client 2 said : " part in the len of the bytes sent
            }

            //checking if sockfd2 has data
            if (FD_ISSET(client2_sfd, &working_set)) {
                
                int n = recv(client2_sfd, buffer, sizeof(buffer), 0);

                if (n <= 0) {
                    //error
                    break;
                }

                snprintf(reply, sizeof(reply), "Client 1 said : %s", buffer);

                send(client1_sfd, reply, n+16, 0);

            }


            

        }

        

    }
    

    close(client1_sfd);
    close(client2_sfd);
    close(sockfd);

    freeaddrinfo(res); //all done with addr cuz we got the socket? 


    return 0;
}

/*
When you do:

nc localhost 8080

nc creates socket
nc calls connect()
your server’s accept() wakes up
new socket created
you send data
nc prints it
*/