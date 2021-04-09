//
//  my_malloc-driver.c
//  Lab1: Malloc
//


#include "my_malloc.h"

int main(int argc, const char * argv[]) {
    for(int i=0; i<5; i++){
        my_malloc(3000);
    }
    coalesce_free_list();

}
