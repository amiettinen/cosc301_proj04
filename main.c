#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <assert.h>
#include "network.h"
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void list_init(list_t *list){
  list-> head = NULL;

  int rc = pthread_mutex_init(&(list->lock),NULL);
  assert(rc== 0);
  pthread_cond_init(&(list -> signal) ,NULL );


}

void add_head ( list_t * list, int sock){
  pthread_mutex_lock(&(list-> lock));

  struct node * new_node = (struct node * ) malloc(sizeof(struct node));
  new_node -> socket = sock;
  if (! new_node){
    printf("\n Didn' malloc\n");
    abort();
  }

  struct node * tmp =  list -> head  ;
  if ( tmp !=NULL ){
    new_node -> next  = tmp ;
    list-> head  = new_node ;
    new_node -> next = tmp ;
  }else{
    new_node ->next = NULL;
    list-> head = new_node ;
  }
  pthread_mutex_unlock(&(list-> lock));
  return ; 
}

void * pop_head( list_t * list){
  //  pthread_mutex_lock(&(list-> lock));
  struct node * tmp = list -> head ;
  if ( tmp == NULL){
    return NULL ;
  }else{
    struct node * tmp_next = tmp -> next ;
    list -> head = tmp_next ;
    return (void * )&(tmp -> socket );
  }

  //  pthread_mutex_unlock(&(list-> lock));
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// global variable; can't be avoided because
// of asynchronous signal interaction
int still_running = TRUE;
void signal_handler(int sig) {
    still_running = FALSE;
}


//Will need to lock mutex around the job queue manipulation

//struct node * head = NULL;
//struct node * tail = NULL;
list_t jobs; 
list_t* job_list= &jobs;
//list_init(job_list);

FILE * weblog;

//pthread_cond_t signal;
//pthread_cond_init(signal);

int test = 0;

void usage(const char *progname) {
    fprintf(stderr, "usage: %s [-p port] [-t numthreads]\n", progname);
    fprintf(stderr, "\tport number defaults to 3000 if not specified.\n");
    fprintf(stderr, "\tnumber of threads is 1 by default.\n");
    exit(0);
}

void * worker_function( void * arg){
  char* reqbuffer =(char *) malloc(sizeof(char) *1024);
  int buffsize = 1024;
  int getreq;
  int bytes_written;
  int file_size;
  FILE * readme;
  //for logging
  struct sockaddr_in client_address;
  //  socklen_t addr_len;

  while(1){
  
    printf("\nafter while before lock\n");
  pthread_mutex_lock(&(job_list->lock));
  // This pthread is now busy... I'll need to indicate that somehow?
  while(test==0){
    pthread_cond_wait(&(job_list->signal),&(job_list->lock));
  }
  test = 0;
   
   //before sock 
  int sock = * ((int *)pop_head( job_list));
  //after sock 
  
  getreq = getrequest( sock, reqbuffer, buffsize);
  //  printf("\n%s\n", reqbuffer);
  bytes_written = 0;
  if (getreq < 0){
    
    fprintf(stderr,"Had no request in poll, not sure how to deal with this./n");
  }
  else{
    struct stat file_stat;

    if(stat(reqbuffer,&file_stat)<0){
      senddata(sock, HTTP_404, strlen(HTTP_404));
      //write 404 error to log
      time_t now = time(NULL);
      fprintf(weblog, "%s:%d at %s GET  %s 404 %zd\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port),ctime(&now),reqbuffer,strlen(HTTP_404));

  
    }
    else{
      readme = fopen(reqbuffer,"r");
      file_size = file_stat.st_size;
      char * line  = (char*) malloc (sizeof(char)*1024);
      char * content  = (char*) malloc (sizeof(char)*4096);
      content [0]='\0'; 
      //add all the contents of the file to the variable content 
      while (fgets( line, 1024, readme)){
	strcat(content,line ) ;
	}
      //format string for send data
      char * to_send =  (char*) malloc (sizeof(char)*4096);
      sprintf( to_send, "HTTP/1.1 200 OK\r\n Content-Type: text/html\r\n Content-length: %d\r\n \r\n%s",file_size,content); 
      //send data
      bytes_written = senddata(sock, to_send, strlen(to_send));
    printf("\n%s\n", reqbuffer);
      //write to log request and size
      time_t now = time(NULL);
      fprintf(weblog, "%s:%d at %s GET %s 200 %zd\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port),ctime(&now),reqbuffer,strlen(to_send));


    }
  }
  close(sock); 
  fclose(readme);
  
  pthread_mutex_unlock(&(job_list->lock));
}
}

void runserver(int numthreads, unsigned short serverport) {
    //////////////////////////////////////////////////

    // create your pool of threads here - use 10 threads
  weblog = fopen("weblog.txt", "w");
  if (weblog == NULL)
    {
      printf("Error opening file!\n");
      exit(1);
    }

  pthread_t * thread_arr =(pthread_t *) malloc(sizeof(pthread_t)*numthreads); 
  int iterate = 0;
  while(iterate < numthreads){
    pthread_t newthread;
    if(pthread_create(&newthread,NULL, worker_function,job_list) < 0){
      fprintf(stderr, "pthread create failed!\n");
    }
    *(thread_arr+iterate) = newthread;
    iterate +=1 ;
  }
  
    ///////////////////////////////////////////////
     int main_socket = prepare_server_socket(serverport);
    if (main_socket < 0) {
        exit(-1);
    }
    signal(SIGINT, signal_handler);

    struct sockaddr_in client_address;
    socklen_t addr_len;

    fprintf(stderr, "Server listening on port %d.  Going into request loop.\n", serverport);
    while (still_running) {
        struct pollfd pfd = {main_socket, POLLIN};
        int prv = poll(&pfd, 1, 10000);

        if (prv == 0) {
            continue;
        } else if (prv < 0) {
            PRINT_ERROR("poll");
            still_running = FALSE;
            continue;
        }
        
        addr_len = sizeof(client_address);
        memset(&client_address, 0, addr_len);

        int new_sock = accept(main_socket, (struct sockaddr *)&client_address, &addr_len);
        if (new_sock > 0) {
            
             time_t now = time(NULL);
            fprintf(stderr, "Got connection from %s:%d at %s\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), ctime(&now));

           ////////////////////////////////////////////////////////
           /* You got a new connection.  Hand the connection off
            * to one of the threads in the pool to process the
            * request.
            *
            * Don't forget to close the socket (in the worker thread)
            * when you're done.
            */
           ////////////////////////////////////////////////////////


	    printf("\nbefore lock \n");
	    // In worker thread, call shut_down() on sock.



	    add_head(job_list, new_sock); //add sock to job queue

    	    printf("\n %d \n", job_list -> head -> socket);
	    pthread_cond_signal(&(job_list->signal));
	    test = 1;


        }
    }

  iterate = 0;
  pthread_t kill_me ; 
  while(iterate < numthreads){
        
    kill_me = *(thread_arr+iterate) ;
    pthread_join(kill_me, NULL); 
    iterate +=1 ;
  }

    fprintf(stderr, "Server shutting down.\n");
    fclose(weblog);        
    close(main_socket);

}


int main(int argc, char **argv) {
    list_init(job_list);
    unsigned short port = 3000;
    int num_threads = 1;

    int c;
    while (-1 != (c = getopt(argc, argv, "hp:t:"))) {
        switch(c) {
            case 'p':
                port = atoi(optarg);
                if (port < 1024) {
                    usage(argv[0]);
                }
                break;

            case 't':
                num_threads = atoi(optarg);
                if (num_threads < 1) {
                    usage(argv[0]);
                }
                break;
            case 'h':
            default:
                usage(argv[0]);
                break;
        }
    }

    runserver(num_threads, port);
    
    fprintf(stderr, "Server done.\n");
    exit(0);
}
