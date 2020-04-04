#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();

    sf_malloc(16);

    sf_mem_fini();

    return EXIT_SUCCESS;
}
