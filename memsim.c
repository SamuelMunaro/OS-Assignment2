#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ----------------------------------------------------------------
 * Virtual-Memory Page Replacement Simulator
 *
 * Implements three policies:
 *   - rand  : replace a random resident page
 *   - fifo  : replace the oldest resident page (first-in-first-out)
 *   - lru   : replace the least-recently used resident page
 *   - clock : single-hand CLOCK (2nd-chance) using a reference bit
 *
 * Page size is fixed to 4KB (offset = 12bits)
 *
 * Modes:
 *   - quiet : prints only a summary at the end
 *   - debug : print per-access actions as they happen
 *
 * Input trace format (after gunzip):
 *   hex_address R|W
 *   0041f7a0 R
 *   31348900 W
** -------------------------------------------------------------- */

#define PAGE_OFFSET 12 /* 4KB */
typedef unsigned long long ull;

typedef struct {
  int pageNo;
  int modified;
} page;

enum repl { RANDOM, FIFO, LRU, CLOCK };

int createMMU(int);
int checkInMemory(int);
int allocateFrame(int);
page selectVictim(int, enum repl);

int numFrames;

/* ----------------- Internal MMU State ------------------- */
/* One slot per frame */
static int *frame_page = NULL;          /* -1 if free, else resident page number */
static char *frame_dirty = NULL;        /* 0/1 dirty bit */
static char *frame_ref = NULL;          /* reference bit (for CLOCK) */
static ull *frame_last = NULL;          /* last-used timestamp (for LRU) */

static int next_free_idx = 0;           /* where to place next free frame */
static int clock_hand = 0;              /* pointer for CLOCK */
static int fifo_hand = 0;               /* next victim for FIFO */
static ull tnow = 0ULL;                 /* global timestamp for LRU updates */

static int debugmode = 0;               /* read from argv[4] in main() */

/* Creates the page table structure to record memory allocation */
int createMMU(int frames) {
    // to do
    return 0;
}

/* Checks for residency: returns frame no or -1 if not found */
int checkInMemory(int page_number) {
    for (int i = 0; i < numFrames; i++) {
        if (frame_page[i] == page_number) {
            frame_last[i] == ++tnow;
            frame_ref[i] == 1;
            return i;
        }
    }
    return -1;      
}

/* allocate page to the next free frame and record where it put it */
int allocateFrame(int page_number) {
    // to do
    return -1;
}

/* Selects a victim for eviction/discard according to the replacement algorithm,
 * returns chosen frame_no  */
page selectVictim(int page_number, enum repl mode) {
    page victim;
    // to do
    victim.pageNo = 0;
    victim.modified = 0;
    return (victim);
}

int main(int argc, char *argv[]) {
    char *tracename;
    int page_number, frame_no, done;
    int do_line, i;
    int no_events, disk_writes, disk_reads;
    int debugmode;
    enum repl replace;
    int allocated = 0;
    int victim_page;
    unsigned address;
    char rw;
    page Pvictim;
    FILE *trace;

    if (argc < 5) {
        printf(
            "Usage: ./memsim inputfile numberframes replacementmode debugmode \n");
        exit(-1);
    } else {
        tracename = argv[1];
        trace = fopen(tracename, "r");

        if (trace == NULL) {
            printf("Cannot open trace file %s \n", tracename);
            exit(-1);
        }

        numFrames = atoi(argv[2]);
        
        if (numFrames < 1) {
            printf("Frame number must be at least 1\n");
            exit(-1);
        }

        if (strcmp(argv[3], "lru\0") == 0) {
            replace = LRU;
        } else if (strcmp(argv[3], "rand\0") == 0) {
            replace = RANDOM;
        } else if (strcmp(argv[3], "clock\0") == 0) {
            replace = CLOCK;
        } else if (strcmp(argv[3], "fifo\0") == 0) {
            replace = FIFO;
        } else {
            printf("Replacement algorithm must be rand/fifo/lru/clock  \n");
            exit(-1);
        }

        if (strcmp(argv[4], "quiet\0") == 0) {
            debugmode = 0;
        } else if (strcmp(argv[4], "debug\0") == 0) {
            debugmode = 1;
        } else {
            printf("Replacement algorithm must be quiet/debug  \n");
            exit(-1);
        }
    }

    done = createMMU(numFrames);

    if (done == -1) {
        printf("Cannot create MMU");
        exit(-1);
    }

    no_events = 0;
    disk_writes = 0;
    disk_reads = 0;

    do_line = fscanf(trace, "%x %c", &address, &rw);

    while (do_line == 2) {
        page_number = address >> PAGE_OFFSET;
        frame_no = checkInMemory(page_number); /* ask for physical address */

        if (frame_no == -1) {
            disk_reads++; /* Page fault, need to load it into memory */ 
            if (debugmode) {
                printf("Page fault %8d \n", page_number);
            }
            if (allocated < numFrames) {/* allocate it to an empty frame */
                frame_no = allocateFrame(page_number);
                allocated++;
            } else {
                Pvictim = selectVictim(page_number,replace); /* returns page number of the victim */
                frame_no = checkInMemory(page_number);     /* find out the frame the new page is in */
                if (Pvictim.modified) {/* need to know victim page and modified  */
                    disk_writes++;
                }
                if (debugmode) {
                    printf("Disk write %8d \n", Pvictim.pageNo);
                } else if (debugmode) {
                    printf("Discard    %8d \n", Pvictim.pageNo);
                }
            }
        }
    }
    if (rw == 'R') {
        if (debugmode) { 
            printf("reading    %8d \n", page_number);
        }
    } else if (rw == 'W') {
        // mark page in page table as written - modified
        if (debugmode) {
            printf("writting   %8d \n", page_number);
        }
    } else {
        printf("Badly formatted file. Error on line %d\n", no_events + 1);
        exit(-1);
    }

    no_events++;
    do_line = fscanf(trace, "%x %c", &address, &rw);

    printf("total memory frames:  %d\n", numFrames);
    printf("events in trace:      %d\n", no_events);
    printf("total disk reads:     %d\n", disk_reads);
    printf("total disk writes:    %d\n", disk_writes);
    printf("page fault rate:      %.4f\n", (float)disk_reads / no_events);

    /* cleanup */
    free(frame_page);
    free(frame_dirty);
    free(frame_ref);
    free(frame_last);
    fclose(trace);
    return 0;
}
