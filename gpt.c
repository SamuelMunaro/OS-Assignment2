#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

/* ----------------------------------------------------------------
 * Virtual-Memory Page Replacement Simulator

 * Implements four policies:
    - rand  : replace a random resident page
    - fifo  : replace the oldest resident page (first-in-first-out)
    - lru   : replace the least-recently used resident page
    - clock : single-hand CLOCK (2nd-chance) using a reference bit

 * Page size is fixed to 4KB (offset = 12bits)

 * Modes:
    - quiet : prints only a summary at the end
    - debug : print per-access actions as they happen

 * Input trace format (after gunzip):
    hex_address R|W
    0041f7a0 R
    31348900 W 
 * ------------------------------------------------------------- */

#define PAGE_OFFSET 12  /* 4KB */

typedef struct {
        int pageNo;
        int modified;
} page;

enum repl { random, fifo, lru, clock };

int  createMMU(int);
int  checkInMemory(int) ;
int  allocateFrame(int) ;
page selectVictim(int, enum repl) ;

int  numFrames;

/* ----------------- Internal MMU State ------------------- */
/* One slot per frame */
static int    *frame_page      = NULL;    /* -1 if free, else resident page number */
static char   *frame_dirty     = NULL;    /* 0/1 dirty bit */
static char   *frame_ref       = NULL;    /* reference bit (for CLOCK) */
static unsigned long long *frame_last    = NULL; /* last-used timestamp (for LRU) */

static int     next_free_idx   = 0;       /* where to place next free frame */
static int     fifo_hand       = 0;       /* next victim for FIFO */
static int     clock_hand      = 0;       /* pointer for CLOCK */
static unsigned long long tnow = 0ULL;    /* global timestamp for LRU updates */

static int     debugmode       = 0;       /* read from argv[4] in main() */

/* Creates the page table structure to record memory allocation */
int createMMU (int frames)
{
        frame_page   = (int*)malloc(sizeof(int)*frames);
        frame_dirty  = (char*)malloc(sizeof(char)*frames);
        frame_ref    = (char*)malloc(sizeof(char)*frames);
        frame_last   = (unsigned long long*)malloc(sizeof(unsigned long long)*frames);

        if (!frame_page || !frame_dirty || !frame_ref || !frame_last)
                return -1;

        for (int i = 0; i < frames; i++) {
                frame_page[i] = -1;   /* free */
                frame_dirty[i] = 0;
                frame_ref[i]   = 0;
                frame_last[i]  = 0ULL;
        }

        next_free_idx = 0;
        fifo_hand = 0;
        clock_hand = 0;
        tnow = 0ULL;

        /* deterministic RNG so runs are reproducible */
        srand(0xC0FFEE);

        return 0;
}

/* Checks for residency: returns frame no or -1 if not found
   Also updates LRU timestamp and CLOCK ref bit on access. */
int checkInMemory(int page_number)
{
        for (int i = 0; i < numFrames; i++) {
                if (frame_page[i] == page_number) {
                        /* hit: update metadata */
                        frame_last[i] = ++tnow;
                        frame_ref[i]  = 1;
                        return i;
                }
        }
        return -1 ;
}

/* allocate page to the next free frame and record where it put it */
int allocateFrame(int page_number)
{
        /* find a free frame (marked by page == -1).
           next_free_idx is just a hint for speed; fall back to scan. */
        int i = -1;
        for (int k = 0; k < numFrames; k++) {
                int idx = (next_free_idx + k) % numFrames;
                if (frame_page[idx] == -1) { i = idx; break; }
        }
        if (i == -1) {
                /* caller should only call when a free frame exists */
                return -1;
        }

        frame_page[i]  = page_number;
        frame_dirty[i] = 0;       /* freshly loaded page is clean */
        frame_ref[i]   = 1;       /* referenced upon load */
        frame_last[i]  = ++tnow;  /* now the most recently used */

        next_free_idx = (i + 1) % numFrames;
        return i;
}

/* helpers for victim selection */
static int choose_random_victim(void) {
        return rand() % numFrames;
}

static int choose_lru_victim(void) {
        unsigned long long best = ULLONG_MAX;
        int victim = 0;
        for (int i = 0; i < numFrames; i++) {
                /* all frames are assumed full when we call victim selection */
                if (frame_last[i] < best) {
                        best = frame_last[i];
                        victim = i;
                }
        }
        return victim;
}

static int choose_fifo_victim(void) {
        /* pop current hand, advance hand */
        int v = fifo_hand;
        fifo_hand = (fifo_hand + 1) % numFrames;
        return v;
}

static int choose_clock_victim(void) {
        while (1) {
                if (frame_ref[clock_hand] == 0) {
                        int v = clock_hand;
                        clock_hand = (clock_hand + 1) % numFrames;
                        return v;
                }
                /* give second chance */
                frame_ref[clock_hand] = 0;
                clock_hand = (clock_hand + 1) % numFrames;
        }
}

