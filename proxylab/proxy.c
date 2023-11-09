#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE_NUM 10

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *con_hdr = "Connection: close\r\n";
static const char *proxy_con_hdr = "Proxy-Connection: close\r\n";

struct sURL{  /* URL structure */
    char host[MAXLINE];
    char port[MAXLINE];
    char path[MAXLINE];
};

struct CacheEntry{  /* Cache Entry */
    int valid;  /* valid bit */
    int time;  /* past time from last use */
    char key[MAXLINE];  /* index(URL) */
    size_t bodylen;  /* body's length */
    char body[MAX_OBJECT_SIZE];  /* body */
};

struct rwlock_t{
    sem_t mutex;  
    sem_t w;  /* writer lock */
    int readers;  /* number of readers */
};

struct CacheEntry Cache[MAX_CACHE_NUM];
struct rwlock_t *rw;

void init_global();
void *thread(void* v);
void doit(int fd);
void parse_url(char * url, struct sURL * su);
void gethttpinfo(rio_t *rio, struct sURL *su, char *httpinfo);
int read_cache(int fd, char * key);
void write_cache(char * buf, size_t len, char * key);

int main(int argc, char * argv[])
{
    printf("%s", user_agent_hdr);
    rw = Malloc(sizeof(struct rwlock_t));
    pthread_t tid;
    int listenfd, *connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char hostname[MAXLINE], port[MAXLINE];

    if(argc != 2){
        fprintf(stderr, "usage:%s\n", argv[0]);
        exit(1);
    }

    Signal(SIGPIPE, SIG_IGN);
    init_global();  /* initialize */
    listenfd = Open_listenfd(argv[1]);
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        // printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, thread, connfd);
    }

    return 0;
}

void init_global(){
    /* init lock */
    rw->readers = 0;
    Sem_init(&rw->mutex, 0, 1);
    Sem_init(&rw->w, 0, 1);

    /* init cache */
    for(int i = 0; i < MAX_CACHE_NUM; i++){
        Cache[i].valid = 0;
        Cache[i].time = 0;
        memset(Cache[i].key, 0, sizeof(Cache[i].key));
        memset(Cache[i].body, 0, sizeof(Cache[i].body));
        Cache[i].bodylen = 0;
    }
}

void *thread(void * v){
    int fd = *(int *)v;
    Pthread_detach(Pthread_self());
    Free(v);
    doit(fd);
    Close(fd);
    return NULL;
}

void doit(int fd){
    char buf[MAXLINE],
        method[MAXLINE],
        url[MAXLINE],
        version[MAXLINE],
        tmpurl[MAXLINE],
        httpinfo[MAXLINE];
    struct sURL su;
    rio_t rio, serv_rio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);

    sscanf(buf, "%s %s %s", method, url, version);
    strcpy(tmpurl, url);

    if(strcmp(method, "GET") != 0){
        printf("Method %s cannot be handled.\n", method);
        return;
    }

    if(read_cache(fd, tmpurl) != 0) return;  /* cache hit */

    parse_url(url, &su);
    gethttpinfo(&rio, &su, httpinfo);

    int server_fd = Open_clientfd(su.host, su.port); /* connect to server */
    Rio_readinitb(&serv_rio, server_fd);
    Rio_writen(server_fd, httpinfo, strlen(httpinfo));

    size_t n = 0;
    size_t totlen = 0;  /* total length of the body */
    char cachebuf[MAX_OBJECT_SIZE];
    while((n = Rio_readnb(&serv_rio, buf, MAXLINE)) != 0){
        Rio_writen(fd, buf, n);
        totlen += n;
        if(totlen > MAX_OBJECT_SIZE){
            continue;
        }            
        memcpy(cachebuf + totlen - n, buf, n);   
    }

    if(totlen <= MAX_OBJECT_SIZE){  /* can be put into cache */
        write_cache(cachebuf, totlen, tmpurl);
    }
    Close(server_fd);
}

/* possible URL:
 * case 1: /home.html
 * case 2: http://www.XXX.com:PORT/home.html
 */
