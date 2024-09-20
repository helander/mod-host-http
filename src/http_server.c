#include <stdio.h>  // console input/output, perror

#include <stdlib.h> // exit
#include <stdbool.h> // boolean
#include <string.h> // string manipulation
#include <netdb.h>  // getnameinfo

#include <sys/socket.h> // socket APIs
#include <netinet/in.h> // sockaddr_in
#include <unistd.h>     // open, close

#include <signal.h> // signal handling
#include <time.h>   // time

#include <math.h>   // math




#define SIZE 1024  // buffer size

#define BACKLOG 10 // number of pending connections queue will hold

#include <pthread.h>
#include "effects.h"



static void *http_server_run(void *);

pthread_t t_http_server;

/*

rc/http_server.c: I funktion ”http_server_start”:src/http_server.c:35:55: varning: skickar argument 3 till ”pthread_create” från inkompatibel pekartyp [-Wincompatible-pointer-types]   35 |         int k = pthread_create (&t_http_server, NULL, http_server_run, NULL);      |                                                       ^~~~~~~~~~~~~~~
      |                                                       |
      |                                                       void * (*)(void)
I filen inkluderad ifrån src/http_server.c:24:/usr/include/pthread.h:204:36: anm: ”void * (*)(void *)” förväntades men argumentet har typ ”void * (*)(void)”  204 |                            void *(*__start_routine) (void *),      |                            ~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~
c
*/


int http_server_start(void) {

        int k = pthread_create (&t_http_server, NULL, http_server_run, NULL);
        if (k != 0) {
        fprintf (stderr, "%d : %s\n", k, "pthread_create : HTTPServer thread");
                return (1);
        }
	return (0);
}

void http_server_stop(void) {
   pthread_join (t_http_server, NULL);
}



void handleSignal(int signal);


int serverSocket;
int clientSocket;

char *request;

const char *OKPrebody   =  "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
const char *BadRequest  =  "HTTP/1.1 400 Bad request\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
const char *NoResource  =  "HTTP/1.1 404 No such resource\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
const char *WrongMethod =  "HTTP/1.1 405 Method not allowed\r\nAccess-Control-Allow-Origin: *\r\n\r\n";


void *http_server_run(void *)
{

  signal(SIGINT, handleSignal);

  int serverPort = 7777;
  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;                     // IPv4
  serverAddress.sin_port = htons(serverPort);                   // port number in network byte order (host-to-network short)
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // localhost (host to network long)

  serverSocket = socket(AF_INET, SOCK_STREAM, 0);

  setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

  if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  {
    printf("Error: Fail to bind to port %d.\n",serverPort);
    return NULL;
  }

  if (listen(serverSocket, BACKLOG) < 0)
  {
    printf("Error: The server is not listening.\n");
    return NULL;
  }

//  char hostBuffer[NI_MAXHOST], serviceBuffer[NI_MAXSERV];
//  int error = getnameinfo((struct sockaddr *)&serverAddress, sizeof(serverAddress), hostBuffer,
//                          sizeof(hostBuffer), serviceBuffer, sizeof(serviceBuffer), 0);

//  if (error != 0)
//  {
//    printf("Error: %s\n", gai_strerror(error));
//    return NULL;
//  }

  printf("\nServer is listening on port %d/\n\n",serverPort);
  fflush(stdout);

  while (1)
  {
    request = (char *)malloc(SIZE * sizeof(char));
    char method[10], route[300];
    char *resource = NULL, *instance = NULL, *entityType = NULL, *entityName = NULL, *entityValue = NULL;
    char response[50000];

    clientSocket = accept(serverSocket, NULL, NULL);
    read(clientSocket, request, SIZE);

    sscanf(request, "%s %s", method, route);

    char *token;

    token = strtok(route, "/");
    if (token != NULL) {
         resource = token;
         token = strtok(NULL, "/");
         if (token != NULL) {
            instance = token;
            token = strtok(NULL, "/");
            if (token != NULL) {
               entityType = token;
               token = strtok(NULL, "/");
               if (token != NULL) {
                  entityName = token;
                  token = strtok(NULL, "/");
                  if (token != NULL) {
                     entityValue = token;
                  }
               }
            }
         }
    }

    free(request);
    {

      if (resource == NULL) {
         send(clientSocket, NoResource, strlen(NoResource), 0);
      } else if (strcmp(resource,"effect") == 0) {
         if (instance != NULL) {
           if (strcmp(method, "GET") == 0) {
             http_effect_get(atoi(instance),response);
             send(clientSocket, OKPrebody, strlen(OKPrebody), 0);
             send(clientSocket, response, strlen(response), 0);
           } else if (strcmp(method, "PUT") == 0) {
             if (entityValue != NULL) {
                http_effect_put(atoi(instance),entityType,entityName,entityValue,response);
                send(clientSocket, OKPrebody, strlen(OKPrebody), 0);
             } else {
                send(clientSocket, NoResource, strlen(NoResource), 0);
             }
           } else {
             send(clientSocket, WrongMethod, strlen(WrongMethod), 0);
           }
         } else {
           if (strcmp(method, "GET") == 0) {
             http_effect_instances(response);
             send(clientSocket, OKPrebody, strlen(OKPrebody), 0);
             send(clientSocket, response, strlen(response), 0);
           } else {
             send(clientSocket, WrongMethod, strlen(WrongMethod), 0);
           }
         }
      } else {
         send(clientSocket, NoResource, strlen(NoResource), 0);
      } 
    }
    close(clientSocket);
    printf("\n");
  }
  return NULL;
}


void handleSignal(int signal)
{
  if (signal == SIGINT)
  {
    printf("\nShutting down server...\n");

    close(clientSocket);
    close(serverSocket);

    if (request != NULL)
      free(request);

    exit(0);
  }
}
