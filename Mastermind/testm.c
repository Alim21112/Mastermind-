#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define LENGTH 3
#define COLORS 3

#define NAN1 8
#define NAN2 9

const int seqlen = LENGTH;
const int seqmax = COLORS;

/* ********************************** */
/* take these fcts from master-mind.c */
/* ********************************** */

/* Show given sequence */
void showSeq(int *seq) {
    printf("Seq: %d %d %d\n", seq[0], seq[1], seq[2]);
}

/* Parse an integer value as a list of digits of base MAX */
void readSeq(int *seq, int val) {
    for (int i = seqlen-1; i >= 0; i--) {
        seq[i] = val % 10;
        val /= 10;
    }
}

/* counts how many entries in seq2 match entries in seq1 */
/* returns exact and approximate matches, either both encoded in one value, */
/* or as a pointer to a pair of values */
int countMatches(int *seq1, int *seq2) {
    int exact = 0, approx = 0;
    int used1[LENGTH] = {0};
    int used2[LENGTH] = {0};

    // Count exact matches first
    for (int i = 0; i < LENGTH; i++) {
        if (seq1[i] == seq2[i]) {
            exact++;
            used1[i] = 1;
            used2[i] = 1;
        }
    }

    // Count approximate matches
    for (int i = 0; i < LENGTH; i++) {
        if (used2[i]) continue;
        for (int j = 0; j < LENGTH; j++) {
            if (!used1[j] && seq2[i] == seq1[j]) {
                approx++;
                used1[j] = 1;
                break;
            }
        }
    }

    return exact * 10 + approx;
}

/* show the results from calling countMatches on seq1 and seq1 */
void showMatches(int code, int *seq1, int *seq2, int lcd_format) {
    int exact = code / 10;
    int approx = code % 10;
    printf("Exact matches: %d, Approximate matches: %d\n", exact, approx);
}

/* read a guess sequence fron stdin and store the values in arr */
/* only needed for testing the game logic, without button input */
int readNum(int max) {
    int num;
    scanf("%d", &num);
    return num;
}