void parse_url(char * url, struct sURL * su){
    char * sep = strstr(url, "//");
    if(sep == NULL){  /* case 1 */
        sep = strstr(url, "/");
        if(sep != NULL){
            strcpy(su->path, sep);
        }
        strcpy(su->port, "80");  /* default port is 80 */
        return;
    }
    else{  /* case 2 */
        char * portsep = strstr(sep + 2, ":");
        if(portsep != NULL){  /* port given */
            char * pathsep = strstr(portsep + 1, "/");
            strcpy(su->path, pathsep);  /* read path */
            *pathsep = '\0';
            strcpy(su->port, portsep + 1);
            *portsep = '\0';
        }
        else{  /* no port given, use default */
            char * pathsep = strstr(sep + 2, "/");
            if(pathsep != NULL){
                strcpy(su->path, pathsep);
                strcpy(su->port, "80");
                *pathsep = '\0';
            }
        }
        strcpy(su->host, sep + 2);  /* copy hostname */
    }
}

void gethttpinfo(rio_t *rio, struct sURL *su, char *httpinfo){
    char buf[MAXLINE],
        reqline[MAXLINE],
        hosthdr[MAXLINE],
        otherinfo[MAXLINE];
    
    sprintf(reqline, "GET %s HTTP/1.0\r\n", su->path);

    while(Rio_readlineb(rio, buf, MAXLINE) > 0){
        if(strcmp(buf, "\r\n") == 0){
            strcat(otherinfo, "\r\n");
            break;
        }
        else if(strncasecmp(buf, "Host:", 5) == 0){
            strcpy(hosthdr, buf);
        }
        else if(strncasecmp(buf, "Connection:", 11) 
            && strncasecmp(buf, "Proxy-Connection:", 17)
            && strncasecmp(buf, "User-Agent:", 11)){
            strcat(otherinfo, buf);
        }
    }
    if(strlen(hosthdr) == 0){
        sprintf(hosthdr, "Host: %s\r\n", su->host);
    }
    sprintf(httpinfo, "%s%s%s%s%s%s", 
    reqline, hosthdr, con_hdr, proxy_con_hdr, user_agent_hdr, otherinfo);
}

int read_cache(int fd, char * key){
    /* lock */
    P(&rw->mutex);
    if(rw->readers == 0){
        P(&rw->w);
    }
    rw->readers++;
    V(&rw->mutex);

    /* read cache */
    int flag = 0;  /* if hit */
    for(int i = 0; i < MAX_CACHE_NUM; i++){
        if(Cache[i].valid == 1 && strcmp(key, Cache[i].key) == 0){
            Rio_writen(fd, Cache[i].body, Cache[i].bodylen);
            Cache[i].time = 0;
            flag = 1;
            break;
        }
    }
    for(int i = 0; i < MAX_CACHE_NUM; i++){
        if(Cache[i].valid == 1){
            Cache[i].time++;
        }
    }
    /* read cache end */

    /* unlock */
    P(&rw->mutex);
    rw->readers--;
    if(rw->readers == 0){
        V(&rw->w);
    }
    V(&rw->mutex);

    return flag;
}

void write_cache(char * buf, size_t len, char * key){
    /* lock */
    P(&rw->w);

    /* write cache */
    int id;
    int maxtime = -1;
    for(int i = 0; i < MAX_CACHE_NUM; i++){
        if(Cache[i].valid == 0){
            Cache[i].valid = 1;
            id = i;
            break;
        }
        else if(Cache[i].time > maxtime){
            maxtime = Cache[i].time;
            id = i;
        }
    }

    /* update cache information */
    Cache[id].time = 0;
    strcpy(Cache[id].key, key);
    memcpy(Cache[id].body, buf, len);
    Cache[id].bodylen = len;

    for(int i = 0; i < MAX_CACHE_NUM; i++){
        if(Cache[i].valid == 1){
            Cache[i].time++;
        }
    }

    /* unlock */
    V(&rw->w); 
}
