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
    int ret;
    ret = validargs(argc, argv);
    if(ret == 0){
        if(global_options & 1){
            USAGE(*argv, EXIT_SUCCESS);
        }
        else if(global_options & 4){
            if(decompress(stdin, stdout) == EOF){
                exit(EXIT_FAILURE);
            }
            else exit(EXIT_SUCCESS);
        }
        else if(global_options & 2){
            int g = global_options;
            g = g & 0xffff0000;
            g = g >>16;
            if(compress(stdin, stdout, (g*1024)) == EOF){
                exit(EXIT_FAILURE);
            }
            else exit(EXIT_SUCCESS);
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