// The ARM assembler version of the matching fct
extern int matches(int *val1, int *val2);

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main(int argc, char **argv) {
    int res, res_c, t, t_c, m, n;
    int *seq1, *seq2, *cpy1, *cpy2;
    struct timeval t1, t2;
    char str_in[20], str[20] = "some text";
    int verbose = 0, debug = 0, help = 0, opt_s = 0, opt_n = 0;
  
    // see: man 3 getopt for docu and an example of command line parsing
    { // see the CW spec for the intended meaning of these options
        int opt;
        while ((opt = getopt(argc, argv, "hvd:s:n:")) != -1) {
            switch (opt) {
            case 'v':
                verbose = 1;
                break;
            case 'h':
                help = 1;
                break;
            case 'd':
                debug = 1;
                break;
            case 's':
                opt_s = atoi(optarg); 
                break;
            case 'n':
                opt_n = atoi(optarg); 
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-s <seed>] [-n <no. of iterations>] [seq1 seq2]\n", argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }

    if (help) {
        fprintf(stderr, "Test program for MasterMind matching function\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -h      Show this help message\n");
        fprintf(stderr, "  -v      Verbose output\n");
        fprintf(stderr, "  -d      Debug output\n");
        fprintf(stderr, "  -s NUM  Set random seed\n");
        fprintf(stderr, "  -n NUM  Number of test iterations\n");
        exit(EXIT_SUCCESS);
    }

    seq1 = (int*)malloc(seqlen*sizeof(int));
    seq2 = (int*)malloc(seqlen*sizeof(int));
    cpy1 = (int*)malloc(seqlen*sizeof(int));
    cpy2 = (int*)malloc(seqlen*sizeof(int));
  
    if (argc > optind+1) {
        // Test with specific sequences provided as arguments
        strcpy(str_in, argv[optind]);
        m = atoi(str_in);
        strcpy(str_in, argv[optind+1]);
        n = atoi(str_in);
        fprintf(stderr, "Testing matches function with sequences %d and %d\n", m, n);
        readSeq(seq1, m);
        readSeq(seq2, n);

        // Make copies of original sequences for comparison
        memcpy(cpy1, seq1, seqlen*sizeof(int));
        memcpy(cpy2, seq2, seqlen*sizeof(int));
        
        // Time C version
        gettimeofday(&t1, NULL);
        res_c = countMatches(seq1, seq2);
        gettimeofday(&t2, NULL);
        if (t2.tv_usec < t1.tv_usec)
            t_c = (1000000 + t2.tv_usec) - t1.tv_usec;
        else
            t_c = t2.tv_usec - t1.tv_usec;

        if (debug) {
            fprintf(stdout, "DBG: sequences after matching:\n");    
            showSeq(seq1);
            showSeq(seq2);
        }
        
        // Restore sequences
        memcpy(seq1, cpy1, seqlen*sizeof(int));
        memcpy(seq2, cpy2, seqlen*sizeof(int));
        
        // Time Assembler version
        gettimeofday(&t1, NULL);
        res = matches(seq1, seq2);
        gettimeofday(&t2, NULL);
        if (t2.tv_usec < t1.tv_usec)
            t = (1000000 + t2.tv_usec) - t1.tv_usec;
        else
            t = t2.tv_usec - t1.tv_usec;

        if (debug) {
            fprintf(stdout, "DBG: sequences after matching:\n");    
            showSeq(seq1);
            showSeq(seq2);
        }

        // Restore sequences again
        memcpy(seq1, cpy1, seqlen*sizeof(int));
        memcpy(seq2, cpy2, seqlen*sizeof(int));
        
        // Show results
        showMatches(res_c, seq1, seq2, 0);
        showMatches(res, seq1, seq2, 0);

        // Compare results
        if (res == res_c) {
            fprintf(stdout, "__ result OK\n");
        } else {
            fprintf(stdout, "** result WRONG\n");
        }
        fprintf(stderr, "C   version:\t\tresult=%d (elapsed time: %dµs)\n", res_c, t_c);
        fprintf(stderr, "Asm version:\t\tresult=%d (elapsed time: %dµs)\n", res, t);
    } else {
        // Run automated tests with random sequences
        int i, j, n = 10, res, res_c, oks = 0, tot = 0;
        fprintf(stderr, "Running tests of matches function with %d pairs of random input sequences ...\n", n);
        if (opt_n != 0)
            n = opt_n;
        
        // Используем текущее время как seed, если не указан другой
        if (opt_s != 0) {
            srand(opt_s);
        } else {
            srand(time(NULL));  // Используем текущее время для seed
        }
        
        for (i = 0; i < n; i++) {
            // Generate random sequences
            for (j = 0; j < seqlen; j++) {
                seq1[j] = (rand() % seqmax) + 1;
                seq2[j] = (rand() % seqmax) + 1;
            }
            
            // Make copies of original sequences
            memcpy(cpy1, seq1, seqlen*sizeof(int));
            memcpy(cpy2, seq2, seqlen*sizeof(int));
            
            if (verbose) {
                fprintf(stderr, "Random sequences are:\n");
                showSeq(seq1);
                showSeq(seq2);
            }
            
            // Test Assembler version
            res = matches(seq1, seq2);
            // Restore sequences
            memcpy(seq1, cpy1, seqlen*sizeof(int));
            memcpy(seq2, cpy2, seqlen*sizeof(int));
            // Test C version
            res_c = countMatches(seq1, seq2);
            
            if (debug) {
                fprintf(stdout, "DBG: sequences after matching:\n");    
                showSeq(seq1);
                showSeq(seq2);
            }
            
            fprintf(stdout, "Matches (encoded) (in C):   %d\n", res_c);
            fprintf(stdout, "Matches (encoded) (in Asm): %d\n", res);
            
            // Restore sequences again
            memcpy(seq1, cpy1, seqlen*sizeof(int));
            memcpy(seq2, cpy2, seqlen*sizeof(int));
            // Show results
            showMatches(res_c, seq1, seq2, 0);
            showMatches(res, seq1, seq2, 0);
            
            tot++;
            if (res == res_c) {
                fprintf(stdout, "__ result OK\n");
                oks++;
            } else {
                fprintf(stdout, "** result WRONG\n");
            }
        }
        fprintf(stderr, "%d out of %d tests OK\n", oks, tot);
        exit(oks == tot ? 0 : 1);
    }

    // Clean up
    free(seq1);
    free(seq2);
    free(cpy1);
    free(cpy2);

    return 0;
}
