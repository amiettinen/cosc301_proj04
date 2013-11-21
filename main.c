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

#include "network.h"


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
  //  list_t * job_list = (list_t*) arg; 
  while(1){
  

  pthread_mutex_lock(&(job_list->lock));
  // This pthread is now busy... I'll need to indicate that somehow?
  while(test==0){
    pthread_cond_wait(&(job_list->signal),&(job_list->lock));
  }
  test = 0;

  int sock = * (int *)pop_head( job_list);
  getreq = getrequest( sock, reqbuffer, buffsize);
  bytes_written = 0;
  if (getreq < 0){
    fprintf(stderr,"Had no request in poll, not sure how to deal with this./n");
  }
  else{
    struct stat file_stat;

    if(stat(reqbuffer,&file_stat)<0){
      senddata(sock, HTTP_404, strlen(HTTP_404));

      //write to log an http error
    }
    else{
      readme = fopen(reqbuffer,"r");
      file_size = file_stat.st_size;
      
      //      char * file_buffer = (char*) malloc (sizeof(char)*1024);
      
    bytes_written = senddata(sock, reqbuffer, buffsize);
    //write out file output
    //write to log request and size

    }
  }
  close(sock); 
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



	    // In worker thread, call shut_down() on sock.
	    pthread_mutex_lock(&(job_list->lock));
	    add_head(job_list, new_sock); //add sock to job queue
	    pthread_cond_signal(&(job_list->signal));
	    test = 1;
	    pthread_mutex_unlock(&(job_list->lock));

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
