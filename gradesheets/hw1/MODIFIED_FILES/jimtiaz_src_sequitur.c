--- hw1/dev_repo/solution/src/sequitur.c

+++ hw1/repos/jimtiaz/hw1/src/sequitur.c

@@ -34,7 +34,7 @@

     if(this->next) {

 	// We will be assigning to this->next, which will destroy any digram

 	// that starts at this.  So that is what we have to delete from the table.

-	digram_delete(this);

+    digram_delete(this);

 

 	// We have to have special-case treatment of triples, which are occurrences

 	// of the same three symbols in a row.  These violations of "no repeated digrams"

@@ -51,7 +51,7 @@

 	//    abbbc  ==> abbc   (handle this first)

 	//      ^

 	//    abbbc  ==> abbc   (then check for this one)

-        //     ^

+    //     ^

 	if(next->prev && next->next &&

 	   next->value == next->prev->value && next->value == next->next->value)

 	    digram_put(next);

@@ -189,7 +189,6 @@

     debug("Process matching digrams <%lu> and <%lu>",

 	  SYMBOL_INDEX(this), SYMBOL_INDEX(match));

     SYMBOL *rule = NULL;

-

     if(IS_RULE_HEAD(match->prev) && IS_RULE_HEAD(match->next->next)) {

 	// If the digram headed by match constitutes the entire right-hand side

 	// of a rule, then we don't create any new rule.  Instead we use the

@@ -276,17 +275,15 @@

  */

 int check_digram(SYMBOL *this) {

     debug("Check digram <%lu> for a match", SYMBOL_INDEX(this));

-

     // If the "digram" is actually a single symbol at the beginning or

     // end of a rule, then there is no need to do anything.

     if(IS_RULE_HEAD(this) || IS_RULE_HEAD(this->next))

 	return 0;

-

     // Otherwise, look up the digram in the digram table, to see if there is

     // a matching instance.

     SYMBOL *match = digram_get(this->value, this->next->value);

     if(match == NULL) {

-        // The digram did not previously exist -- insert it now.

+    // The digram did not previously exist -- insert it now.

 	digram_put(this);

 	return 0;

     }
