#include "rulebasedmodels.h"
#include "rsample.h"
#include "strbuf.h"
#include "redefine.h"

#include <string.h>
#include <malloc.h>

extern void c50main();
extern void sample(double *outputv);
extern void FreeCases(void);

void c50(char **namesv,
         char **datav,
         char **costv,
         int *subset,
         int *rules,
         int *utility,
         int *trials,
         int *winnow,
         double *sample,
         int *seed,
         int *noGlobalPruning,
         double *CF,
         int *minCases,
         int *fuzzyThreshold,
         int *earlyStopping,
         char **treev,
         char **rulesv,
         char **outputv)
{
    int val;  /* Used by setjmp/longjmp for implementing rbm_exit */

    // Announce ourselves for testing
    // printf("c50 called\n");

    // Initialize the globals to the values that the c50
    // program would have at the start of execution
    initglobals();

    // Set globals based on the arguments.  This is analogous
    // to parsing the command line in the c50 program.
    setglobals(*subset, *rules, *utility, *trials, *winnow, *sample,
               *seed, *noGlobalPruning, *CF, *minCases, *fuzzyThreshold,
               *earlyStopping, *costv);

    // Handles the strbufv data structure
    rbm_removeall();

    // Deallocates memory allocated by NewCase.
    // Not necessary since it's also called at the end of this function,
    // but it doesn't hurt, and I'm feeling paranoid.
    FreeCases();

    // XXX Should this be controlled via an option?
    // printf("Calling setOf\n");
    setOf();

    // Create a strbuf using *namesv as the buffer.
    // Note that this is a readonly strbuf since we can't
    // extend *namesv.
    STRBUF *sb_names = strbuf_create_full(*namesv, strlen(*namesv));

    // Register this strbuf using the name "undefined.names"
	if (rbm_register(sb_names, "undefined.names", 0) < 0) {
    fprintf(stderr, "undefined.names already exists");
	}

    // Create a strbuf using *datav and register it as "undefined.data"
    STRBUF *sb_datav = strbuf_create_full(*datav, strlen(*datav));
    // XXX why is sb_datav copied? was that part of my debugging?
    // XXX or is this the cause of the leak?
	if (rbm_register(strbuf_copy(sb_datav), "undefined.data", 0) < 0) {
		fprintf(stderr, "undefined data already exists");
	}

    // Create a strbuf using *costv and register it as "undefined.costs"
    if (strlen(*costv) > 0) {
        // printf("registering cost matrix: %s", *costv);
        STRBUF *sb_costv = strbuf_create_full(*costv, strlen(*costv));
        // XXX should sb_costv be copied?
	    if (rbm_register(sb_costv, "undefined.costs", 0) < 0) {
		    fprintf(stderr, "undefined.cost already exists");
	    }
    } else {
        // printf("no cost matrix to register\n");
    }

    /*
     * We need to initialize rbm_buf before calling any code that
     * might call exit/rbm_exit.
     */
    if ((val = setjmp(rbm_buf)) == 0) {

        // Real work is done here
        // printf("Calling c50main\n");
        c50main();

        // printf("c50main finished\n");

        if (*rules == 0) {
            // Get the contents of the the tree file
            STRBUF *treebuf = rbm_lookup("undefined.tree");
            if (treebuf != NULL) {
                char *treeString = strbuf_getall(treebuf);
                char *treeObj = calloc(strlen(treeString) + 1, 1);
                strcpy(treeObj, treeString);

                // I think the previous value of *treev will be garbage collected
                *treev = treeObj;
            } else {
                // XXX Should *treev be assigned something in this case?
                // XXX Throw an error?
            }
        } else {
            // Get the contents of the the rules file
            STRBUF *rulesbuf = rbm_lookup("undefined.rules");
            if (rulesbuf != NULL) {
                char *rulesString = strbuf_getall(rulesbuf);
                char *rulesObj = calloc(strlen(rulesString) + 1, 1);
                strcpy(rulesObj, rulesString);

                // I think the previous value of *rulesv will be garbage collected
                *rulesv = rulesObj;
            } else {
                // XXX Should *rulesv be assigned something in this case?
                // XXX Throw an error?
            }
        }
    } else {
        printf("c50 code called exit with value %d\n", val - JMP_OFFSET);
    }

    // Close file object "Of", and return its contents via argument outputv
    char *outputString = closeOf();
    char *output = calloc(strlen(outputString) + 1, 1);
    strcpy(output, outputString);
    *outputv = output;

    // Deallocates memory allocated by NewCase
    FreeCases();

    // We reinitialize the globals on exit out of general paranoia
    initglobals();
}

void predictions(char **casev,
                 char **namesv,
                 char **treev,
                 char **rulesv,
                 char **costv,
                 int *predv,  /* XXX predictions are character */
			double *confidencev,
			int *trials,
                        char **outputv)
{
    int val;  /* Used by setjmp/longjmp for implementing rbm_exit */

    // Announce ourselves for testing
    // printf("predictions called\n");

    // Initialize the globals
    initglobals();

    // Handles the strbufv data structure
    rbm_removeall();

    // XXX Should this be controlled via an option?
    // printf("Calling setOf\n");
    setOf();

    STRBUF *sb_cases = strbuf_create_full(*casev, strlen(*casev));
	if (rbm_register(sb_cases, "undefined.cases", 0) < 0) {
		fprintf(stderr, "undefined.cases already exists");
	}

    STRBUF *sb_names = strbuf_create_full(*namesv, strlen(*namesv));
	if (rbm_register(sb_names, "undefined.names", 0) < 0) {
		fprintf(stderr, "undefined.names already exists");
	}

    if (strlen(*treev)) {
	STRBUF *sb_treev = strbuf_create_full(*treev, strlen(*treev));
	/* XXX should sb_treev be copied? */
	if (rbm_register(sb_treev, "undefined.tree", 0) < 0) {
	    fprintf(stderr, "undefined.tree already exists");
	}
    } else if (strlen(*rulesv))  {
	STRBUF *sb_rulesv = strbuf_create_full(*rulesv, strlen(*rulesv));
	/* XXX should sb_rulesv be copied? */
	if (rbm_register(sb_rulesv, "undefined.rules", 0) < 0) {
	    fprintf(stderr, "undefined.rules already exists");
	}
	setrules(1);
    } else {
	fprintf(stderr, "either a tree or rules must be provided");
    }

    // Create a strbuf using *costv and register it as "undefined.costs"
    if (strlen(*costv) > 0) {
        // printf("registering cost matrix: %s", *costv);
        STRBUF *sb_costv = strbuf_create_full(*costv, strlen(*costv));
        // XXX should sb_costv be copied?
	    if (rbm_register(sb_costv, "undefined.costs", 0) < 0) {
		    fprintf(stderr, "undefined.cost already exists");
	    }
    } else {
        // printf("no cost matrix to register\n");
    }

    /*
     * We need to initialize rbm_buf before calling any code that
     * might call exit/rbm_exit.
     */
    if ((val = setjmp(rbm_buf)) == 0) {
        // Real work is done here
        // printf("\n\nCalling rpredictmain\n");
        rpredictmain(trials ,predv ,confidencev);

        // printf("predict finished\n\n");
    } else {
        printf("predict code called exit with value %d\n\n", val - JMP_OFFSET);
    }

    // Close file object "Of", and return its contents via argument outputv
    char *outputString = closeOf();
    char *output = calloc(strlen(outputString) + 1, 1);
    strcpy(output, outputString);
    *outputv = output;

    // We reinitialize the globals on exit out of general paranoia
    initglobals();
}

