#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* server information is in this struct*/
typedef struct {
  char serverHost[MAXLINE];
  char serverPort[10];
  char fileName[MAXLINE];
} UriInfo;

/* function prototypes */
void doIt(int clientfd);
void readRequestHdrs(rio_t *rp);
UriInfo parseUri(char *uri);
/* void getFileType(char *fileName, char *fileType); */
void clientError(int fd, char *cause, char *errNum, char *shortMsg, char *loneMsg);
void forwardRequest(int serverfd, char *serverHost, char *fileName, char *method);
void acceptReply(int serverfd, int clientfd);


/*
 * In this web proxy accepts requests from client(browser), and  * forwards these requests to the end server.
 * When the end server replies to the proxy, the proxy sends the * reply on to the browser.
 */
int main(int argc, char **argv)
{
  int listenfd,clientfd;
  char clientHost[MAXLINE],clientPort[10];
  socklen_t clientLen;
  struct sockaddr_storage clientAddr;
  
  /* Check command line args*/
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n",argv[0]);
    exit(1);
  }
  strcpy(clientPort, argv[1]);
  listenfd = open_listenfd(clientPort);
  if (listenfd < 0) {
    perror("open_listenfd failed");
    exit(1);
  }
  
  while (1) {
    clientLen = sizeof(struct sockaddr_storage);
    clientfd = Accept(listenfd, (SA *)&clientAddr, &clientLen);
    /* clientfd and connfd are the same */
    Getnameinfo((SA *)&clientAddr, clientLen, clientHost, MAXLINE,
		clientPort, MAXLINE, 0);
    printf("Connected to (%s, %s)\n",clientHost, clientPort);
    doIt(clientfd);
    Close(clientfd);
  }
  printf("%s", user_agent_hdr);
  exit(0);            
}

/*
 * Sends requests to server, and replies to client.
 */
void doIt(int clientfd) {
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  UriInfo info;
  rio_t rio;
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

  int serverfd = open_clientfd(info.serverHost,info.serverPort);
  if (serverfd < 0) {
    clientError(clientfd, info.serverHost, "502", "Bad Gateway",
                "Proxy failed to connect to end server");
    return;
}

  forwardRequest(serverfd, info.serverHost, info.fileName, method);
  
  acceptReply(serverfd, clientfd);
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
 *  Reads each line from the server, and sends it back to the client.
 */
void acceptReply(int serverfd, int clientfd) {
  rio_t rio;
  char buf[MAXLINE];
  ssize_t n;

  Rio_readinitb(&rio, serverfd);
  
  while((n = Rio_readlineb(&rio, buf, MAXLINE)) > 0) {
      Rio_writen(clientfd, buf, n);
    }
}


























