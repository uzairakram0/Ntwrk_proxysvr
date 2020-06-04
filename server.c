/* README
This is a proxy server Program that caches 5 raw html pages
and deslivers them upon client request, if the page is not in
cache the proxy server will forward request to the client and
store the requested page in cache.
¯\_(ツ)_/¯ */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define MAX_SIZE 65536

//Prtototypes
void cacheWeb(int *WriteIndex, char req[5][512],char cache[MAX_SIZE], char urls[5][512], char hostname[512], char webuf[MAX_SIZE], char url[512], char MY_TIME[50]);
bool checkCode(char response[MAX_SIZE]);
void readBlocked(char blocked[10][256]);
void readReq(char Req[5][512]);
void readUrls(char URLs[5][512]);
void readCache(char cache[MAX_SIZE], char TIME[50]);
bool checkBlocked(char blocked[10][256], char hostname[256], char MY_TIME[50]);
 
int main(int argc, char **argv)
{
    int listen_fd, conn_fd;
    /*Proxy Cache Buffers*/
    char buffer[512];
    char url[512];
    char cache[MAX_SIZE];
    char req[5][512];
    char urls[5][512];
    char blocked[10][256];
    
    int WriteIndex = 0;
    
    /*Read files*/
    readBlocked(blocked);
    readReq(req);
    readUrls(urls);

    /*Print USAGE if ran withput Port No.*/
    if(argc < 2 ){fprintf(stderr, "usage: %s <port_number>\n", argv[0]); exit(1);}
    /*Convert port num to an int*/
    int portnum = atoi(argv[1]);
    
    /* AF_INET - IPv4 IP , TCP socket, protocol handled by OS*/
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    /*Socket Address Struct*/
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));//clear address structure
    /* setup servaddr atributes */
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(portnum);
    /* Binds the above details to the socket */
	bind(listen_fd,  (struct sockaddr *) &servaddr, sizeof(servaddr));
    
	/* Start listening to incoming connections */
    /*backlog queue of clients size 10*/
	listen(listen_fd, 10);
    printf("Listening...\n");
    
    while(1)
    {
      /* Accepts an incoming connection */
	  conn_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL);
        printf("Pending Client HTTP request...\n");
        
        /*Read HTTP query from client*/
        bzero(buffer, 512);
        read(conn_fd, buffer, 512);
        strcpy(url, buffer);
        printf("Processing http request: %s\n", url);
        
        time_t t ;
        struct tm *tmp ;
        char MY_TIME[50];
        time( &t );
          
        tmp = localtime( &t );
          
        // using strftime to display time
        strftime(MY_TIME, sizeof(MY_TIME), "%Y%m\%d%H%M\%S", tmp);
          
        printf("Date & time : %s\n", MY_TIME );
        
        /*write reply to client*/
        bzero(buffer, 512);
        strcpy(buffer, "Proxy is fetching data... \n\r\0");
        write(conn_fd, buffer, strlen(buffer));
        
            /* For webserver */
            int webfd;
            char webuf[MAX_SIZE];
            char response[MAX_SIZE];
            struct sockaddr_in webaddr;
        
            /* Parse URL */
            char hostname[256], substr[256], http_req[512];
            sscanf(url, "%[^/\n]", hostname);
            sscanf(url, "%*[^/]/%[^\n]", substr);
        
            if (strcmp(substr, " ")==0)
                sprintf(http_req, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", " ", hostname);
            else
                sprintf(http_req, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", substr, hostname);
        
            /*For blocked pages ( う-´)づ︻╦̵̵̿╤──   \(˚☐˚”)/  */
            if (checkBlocked(blocked, hostname, MY_TIME)){
                printf("blocked website\n");

                bzero(buffer, 512);
                strcpy(buffer, "website blocked \n\r\0");
                write(conn_fd, buffer, strlen(buffer));
                close(conn_fd);

                continue;
            }
            
            /* For getting address info*/
            /* retrieved from man7.org*/
            struct addrinfo hints;
            struct addrinfo *result, *rp;
            int s;
            
            memset(&hints, 0, sizeof(struct addrinfo));
           hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
           hints.ai_socktype = SOCK_STREAM; /* TCP socket */
            /*Get info and connect server port 80 for http*/
            s=getaddrinfo(hostname, "80", &hints, &result);
            
        if(s != 0){
            fprintf(stderr, "close: %s\n", gai_strerror(s));
            exit(EXIT_FAILURE);
        }
        
            /* getaddrinfo() returns a list of address structures.
              Try each address until we successfully connect(2).
              If socket(2) (or connect(2)) fails, we (close the socket
              and) try the next address. */

           for (rp = result; rp != NULL; rp = rp->ai_next)
           {
               if ((webfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))==-1){
                   printf("Error creating socket");
                   return 0;
               }

               if (connect(webfd, rp->ai_addr, rp->ai_addrlen)==-1){
                  close(webfd);
                   perror("web socket connect error");
                   continue;
               }
               
           }
        

        freeaddrinfo(result);
        
        /*Check if the URL request is cached*/
        int urlIndex;
        bool cached = false;
        for (urlIndex = 0; urlIndex < 5; urlIndex++){
            if(strcmp(url, urls[urlIndex]) == 0){
                cached = true;
                break;
            }
        }
        
        if(cached){ /*if page is cached*/
            /*send corresponding request to client*/
            char time[50];
            char tmp[256];
            
            strcpy(tmp, req[urlIndex]);
            char * token = strtok(tmp, " ");
            token = strtok(NULL, " ");
            strcpy(time, token);
            printf("%s\n", time);
            
            send(conn_fd, cache, MAX_SIZE, 0);
            
        } else { /*Forward and cache*/
            /* sending request to web server */
            send(webfd, http_req, strlen(http_req), 0);
        
            /*get http data from webserver*/
            bzero(webuf, MAX_SIZE);
            recv(webfd, webuf, MAX_SIZE, MSG_WAITALL);
        
            /* forward info to client */
            send(conn_fd, webuf, MAX_SIZE, 0);
            
            /*CACHING*/
            strcpy(response, webuf);
            if (checkCode(response)){
                cacheWeb(&WriteIndex, req, cache, urls, hostname, webuf, url, MY_TIME);
            }
        }
        
        // close connection with webserver
        close(webfd);
        close(conn_fd);
    }
        
        return 0;
        exit(1);
}

