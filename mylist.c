#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>


struct node {
  int socket;
  struct node * prev ;


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

void add_head ( list_t * list){
  pthread_mutex_lock(&(list-> lock));
  struct node * new_node = (struct node * ) malloc(sizeof(struct node));
  if (! new_node){
    printf("\n Didn' malloc\n");
    abort();
}
  struct node * tmp =  list -> head  ; 
  new_node = tmp ; 
  list-> head = new_node ; 

   pthread_mutex_unlock(&(list-> lock));
}

void print_ll( list_t * list){
   pthread_mutex_lock(&(list-> lock));
   struct node * itr = list -> head ;
   while (itr != NULL ){
     printf("\n socket: %d\n",itr -> socket);
     itr ++;
     
   }
   pthread_mutex_unlock(&(list-> lock));
}   


int main (){
  
  return 0 ;
}
