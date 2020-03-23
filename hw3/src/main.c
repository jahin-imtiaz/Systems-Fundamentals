#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();

    printf("%s%lu\n", "SIZE OF DOUBLE: ", sizeof(double));
    double* ptr1 = sf_malloc(sizeof(double));
    double* ptr2= sf_malloc(sizeof(int));
    double* ptr3= sf_malloc(50);
    double* ptr4= sf_malloc(124);
    double* ptr5= sf_malloc(4);
    double* ptr6= sf_malloc(128);

    sf_show_heap();
    printf("%s\n", "XXX" );
    sf_show_free_lists();
    printf("%s\n", "XXX" );
    sf_show_blocks();
    printf("%s\n", "XXX" );
/*   *ptr = 320320320e-320;

    printf("%f\n", *ptr);*/


    sf_free(ptr1);
    sf_free(ptr2);
    sf_free(ptr3);
    sf_free(ptr4);
    sf_free(ptr5);
    sf_free(ptr6);

    sf_show_heap();
    printf("%s\n", "ZZZ" );
    sf_show_free_lists();
    printf("%s\n", "ZZZ" );
    sf_show_blocks();

    sf_mem_fini();

    return EXIT_SUCCESS;
}
