#include "const.h"
#include "sequitur.h"
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

#define SOT 0x81
#define SOB 0x83
#define RD  0x85
#define EOB 0x84
#define EOT 0x82

int isDigit(char c);
SYMBOL *createRule(int ci, FILE *in, int *pc);
int getByteCount(int i);
int getValue(int i, int byteCount, FILE *in);
void expand_rules(SYMBOL *rule, FILE *out);
int validUTF(int c);
int notMarker(int c);
void expand_blocks(SYMBOL *rule_head, FILE *out);
void print_UTF_value(int v, FILE *out);
/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

/**
 * Main compression function.
 * Reads a sequence of bytes from a specified input stream, segments the
 * input data into blocks of a specified maximum number of bytes,
 * uses the Sequitur algorithm to compress each block of input to a list
 * of rules, and outputs the resulting compressed data transmission to a
 * specified output stream in the format detailed in the header files and
 * assignment handout.  The output stream is flushed once the transmission
 * is complete.
 *
 * The maximum number of bytes of uncompressed data represented by each
 * block of the compressed transmission is limited to the specified value
 * "bsize".  Each compressed block except for the last one represents exactly
 * "bsize" bytes of uncompressed data and the last compressed block represents
 * at most "bsize" bytes.
 *
 * @param in  The stream from which input is to be read.
 * @param out  The stream to which the block is to be written.
 * @param bsize  The maximum number of bytes read per block.
 * @return  The number of bytes written, in case of success,
 * otherwise EOF.
 */
int compress(FILE *in, FILE *out, int bsize) {
    // To be implemented.
    fputc(SOT, out);
    int c;
    while((c=fgetc(in)) != EOF){
        int i;
        init_symbols();
        init_rules();
        init_digram_hash();

        SYMBOL *m_rule = new_rule(next_nonterminal_value);
        add_rule(m_rule);

        SYMBOL *ptr = main_rule;
        for(i=0; i < bsize ; i++){
            SYMBOL *s = new_symbol(c, NULL);
            insert_after(ptr, s);
            check_digram(ptr);
            ptr = s;
            if(i != bsize-1){
                c = fgetc(in);
            }
            if(c == EOF){
                break;
            }
        }
        expand_blocks(main_rule, out);
    }
    fputc(EOT, out);
    return EOF;
}

void expand_blocks(SYMBOL *rule_head, FILE *out){
    if(rule_head != NULL){
        fputc(SOB, out);
        SYMBOL *ptr = rule_head;
        do{
            SYMBOL *ptr2 = ptr;
            do{
                //print the symbol at ptr2
                print_UTF_value(ptr2->value, out);
                ptr2 = ptr2 ->next;
            }while(ptr2 != ptr);

            fputc(RD, out);
            ptr = ptr->nextr;
        }while(ptr != rule_head);
        fputc(EOB, out);
    }
}

void print_UTF_value(int v, FILE *out){
    if(v >= 0 && v <= 0x7f){
        //1 Byte
        fputc(v, out);
    }
    else if(v > 0x7f && v <= 0x7ff){
        //2 Byte
        int byte2;
        int byte1;

        byte2 = v & 0x3f;
        byte2 =  byte2 | 0x80;

        v = v >> 6;

        byte1 = v | 0xc0;

        fputc(byte1, out);
        fputc(byte2, out);
    }
    else if(v >0x7ff && v <= 0xffff){
        //3 Byte
        int byte3;
        int byte2;
        int byte1;

        byte3 = v & 0x3f;
        byte3 = byte3 | 0x80;

        v = v >> 6;

        byte2 = v & 0x3f;
        byte2 = byte2 | 0x80;

        v = v >> 6;

        byte1 = v | 0xe0;

        fputc(byte1, out);
        fputc(byte2, out);
        fputc(byte3, out);

    }
    else if(v > 0xffff && v <= 0x1ffff){
        //4 Byte
        int byte4;
        int byte3;
        int byte2;
        int byte1;

        byte4 = v & 0x3f;
        byte4 = byte4 | 0x80;

        v = v >> 6;

        byte3 = v & 0x3f;
        byte3 = byte3 | 0x80;

        v = v >> 6;

        byte2 = v & 0x3f;
        byte2 = byte2 | 0x80;

        v = v >> 6;

        byte1 = v | 0xf0;

        fputc(byte1, out);
        fputc(byte2, out);
        fputc(byte3, out);
        fputc(byte4, out);
    }
}
/**
 * Main decompression function.
 * Reads a compressed data transmission from an input stream, expands it,
 * and and writes the resulting decompressed data to an output stream.
 * The output stream is flushed once writing is complete.
 *
 * @param in  The stream from which the compressed block is to be read.
 * @param out  The stream to which the uncompressed data is to be written.
 * @return  The number of bytes written, in case of success, otherwise EOF.
 */