/*Function for writing the queries, urls, websites and the HTTP page to the appropriate buffers and files*/
void cacheWeb(int *WriteIndex, char req[5][512],char cache[MAX_SIZE], char urls[5][512], char hostname[512], char webuf[MAX_SIZE], char url[MAX_SIZE], char MY_TIME[50]){
    
    /*Cache requests*/
    char store[256];
    strcpy(store, hostname);
    strcat(store, " ");
    strcat(store, MY_TIME);
    strcat(store, "\n");
    bzero(req[(*WriteIndex)], 512);
    strcpy(req[(*WriteIndex)], store);
    
    /*Cache Pages*/
     bzero(cache, MAX_SIZE);
     strcpy(cache, webuf);
     
     /*Cache URL*/
     bzero(urls[(*WriteIndex)], 512);
     strcpy(urls[(*WriteIndex)], url);
    
    (*WriteIndex)++;
     if((*WriteIndex) >= 5) *WriteIndex = 0;
     
    char cachefile[50];
    strcpy(cachefile, "cache");
    strcat(cachefile, MY_TIME);
    strcat(cachefile, ".txt");
    
     FILE *fp;
     FILE *fp2;
     FILE *fp3;

     fp = fopen("list.txt", "w+");
     fp2 = fopen("urls.txt", "w+");
     fp3 = fopen(cachefile, "w+");
    
     //write to data to apropriate cache file
     for(int PrintIndex = 0; PrintIndex < 5; PrintIndex++){
         fputs(req[PrintIndex], fp);
         fputs(urls[PrintIndex], fp2);
     }

    fputs(cache, fp3);
    
    /*close all cache files*/
     fclose(fp);
     fclose(fp2);
     fclose(fp3);
}

