/*
** client.c -- a stream socket client demo
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "6969" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 
#define NUM_THREADS 2

void *read_from_socket(void *sfd)
{
   int sockfd, numbytes;
   char buf[MAXDATASIZE];
   sockfd = (int)sfd;
   while ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0) {
	   buf[numbytes] = '\0';
           printf("%s",buf);
   }


   if(numbytes == -1)
   {
        perror("recv");
        exit(1);
    }

   pthread_exit(NULL);
}

void *write_to_socket(void *sfd)
{
   int sockfd;
   sockfd = (int)sfd;
   char buf[MAXDATASIZE];
   while(scanf("%s", buf) > 0)
   {
     strcat(buf, "\n");
     send(sockfd, buf, strlen(buf), 0);
   }
   pthread_exit(NULL);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

int main(int argc, char *argv[])
{
    int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    pthread_t threads[NUM_THREADS];
    int rc;

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    rc = pthread_create(&threads[0], NULL, read_from_socket, (void *)sockfd);
    if (rc){
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }

    rc = pthread_create(&threads[1], NULL, write_to_socket, (void *)sockfd);
    if (rc){
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    close(sockfd);

    return 0;
}