int decompress(FILE *in, FILE *out) {
    // To be implemented.
    int c;
    int *pc =&c;
    while((unsigned char)(c = fgetc(in))!=(unsigned char)EOT && (unsigned char)c!=(unsigned char)EOF){
        if((unsigned char)c == (unsigned char)SOB){            c=fgetc(in);
            while((unsigned char)c !=(unsigned char)EOB){
                if(validUTF(c) && notMarker(c)){
                    SYMBOL *s = createRule(c,in, pc);
                    *(rule_map+(s->value)) = s;
                    if((unsigned char)c ==(unsigned char)EOB){
                        break;
                    }
                }
                c=fgetc(in);
            }
            expand_rules(main_rule, out);
            init_symbols();
            init_rules();
        }

    }
    fflush(out);
    return EOF;
}


SYMBOL *createRule(int ci, FILE *in, int *pc){
    int count = getByteCount(ci);
    int v= getValue(ci,count, in);
    SYMBOL *new = new_rule(v);
    SYMBOL *ptr = new;
    add_rule(new);
    while((unsigned char)(ci=fgetc(in)) != (unsigned char)RD && (unsigned char)ci != (unsigned char) EOB){
        if(validUTF(ci)){
            count = getByteCount(ci);
            v = getValue(ci, count,in);
            SYMBOL *bnew = new_symbol(v,NULL);
            ptr->next = bnew;
            (*bnew).prev = ptr;
            (*bnew).next = new;
            (*new).prev =bnew;
            ptr= bnew;
        }
    }
    if((unsigned char)ci == (unsigned char) EOB){
        *pc = ci;
    }
    return new;
}

int getByteCount(int i){
    if((i & 0xf8) == 0xf0){
        return 4;
    }
    else if((i & 0xf0) == 0xe0){
        return 3;
    }
    else if((i & 0xe0) == 0xc0){
        return 2;
    }
    else if((i & 0x80) == 0x00){
        return 1;
    }
    else return 0;
}
int getValue(int i, int byteCount, FILE *in){
    int j;
    if(byteCount == 4){
        i = i & 0x07;
    }
    else if(byteCount == 3){
        i = i & 0x0f;
    }
    else if(byteCount == 2){
        i = i & 0x1f;
    }
    else if(byteCount == 1){
        i = i & 0x7f;
        return i;
    }
    int c;
    for(j=0;j<byteCount-1;j++){
        c = fgetc(in);
        c = c & 0x3f;
        i = i<<6;
        i = i | c;
    }
    return i;
}


void expand_rules(SYMBOL *ruleHead, FILE *out){
    SYMBOL *ptr = ruleHead->next;
    while(ptr != ruleHead){
        if(IS_TERMINAL(ptr)){
            fputc((ptr->value),out);
        }
        else if(IS_NONTERMINAL(ptr)){
            expand_rules(*(rule_map+(ptr->value)),out);
        }
        ptr = ptr->next;
    }
}

int validUTF(int c){
    if(getByteCount(c)){
        return 1;
    }
    else return 0;
}

int notMarker(int c){
    if(((unsigned char)c == SOB) || ((unsigned char)c == SOT) ||((unsigned char)c == EOT) || ((unsigned char)c == RD)){
        return 0;
    }
    else return 1;
}
/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv)
{
    // To be implemented.
    char **targv = argv;
    if(argc == 1){
        return -1;
    }
    else if(argc >1){
        targv++;
        argc--;
        if(**targv == '-'){
            char **tp = targv;
            (*tp)++;

            if(**tp == 'h'){
                global_options =1;
                return 0;
            }
            else if(**tp == 'c'){
                if(argc > 1){
                    targv++;
                    argc--;
                    if(argc != 2){
                        return -1;
                    }
                    else{
                        tp = targv;
                        if(**targv == '-'){
                            (*tp)++;
                            if(**tp == 'b'){
                                targv++;
                                tp = targv;
                                int num =0;
                                while(**tp != '\0'){
                                    if(isDigit(**tp)){
                                        num = (num*10) + ((**tp)-'0');
                                    }
                                    else{
                                       return -1;
                                    }
                                    (*tp)++;
                                }
                                if(num<1 || num >1024){
                                    return -1;
                                }
                                else{
                                    num = num <<16;
                                    global_options = num | 2;
                                    return 0;
                                }

                            }
                            else{
                                return -1;
                            }
                        }
                        else{
                            return -1;
                        }
                    }
                }
                else{
                    global_options = 1024;
                    global_options = global_options <<16;
                    global_options = global_options | 2;
                    return 0;
                }
            }
            else if(**tp == 'd'){
                if(argc != 1){
                    return -1;
                }
                else{
                    global_options = 4;
                    return 0;
                }
            }
            else{
                return -1;
            }
        }
        else{
            return -1;
        }
    }

    return -1;
}

int isDigit(char c){
    if(c >= '0' && c <= '9'){
        return 1;
    }
    else return 0;
}