/* Evict frame f (collect victim info), then load new page into same frame.
   Returns the evicted page info so caller can count disk writes if modified. */
page selectVictim(int page_number, enum repl mode )
{
        page victim;
        int f = 0;
        switch (mode) {
                case random: f = choose_random_victim(); break;
                case fifo:   f = choose_fifo_victim();   break;
                case lru:    f = choose_lru_victim();    break;
                case clock:  f = choose_clock_victim();  break;
                default:     f = 0;                      break;
        }

        /* capture victim info before overwrite */
        victim.pageNo  = frame_page[f];
        victim.modified = frame_dirty[f];

        /* "evict" from frame f and load the new page */
        if (debugmode) {
                if (victim.modified)
                        printf("evict (DIRTY) %8d from frame %d\n", victim.pageNo, f);
                else
                        printf("evict (clean) %8d from frame %d\n", victim.pageNo, f);
        }

        /* install new page */
        frame_page[f]  = page_number;
        frame_dirty[f] = 0;       /* new page starts clean */
        frame_ref[f]   = 1;       /* referenced upon load */
        frame_last[f]  = ++tnow;  /* most recently used now */

        return victim ;
}

int main(int argc, char *argv[])
{
	char	*tracename;
	int	page_number,frame_no, done ;
	int	do_line;
	int	no_events, disk_writes, disk_reads;
 	enum	repl  replace;
	int	allocated=0; 
        unsigned address;
    	char 	rw;
	page	Pvictim;
	FILE	*trace;

        if (argc < 5) {
             printf( "Usage: ./memsim inputfile numberframes replacementmode debugmode \n");
             exit ( -1);
	}
	else {
            tracename = argv[1];	
            trace = fopen( tracename, "r");
            if (trace == NULL ) {
                 printf( "Cannot open trace file %s \n", tracename);
                 exit ( -1);
            }
            numFrames = atoi(argv[2]);
            if (numFrames < 1) {
                printf( "Frame number must be at least 1\n");
                exit ( -1);
            }
            if (strcmp(argv[3], "lru") == 0)
                replace = lru;
            else if (strcmp(argv[3], "rand") == 0)
                replace = random;
            else if (strcmp(argv[3], "clock") == 0)
                replace = clock;		 
            else if (strcmp(argv[3], "fifo") == 0)
                replace = fifo;		 
            else {
                printf( "Replacement algorithm must be rand/fifo/lru/clock  \n");
                exit ( -1);
            }

            if (strcmp(argv[4], "quiet") == 0)
                debugmode = 0;
            else if (strcmp(argv[4], "debug") == 0)
                debugmode = 1;
            else {
                printf( "Debug mode must be quiet/debug  \n");
                exit ( -1);
            }
	}
	
	done = createMMU (numFrames);
	if ( done == -1 ) {
		 printf( "Cannot create MMU" ) ;
		 exit(-1);
        }
	no_events = 0 ;
	disk_writes = 0 ;
	disk_reads = 0 ;

        do_line = fscanf(trace,"%x %c",&address,&rw);
	while ( do_line == 2)
	{
		page_number =  (int)(address >> PAGE_OFFSET);
		frame_no = checkInMemory( page_number ) ;    /* ask for physical address */

		if ( frame_no == -1 )
		{
		  disk_reads++ ;			/* Page fault, need to load it into memory */
		  if (debugmode) 
		      printf( "Page fault %8d \n", page_number) ;
		  if (allocated < numFrames)  			/* allocate it to an empty frame */
		   {
                     frame_no = allocateFrame(page_number);
		     allocated++;
                   }
                   else{
		      Pvictim = selectVictim(page_number, replace) ;   /* evict+install */
		      frame_no = checkInMemory( page_number ) ;        /* find the frame of the new page */
                      /* count write-back if needed (caller prints already) */
                      if (Pvictim.modified)
                          disk_writes++;
		   }
		}

		if ( rw == 'R' || rw == 'r'){
		    if (debugmode) printf( "reading    %8d \n", page_number) ;
                    /* reading: already updated ref/LRU in checkInMemory or on load */
		}
		else if ( rw == 'W' || rw == 'w'){
		    /* mark page in page table as written - modified  */
                    if (frame_no >= 0) frame_dirty[frame_no] = 1;
		    if (debugmode) printf( "writting   %8d \n", page_number) ;
		}
		else {
		      printf( "Badly formatted file. Error on line %d\n", no_events+1); 
		      exit (-1);
		}

		no_events++;
        	do_line = fscanf(trace,"%x %c",&address,&rw);
	}

	printf( "total memory frames:  %d\n", numFrames);
	printf( "events in trace:      %d\n", no_events);
	printf( "total disk reads:     %d\n", disk_reads);
	printf( "total disk writes:    %d\n", disk_writes);
	printf( "page fault rate:      %.4f\n", (float) disk_reads/no_events);

        /* cleanup */
        free(frame_page);
        free(frame_dirty);
        free(frame_ref);
        free(frame_last);
        fclose(trace);
        return 0;
}