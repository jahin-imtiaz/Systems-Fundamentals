#include "const.h"
#include "sequitur.h"

SYMBOL *recycled_symbols;
/*
 * Symbol management.
 *
 * The functions here manage a statically allocated array of SYMBOL structures,
 * together with a stack of "recycled" symbols.
 */

/*
 * Initialization of this global variable that could not be performed in the header file.
 */
int next_nonterminal_value = FIRST_NONTERMINAL;

/**
 * Initialize the symbols module.
 * Frees all symbols, setting num_symbols to 0, and resets next_nonterminal_value
 * to FIRST_NONTERMINAL;
 */
void init_symbols(void) {
    num_symbols = 0;
    next_nonterminal_value = FIRST_NONTERMINAL;
    // To be implemented.
}

/**
 * Get a new symbol.
 *
 * @param value  The value to be used for the symbol.  Whether the symbol is a terminal
 * symbol or a non-terminal symbol is determined by its value: terminal symbols have
 * "small" values (i.e. < FIRST_NONTERMINAL), and nonterminal symbols have "large" values
 * (i.e. >= FIRST_NONTERMINAL).
 * @param rule  For a terminal symbol, this parameter should be NULL.  For a nonterminal
 * symbol, this parameter can be used to specify a rule having that nonterminal at its head.
 * In that case, the reference count of the rule is increased by one and a pointer to the rule
 * is stored in the symbol.  This parameter can also be NULL for a nonterminal symbol if the
 * associated rule is not currently known and will be assigned later.
 * @return  A pointer to the new symbol, whose value and rule fields have been initialized
 * according to the parameters passed, and with other fields zeroed.  If the symbol storage
 * is exhausted and a new symbol cannot be created, then a message is printed to stderr and
 * abort() is called.
 *
 * When this function is called, if there are any recycled symbols, then one of those is removed
 * from the recycling list and used to satisfy the request.
 * Otherwise, if there currently are no recycled symbols, then a new symbol is allocated from
 * the main symbol_storage array and the num_symbols variable is incremented to record the
 * allocation.
 */
SYMBOL *new_symbol(int value, SYMBOL *rule) {
    // To be implemented.
    if(num_symbols == MAX_SYMBOLS && recycled_symbols == NULL){
        fprintf(stderr, "New symbol was not created due to exhausted storage\n");
        abort();
    }
    else if(recycled_symbols != NULL){
        recycled_symbols->value = value;
        recycled_symbols->refcnt= 0;
        recycled_symbols->next =NULL;
        recycled_symbols->prev =NULL;
        recycled_symbols->nextr=NULL;
        recycled_symbols->prevr=NULL;
        if(IS_TERMINAL(recycled_symbols)){
            if(rule == NULL){
                recycled_symbols->rule = rule;
            }
            else{
                return NULL;
            }
        }
        else if(IS_NONTERMINAL(recycled_symbols)){
            recycled_symbols->rule = rule;
            if(rule != NULL){
                rule->refcnt = (rule->refcnt)+1;
            }

        }
        SYMBOL *ptr = recycled_symbols;
        recycled_symbols = recycled_symbols->next;
        return ptr;
    }
    else{
        SYMBOL s;
        s.value = value;
        s.refcnt = 0;
        s.next = NULL;
        s.prev = NULL;
        s.nextr = NULL;
        s.prevr = NULL;
        if(IS_TERMINAL(&s)){
            if(rule == NULL){
                s.rule = rule;
            }
            else{
                return NULL;
            }
        }
        else if(IS_NONTERMINAL(&s)){
            s.rule = rule;
            if(rule != NULL){
                rule->refcnt = (rule->refcnt)+1;
            }
        }
        *(symbol_storage + num_symbols) = s;
        num_symbols+=1;
        return (symbol_storage + (num_symbols-1));
    }
    return NULL;
}

/**
 * Recycle a symbol that is no longer being used.
 *
 * @param s  The symbol to be recycled.  The caller must not use this symbol any more
 * once it has been recycled.
 *
 * Symbols being recycled are added to the recycled_symbols list, where they will
 * be made available for re-allocation by a subsequent call to new_symbol.
 * The recycled_symbols list is managed as a LIFO list (i.e. a stack), using the
 * next field of the SYMBOL structure to chain together the entries.
 */
void recycle_symbol(SYMBOL *s) {
    // To be implemented.
    if(recycled_symbols == NULL){
        recycled_symbols = s;
    }
    else{
        s->next = recycled_symbols;
        recycled_symbols= s;
    }

}
