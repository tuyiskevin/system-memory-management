#include "my_malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

MyErrorNo my_errno=MYNOERROR;
FreeListNode free_list_head = NULL;

// FreeList management routines
FreeListNode free_list_begin()
{
    if(free_list_head==NULL) {
        FreeListNode head = (FreeListNode)malloc(sizeof (FreeListNode));
        head->size = 0;
        head->flink = NULL;
        free_list_head = head;
    }
    return free_list_head;
}


void * find_node(uint32_t size){
    FreeListNode curr = free_list_begin();
    while(curr!= NULL){
        if(curr->size>=size) return curr;
        curr=curr->flink;
    }
    return NULL;
}

void insert_node(FreeListNode node){
//    printf("inserted node address: %p with size: %i\n",node,node->size);
    FreeListNode curr = free_list_begin();
    //ensuring order is maintained
    while(curr->flink!=NULL && node>curr) curr=curr->flink;
    FreeListNode tmp = curr->flink;
    curr->flink = node;
    node->flink=tmp;
}

void remove_node(FreeListNode node){
    FreeListNode prev=NULL,curr=free_list_begin();
    while (curr!=NULL){
        prev = curr;
        curr=curr->flink;
        if(curr==node){
            prev->flink=curr->flink;
            break;
        }
    }
}

void coalesce_free_list()
{
    FreeListNode curr = free_list_begin();
    while (curr!= NULL){
        char *c_ptr = ((char *)curr-curr->size);
        char *c_ptr2 = (char*)curr->flink;
        if (c_ptr == c_ptr2){
            curr->size = curr->size + curr->flink->size;
            remove_node(curr->flink);
        }
        else  curr = curr->flink;
    }
}

void *my_malloc(uint32_t size)
{
    if(size<=0){
        my_errno=MYENOMEM;
        return NULL;
    }
    void *ptr;
    int chunk_size=(((int)size+7)/8)*8; //ensuring we have multiples of 8
    chunk_size+=CHUNKHEADERSIZE;

    //search the list
    FreeListNode list_chunk = (FreeListNode) find_node(chunk_size);
    int *h_ptr = (int*)list_chunk;
    int avail_chunk;

    if(list_chunk!=NULL){
        avail_chunk=list_chunk->size;
        remove_node(list_chunk);
    }else{
        //making sure we assign the right size
        if  (chunk_size>=8192){
            int break_size = ((chunk_size+8191)/8192)*8192;
            avail_chunk = break_size;
            h_ptr = sbrk(break_size);
        }else{
            avail_chunk = 8192;
            h_ptr = sbrk(8192);
        }
    }
    //the magic number
    int magic_number=15;
    // Accounting for excess chunk
    if((avail_chunk-chunk_size)>=16){
        char *c_ptr = (char *) h_ptr;
        c_ptr += chunk_size;

        FreeListNode new_node = (FreeListNode)c_ptr;
        new_node->size = avail_chunk - chunk_size;
        new_node->flink = NULL;
        insert_node(new_node);
    }
    // store chunk size, magic number,
    int *tmp = (int *)h_ptr;
    *tmp = chunk_size;
    tmp++;
    *tmp = magic_number;
    tmp++;
    ptr = (void *)tmp;
    return ptr;

}

void my_free(void *ptr)
{
    if (ptr == NULL){
        my_errno = MYBADFREEPTR;
        return;}
    int *tmp_ptr= (int *)ptr;
    tmp_ptr -= 2;
    int chunk = *tmp_ptr;
    tmp_ptr += 1;
    int magic_number = *tmp_ptr;

    if (magic_number != 15){
        my_errno = MYBADFREEPTR;
        return;}

    int list_m_number = 14;
    *tmp_ptr = list_m_number;
    tmp_ptr -= 1;

    FreeListNode node = (FreeListNode)tmp_ptr;
    node->size = chunk;
    node->flink = NULL;
    insert_node(node);
}