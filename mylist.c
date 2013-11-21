#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>


struct node {
  int socket;
  struct node * next;


};

typedef struct{
  struct node *head ;
  pthread_mutex_t lock ;
  pthread_cond_t signal  ; 
}list_t ;

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
}

void print_ll( list_t * list){
   pthread_mutex_lock(&(list-> lock));

   struct node * itr = list -> head ;

   while (itr != NULL ){
     printf("\n socket: %d\n",itr -> socket);
     itr = itr -> next ;
     
   }
   pthread_mutex_unlock(&(list-> lock));
}   

int get_socket( list_t * list){
   pthread_mutex_lock(&(list-> lock));
   struct node * tmp = list -> head ; 
   if ( tmp == NULL){
     return NULL; 
   }else{
     struct node * tmp_next = tmp -> next ;
     list -> head = tmp_next ;
     return tmp -> socket ; 
   }
   


  pthread_mutex_unlock(&(list-> lock));
}
int main (){
  list_t job_list ;
  list_init(& job_list);
  add_head( &job_list, 1);
  add_head(&job_list, 2);
  print_ll(&job_list);
  return 0 ;
}
