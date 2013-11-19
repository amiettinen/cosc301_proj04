#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

//Include error.h

struct node {
  int socket;
  struct node * prev = NULL;
  struct node * next = NULL;
  
};

typdef struct{
  struct node *head ;
  struct node *tail ; 
  pthread_mutext_t lock ; 
  
}list_t ; 


void list_init(list_t *list){
  list-> head = NULL;
  list -> tail = NULL; 
  int rc = pthread_mutex_init(&(list->lock),NULL);
  assert(rc== 0);


}

void add_head(list_t *list, int sock){
  pthread_mutex_lock(&(list->lock));

  struct node * new_node = (struct node *) malloc(sizeof(struct node));
  if (!new_node){
    printf(stderr,"\nDidn't malloc\n");
    abort();
  }
  new_node -> socket = sock;
  new_node -> next = NULL; 
  new_node -> prev = NULL; 
  //if the list is empty
  if (list->head == NULL){
    list -> head = new_node ; 
    list -> tail = new_node ; 
    
  }else{
   struct node * tmp = list -> head ;
   list -> head = new_node ;
   new_node -> next = tmp ; 

  }
  pthread_mutex_unlock(&(list->lock));
  
 }

void remove_tail(list_t * list ){
  pthread_mutex_lock(&(list->lock));

  struct node * tmp = list -> tail; 
  tmp =   tmp -> prev ;
  list -> tail = tmp ; 

  pthread_mutex_unlock(&(list->lock));
  
  

}

void print_list (list_t * list){
  pthread_mutex_lock(&(list->lock));
  struct node * itr = list -> head ;
  
  while ( itr !=NULL){
    printf("\nsocket: %d\n", itr->socket);
  }

  pthread_mutex_unlock(&(list->lock));

}
  
int main(){
  
  exit(0);
}
