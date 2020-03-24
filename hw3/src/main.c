#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();

/*    double* ptr1 = sf_malloc(sizeof(double));
    double* ptr2= sf_malloc(sizeof(int));
    double* ptr3= sf_malloc(50);
    double* ptr4= sf_malloc(124);
    double* ptr5= sf_malloc(4);
    double* ptr6= sf_malloc(128);

    sf_show_heap();
    printf("%s\n", "XXX" );
    printf("%s\n", "XXX" );

    ptr1= sf_realloc(ptr1,sizeof(int));
    ptr2= sf_realloc(ptr2,10*sizeof(int));
    ptr3= sf_realloc(ptr3,60);
    ptr4= sf_realloc(ptr4,116);
    ptr5= sf_realloc(ptr5,4);
    ptr6= sf_realloc(ptr6,20);

    sf_show_heap();
    printf("%s\n", "YYY" );
    printf("%s\n", "YYY" );

    sf_free(ptr1);
    sf_free(ptr2);
    sf_free(ptr3);
    sf_free(ptr4);
    sf_free(ptr5);
    sf_free(ptr6);

    sf_show_heap();*/
    /*void *x = sf_malloc(sizeof(double) * 8);

    sf_show_heap();
    printf("%s\n", "YYY" );
    printf("%s\n", "YYY" );

    void *y = sf_realloc(x, sizeof(int));
    printf("%p%p\n",x, y );*/

    /*void *y = sf_malloc(1984);
    void *x = sf_memalign(sizeof(double), 128);
    printf("%p %p\n", x,y);*/

    size_t size = 1;
    size_t align = 8192;
    void *a = sf_memalign(size, align);
    void *b = sf_memalign(size, align);
    void *c = sf_memalign(size, align);
    void *d = sf_memalign(size, align);
    void *e = sf_memalign(size, align);
    void *f = sf_memalign(size, align);

    printf("%p%p%p%p%p%p\n",a,b,c,d,e,f );

    sf_show_heap();

    sf_mem_fini();

    return EXIT_SUCCESS;
}
