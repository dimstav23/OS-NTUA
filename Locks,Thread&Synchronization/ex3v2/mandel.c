/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <semaphore.h>
#include "mandel-lib.h"
#include <pthread.h>

#define MANDEL_MAX_ITERATION 100000

#define perror_pthread(ret, msg) \
        do { errno = ret; perror(msg); } while (0)

/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;
sem_t *semaphore;
/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
	
/*Every character in the final output is
 xstep x ystep units wide on the complex plane. */
double xstep;
double ystep;

/*A (distinct) instance of this structure
  is passed to each thread */

struct thread_info_struct {
        pthread_t tid; 	/* POSIX thread id, as returned by the library */
        int tnumber; 	// px thread number 1
        int numofthrd;	// numofthreads is always the given number
};
typedef struct thread_info_struct * threadptr;


int safe_atoi(char *s, int *val)
{
        long l;
        char *endp;

        l = strtol(s, &endp, 10);
        if (s != endp && *endp == '\0') {
                *val = l;
                return 0;
        } else
                return -1;
}

void *safe_malloc(size_t size)
{
        void *p;

        if ((p = malloc(size)) == NULL) {
                fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
                        size);
                exit(1);
        }

        return p;
}


/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val`:[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;
	
	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}


void compute_and_output_mandel_line(int fd, int line)
{
	/*
	  A temporary array, used to hold color values for the line being drawn
	 */
	int color_val[x_chars];
	
	compute_mandel_line(line, color_val);
	output_mandel_line(fd, color_val);
	
}



void *thread_start_fn(void *arg)
{
        int line;
	//int color_val[x_chars];
        /* We know arg points to an instance of thread_info_struct */
        threadptr thr = arg;

        //fprintf(stderr, "Thread %d of %d. START.\n", thr->tnumber, thr->numofthrd);

	/*draw the Mandelbrot Set, one line at a time.
         Output is sent to file descriptor '1', i.e., standard output. */
	for (line = thr->tnumber ; line < y_chars; line += thr->numofthrd ) {
		//compute_mandel_line(line, color_val);
		sem_wait(&semaphore[line%thr->numofthrd]);
                compute_and_output_mandel_line(1, line);
		//output_mandel_line(1, color_val);
		//printf("Computing thread: %d\n", thr->tnumber);
		sem_post(&semaphore[(thr->tnumber +1)%(thr->numofthrd)]);
        }
	
        //fprintf(stderr, "Thread %d of %d. END.\n", thr->tnumber, thr->numofthrd);
        return NULL;
}

int main(int argc, char *argv[])
{
	int line, NTHREADS, i;
	threadptr tarray;
	int ret;
	xstep = (xmax - xmin) / x_chars;
        ystep = (ymax - ymin) / y_chars;

	//elenxos swsths eisodou
	if(argc != 2) {
		fprintf(stderr, "Usage: %s NTHREADS\n"
			"Exactly one argument required:\n"
			"	NTHREADS: The number of threads to create.\n",argv[0]);
		exit(1);
	}
	

	if (safe_atoi(argv[1], &NTHREADS) < 0 || NTHREADS <= 0) {
                fprintf(stderr, "`%s' is not valid for `NTHREADS'\n", argv[2]);
                exit(1);
        }
	
	//printf("mpla\n");
        tarray = safe_malloc(NTHREADS * sizeof(*tarray));
	//printf("lala\n");
	semaphore = safe_malloc(NTHREADS * sizeof(*semaphore));
	//printf("lolo\n");

	for(line = 0 ; line < NTHREADS ; line++){
		sem_init(&semaphore[line], 0, 1);
		sem_wait(&semaphore[line]);	
	}

	//sem_post(&semaphore[0]);
	for(line=0; line < NTHREADS ; line++){
		tarray[line].tnumber = line;
		tarray[line].numofthrd = NTHREADS;
                /* Spawn new thread */
                ret = pthread_create(&tarray[line].tid, NULL, thread_start_fn, &tarray[line]);
                if (ret) {
                        perror_pthread(ret, "pthread_create");
                        exit(1);
                }
	//	printf("I manage to create the fucking thread\n");
        }
		
	sem_post(&semaphore[0]);

	for (i = 0; i < NTHREADS; i++) {
		ret = pthread_join(tarray[i].tid, NULL);
		//printf("fdfsdfsdf\n");
		if (ret) {
			perror_pthread(ret, "pthread_join");
			exit(1);
		}
	}
	
	reset_xterm_color(1);
	return 0;
}
