#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_SIZE 1048576
 
int main(int argc,char *argv[])
{
    char buffer[MAX_SIZE];
    char closeSocket[] = "close\n";
    int sockfd, n;
    
    while(1){
        
        int len = sizeof(struct sockaddr);
        
        struct sockaddr_in servaddr;
        
        /*Print USAGE if ran without Server Port No.*/
        if(argc < 2 ){fprintf(stderr, "usage: %s <port_number>\n", argv[0]); exit(1);}
        int svrport = atoi(argv[1]);
        
        /* AF_INET - IPv4 IP , TCP socket, protocol*/
        sockfd=socket(AF_INET, SOCK_STREAM, 0);
        bzero(&servaddr,sizeof(servaddr));
        
        /*set socket atributes*/
        servaddr.sin_family=AF_INET;
        servaddr.sin_port=htons(svrport); // Server port number
        /* Convert IPv4 and IPv6 addresses from text to binary form */
        inet_pton(AF_INET,"129.120.151.94",&(servaddr.sin_addr));
        /* Connect to the server */
        connect(sockfd,(struct sockaddr *)&servaddr, sizeof(servaddr));
        
        // get the URL to visit
        printf("Please enter the URL (starting with www.): ");
        bzero(buffer,MAX_SIZE);
        fgets(buffer,512,stdin);
        write(sockfd,buffer,strlen(buffer));
        
        /*close if close*/
        if(strcmp(buffer, closeSocket) == 0) break;
        
        //recieve server response
        bzero(buffer,MAX_SIZE);
        recv(sockfd,buffer,MAX_SIZE, MSG_WAITALL);
        
        //Display HTTP request
        printf("%s\n", buffer);
    }
    
    close(sockfd);
    exit(1);
    return 0;
}
