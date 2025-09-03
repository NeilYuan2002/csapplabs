#include <stdio.h>
#include "csapp.h"
#include <pthread.h>

pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_BLOCK_NUM 10

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* server information is in this struct*/
typedef struct {
  char serverHost[MAXLINE];
  char serverPort[10];
  char fileName[MAXLINE];
} UriInfo;

/*def of cache */
typedef struct {
  int valid;
  char uri[MAXLINE];
  char object[MAX_OBJECT_SIZE];
  int size; //size of object
  int priority;
} cacheBlock;

typedef struct {  
  cacheBlock blocks[CACHE_BLOCK_NUM];
} cache;

/* function prototypes */
void doIt(int clientfd);
void readRequestHdrs(rio_t *rp);
UriInfo parseUri(char *uri);
/* void getFileType(char *fileName, char *fileType); */
void clientError(int fd, char *cause, char *errNum, char *shortMsg, char *loneMsg);
void forwardRequest(int serverfd, char *serverHost, char *fileName, char *method);
void readFromServer(int serverfd, int clientfd, cache *cp, const char *uri);
void sigchld_handler(int sig);
int hitCheck(cache c, char *uri);
cacheBlock *getCacheBlock(cache* cp, char *uri);
void cacheReply(cacheBlock *bptr, int clientfd);
void initCache(cache *cp);
void replaceBlock(cacheBlock *bptr, const char *uri,const char *object, int size);
void *thread(void *vargp);
void cacheInsert(cache *cp,const  char *uri,const char *object, int size);

/* Globle */
int lru_clock = 0; //maintain a LRU timestamp
cache myCache;

/*
 * In this web proxy accepts requests from client(browser), it will create a new thread, and  * forwards these requests to the end server.
 * When the end server replies to the proxy, the proxy sends the * reply on to the browser.
 */
int main(int argc, char **argv)
{
  int listenfd, *clientfdp;
  char clientHost[MAXLINE],clientPortStr[MAXLINE];
  socklen_t clientLen;
  struct sockaddr_storage clientAddr;
  pthread_t tid;
  
  
  /* Check command line args*/
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n",argv[0]);
    exit(1);
  }  
  listenfd = open_listenfd(argv[1]); //argv[1] is clientPort
  if (listenfd < 0) {
    perror("open_listenfd failed");
    exit(1);
  }
  initCache(&myCache);
  
  while (1) {
    clientLen = sizeof(struct sockaddr_storage);
    clientfdp = malloc(sizeof(int));
    *clientfdp = Accept(listenfd, (SA *)&clientAddr, &clientLen);
    /* clientfd and connfd are the same */
    Getnameinfo((SA *)&clientAddr, clientLen, clientHost, MAXLINE,
		clientPortStr, MAXLINE, 0);
    printf("Connected to (%s, %s)\n",clientHost, clientPortStr);
    pthread_create(&tid, NULL, thread, clientfdp);
  }
  printf("%s", user_agent_hdr);
  exit(0);            
}

void *thread(void *vargp) {
  int clientfd = *((int *)vargp);
  pthread_detach(pthread_self());
  free(vargp);
  doIt(clientfd);
  Close(clientfd);
  
  return NULL;
}

/*
 * Sends requests to server, and replies to client.
 */
void doIt(int clientfd) {
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  UriInfo info;
  rio_t rio;
  cacheBlock *bptr = NULL;
  
  /* Read request line and headers */
  Rio_readinitb(&rio, clientfd);
  Rio_readlineb(&rio, buf, MAXLINE);

  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET")) {
    clientError(clientfd, method, "501", "Not implemented",
		"Proxy does not implement this method");
    return;
  }
  readRequestHdrs(&rio);

  /* Parse URI from GET request */
  info = parseUri(uri);
  if (strlen(info.serverHost) == 0) {
    clientError(clientfd, uri, "400", "Bad Request",
                "Could not parse URI");
    return;
  }
        
  pthread_mutex_lock(&cache_mutex);
  if (hitCheck(myCache, uri)) {    
    /* send object from cacheBlock */    
    bptr = getCacheBlock(&myCache, uri);    
  }
  pthread_mutex_unlock(&cache_mutex);

  if (bptr != NULL) {
    cacheReply(bptr, clientfd);
    
    return;
  } else {
    /* miss, forwards to the server */
    int serverfd = open_clientfd(info.serverHost,info.serverPort);
    if (serverfd < 0) {
      clientError(clientfd, info.serverHost, "502", "Bad Gateway",
		  "Proxy failed to connect to end server");
      return;
    }

    forwardRequest(serverfd, info.serverHost, info.fileName, method);
    
    /* hitcheck miss, so insert a new cacheblock into cache*/
    readFromServer(serverfd, clientfd, &myCache, uri);
  }
  
}



void readRequestHdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

/*
 * Parse serverHost, serverPort, filename from URI (packed into a struct).
 */
