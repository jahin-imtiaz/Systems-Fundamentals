#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    /*int ret;
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    debug("Options: 0x%x", global_options);
    if(global_options & 1)
        USAGE(*argv, EXIT_SUCCESS);*/
    int ret;
    ret = validargs(argc, argv);
    if(ret == 0){
        if(global_options & 1){
            USAGE(*argv, EXIT_SUCCESS);
        }
        else if(global_options & 4){
            decompress(stdin, stdout);
            exit(EXIT_SUCCESS);
        }
        else if(global_options & 2){
            int g = global_options;
            g = g & 0xff00;
            g = g >>16;
            compress(stdin, stdout, g);
            exit(EXIT_SUCCESS);
        }
    }
    else if(ret == -1){
        USAGE(*argv, EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