//This function checks for code 200 to cache the file
bool checkCode(char response[MAX_SIZE]){
    
    bool OK = false;
    char code[] = "200";
    
    char *token = strtok(response, " ");
    
    while(token != NULL){
        if (strcmp(code, token) == 0){
            OK = true;
            break;
        }
        token = strtok(NULL, " ");
    }
    
    return OK;
}

/*Function for reading list of blocked webpages from the text file*/
void readBlocked(char blocked[10][256]){
    
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    fp = fopen("blacklist.txt", "r");
    
    if (fp == NULL){
        printf("No block list\n");
        return;
    }
    
    int i = 0;
    while ((read = getline(&line, &len, fp)) != -1){
        strcpy(blocked[i], line);
        i++;
    }
   

   fclose(fp);
}

/*Function for reading list of cache requests from the text file*/
void readReq(char Req[5][512]){
    
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    fp = fopen("list.txt", "r");
    
    if (fp == NULL){
        return;
    }
    
    int i = 0;
    while ((read = getline(&line, &len, fp)) != -1){
        strcpy(Req[i], line);
        i++;
    }
   

   fclose(fp);
}

/*Function for reading list of cache urls from the text file*/
void readUrls(char URLs[5][512]){
    
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    fp = fopen("urls.txt", "r");
    
    if (fp == NULL){
        return;
    }
    
    int i = 0;
    while ((read = getline(&line, &len, fp)) != -1){
        strcpy(URLs[i], line);
        i++;
    }
   

   fclose(fp);
}

/*Function for reading cache of raw http files from the text file*/
void readCache(char cache[MAX_SIZE], char TIME[50]){
    
    char cachefile[50];
    strcpy(cachefile, "cache");
    strcat(cachefile, TIME);
    strcat(cachefile, ".txt");
    
    FILE * fp;
    char c;

    fp = fopen(cachefile, "r");

    if (fp == NULL){
        return;
    }
    
    int i = 0;
    while ((c = getc(fp)) != EOF){
        cache[i] = c;
        i++;
    }
    
   fclose(fp);
    
    
}

//This function checks for code 200 to cache the file
bool checkBlocked(char blocked[10][256], char hostname[256], char MY_TIME[50]){
    
    bool flag = false;
    char *buffer[1000];
    char *token;
    char startTime[100];
    char endTime[100];
    
    unsigned long long _start;
    unsigned long long _end;
    unsigned long long _current;
    
    int k = 0;
    for (int i = 0; i < 10; i++){
        
        char str[256];
        
        bzero(str, 256);
        strcpy(str, blocked[i]);
        
        token = strtok(str, " ");
        while(token !=NULL)
        {
            buffer[k] = token;
            token = strtok(NULL, " ");
            k++;
        }
        if (strcmp(hostname, buffer[0]) == 0){
            
            strcpy(startTime, buffer[1]);
            strcpy(endTime, buffer[2]);
            sscanf(endTime ,"%[^\n]", endTime);  // get rid \n
            _start = strtoull(startTime, NULL, 10);
            _end = strtoull(endTime, NULL, 10);
            _current = strtoull(MY_TIME, NULL, 10);
            
            if(_current >= _start && _current <= _end)
            {
                printf("blocked ");
                flag = true;
                break;
            }
         
        }
   }
    
    return flag;
}



/*
 NO MORE
 ( ༎ຶ ۝ ༎ຶ )
        ////                ////|||||||\\\\                 ////
       ////                ////         \\\\               ////
      ////                ||||           ||||            ////
     ////                 ||||           ||||           ////
    ////        ||||      ||||           ||||          ////        |
    ||||||||||||||||      ||||           ||||          ||||||||||||||
    ||||||||||||||||      ||||           ||||
                ||||      ||||           ||||
                ||||      ||||           ||||
                ||||      ||||           ||||
                ||||      \\\\           ////
                ||||       \\\\|||||||||////
 */