UriInfo parseUri(char *uri) {
  UriInfo info;
  char *ptr, *hostHeadPtr, *fileName, *portPtr;
  int hostLen;  
  
  /* Init default values */
  strcpy(info.serverPort, "80");
  strcpy(info.fileName, "/");
  info.serverHost[0] = '\0';
  
  ptr = strstr(uri, "://");
  if (ptr == NULL) {
    printf("Invalid URI: %s\n", uri);
    return info;
  }
  ptr += 3; //ignore "://"
  hostHeadPtr = (char *) ptr;
  fileName = strstr(ptr, "/");
  
  /* Get filename (or say, file path) */
  if (fileName != NULL) {
    strcpy(info.fileName, fileName);
    hostLen = fileName - hostHeadPtr;
  } else {
    info.fileName[0] = '/';
    info.fileName[1] = '\0';
    hostLen = strlen(hostHeadPtr);
  }

  /* Get host:port */
  char hostPort[hostLen + 1];
  strncpy(hostPort, hostHeadPtr, hostLen);
  hostPort[hostLen] = '\0';

  /* Get port */
  portPtr = strstr(hostPort, ":");
  if (portPtr != NULL) {
    *portPtr = '\0';
    strcpy(info.serverHost, hostPort);
    strcpy(info.serverPort, portPtr + 1);//ignore ':'  
  } else {
    strcpy(info.serverHost, hostPort); //if no ":port", then hostPort is only hostname
  }
  
  return info;
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clientError */
void clientError(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clientError */

/*
 * Forward request to server
 */
void forwardRequest(int serverfd, char *hostName, char *fileName, char *method) {
  char buf[MAXLINE];
  /* Constructs forward header */
  sprintf(buf, "%s %s HTTP/1.0\r\n", method, fileName);
  sprintf(buf + strlen(buf), "Host: %s\r\n", hostName);
  sprintf(buf + strlen(buf), "%s", user_agent_hdr);
  sprintf(buf + strlen(buf), "Proxy-Connection: close\r\n");
  sprintf(buf + strlen(buf), "\r\n");  

  Rio_writen(serverfd, buf, strlen(buf));       
}


/*
 *  Reads each line from the server, and sends it back to the client,insert into cache.
 */
void readFromServer(int serverfd, int clientfd, cache *cp, const char *uri) {
    rio_t rio;
    char buf[MAXLINE];
    char object_buf[MAX_OBJECT_SIZE];
    ssize_t n;
    size_t total_size = 0;

    Rio_readinitb(&rio, serverfd);

    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) > 0) {
        Rio_writen(clientfd, buf, n);

        // 如果缓存大小未超限，则复制到 object_buf
        if (total_size + n <= MAX_OBJECT_SIZE) {
            memcpy(object_buf + total_size, buf, n);
        }
        total_size += n;
    }

    // 如果内容太大，不缓存
    if (total_size > MAX_OBJECT_SIZE)
        return;

    // 插入缓存（线程安全）
    pthread_mutex_lock(&cache_mutex);
    cacheInsert(cp, uri, object_buf, total_size);
    pthread_mutex_unlock(&cache_mutex);
}

/*
 *  sends obeject from cacheblock to the client.
 */
void cacheReply(cacheBlock *bptr, int clientfd) { 
  printf("Cache hit: serving URI %s from cache\n", bptr->uri);
  Rio_writen(clientfd, bptr->object, bptr->size);    
}

void sigchld_handler(int sig) {
  while (waitpid(-1, 0, WNOHANG) > 0)
    ;
  return;
}


int hitCheck(cache c, char *uri){
  
  for (int i = 0; i < CACHE_BLOCK_NUM; i++) {
        if (c.blocks[i].valid && strcmp(c.blocks[i].uri, uri) == 0) {
            return 1;  
        }
    }
  return 0;
  
}

void initCache(cache *cp) {
  for (int i = 0; i < CACHE_BLOCK_NUM; i++) {
    cp->blocks[i].valid = 0;
    cp->blocks[i].size = 0;
    cp->blocks[i].priority = 0;
    cp->blocks[i].uri[0] = '\0';
  }
}
cacheBlock *getCacheBlock(cache *cp, char *uri) {
    for (int i = 0; i < CACHE_BLOCK_NUM; i++) {
        if (cp->blocks[i].valid && strcmp(cp->blocks[i].uri, uri) == 0) {
            cp->blocks[i].priority = ++lru_clock; 
            return &cp->blocks[i];
        }
    }
    return NULL;
}

void replaceBlock(cacheBlock *bptr,const char *uri,const char *object, int size) {
  strcpy(bptr->uri, uri);
    memcpy(bptr->object, object, size); 
    bptr->size = size;
    bptr->valid = 1;
    bptr->priority = ++lru_clock;  
}

void cacheInsert(cache *cp,const char *uri,const char *object, int size) {
    int min_priority = __INT_MAX__;
    int victim = -1;

    for (int i = 0; i < CACHE_BLOCK_NUM; i++) {
        if (!cp->blocks[i].valid) {
            victim = i;
            break;  // 空块优先
        }
        if (cp->blocks[i].priority < min_priority) {
            min_priority = cp->blocks[i].priority;
            victim = i;
        }
    }

    if (victim != -1) {
        replaceBlock(&cp->blocks[victim], (char *)uri, (char *)object, size);
    }
}




















