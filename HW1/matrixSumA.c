/* matrix summation using pthreads

   features: uses a barrier; the Worker[0] computes
             the total sum from partial sums computed by Workers
             and prints the total sum to the standard output

   usage under Linux:
     gcc matrixSum.c -lpthread
     a.out size numWorkers

*/
#ifndef _REENTRANT 
#define _REENTRANT 
#endif 
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */

pthread_mutex_t barrier;  /* mutex lock for the barrier */
pthread_cond_t go;        /* condition variable for leaving */
int numWorkers;           /* number of workers */ 
int numArrived = 0;       /* number who have arrived */

// A structure to store the matrix statistics i.e the min/max values
typedef struct {
    int max_value;
    int min_value;
    int min_row;
    int min_col;
    int max_row;
    int max_col;
} MatrixStats;

// variable to hold the final values of the matrix statistics
MatrixStats globalStats;

// mutex that protects the global value
pthread_mutex_t statsMutex;


/* a reusable counter barrier */
void Barrier() {
  pthread_mutex_lock(&barrier);
  numArrived++;
  if (numArrived == numWorkers) {
    numArrived = 0;
    pthread_cond_broadcast(&go);
  } else
    pthread_cond_wait(&go, &barrier);
  pthread_mutex_unlock(&barrier);
}

/* timer */
double read_timer() {
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if( !initialized )
    {
        gettimeofday( &start, NULL );
        initialized = true;
    }
    gettimeofday( &end, NULL );
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

double start_time, end_time; /* start and end times */
int size, stripSize;  /* assume size is multiple of numWorkers */
int sums[MAXWORKERS]; /* partial sums */
int matrix[MAXSIZE][MAXSIZE]; /* matrix */

void *Worker(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
  int i, j;
  long l; /* use long in case of a 64-bit system */
  pthread_attr_t attr;
  pthread_t workerid[MAXWORKERS];
  

  /* set global thread attributes */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  /* initialize mutex and condition variable */
  pthread_mutex_init(&barrier, NULL);
  pthread_cond_init(&go, NULL);

    // initialize the mutexlock
 pthread_mutex_init(&statsMutex, NULL);


  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE) size = MAXSIZE;
  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
  stripSize = size/numWorkers;

  /* initialize the matrix */
  for (i = 0; i < size; i++) {
	  for (j = 0; j < size; j++) {
          matrix[i][j] = rand()%99;
	  }
  }

  /* print the matrix */
#ifdef DEBUG
  for (i = 0; i < size; i++) {
	  printf("[ ");
	  for (j = 0; j < size; j++) {
	    printf(" %d", matrix[i][j]);
	  }
	  printf(" ]\n");
  }
#endif

//global stat values are predefined
 globalStats.min_value = 99;
 globalStats.max_value = 0;

  /* do the parallel work: create the workers */
  start_time = read_timer();
  for (l = 0; l < numWorkers; l++)
    pthread_create(&workerid[l], &attr, Worker, (void *) l);
  pthread_exit(NULL);
}

/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg) {
MatrixStats localStats;
  long myid = (long) arg;
  int total, i, j, first, last;

#ifdef DEBUG
  printf("worker %d (pthread id %d) has started\n", myid, pthread_self());
#endif

  /* determine first and last rows of my strip */
  first = myid*stripSize;
  last = (myid == numWorkers - 1) ? (size - 1) : (first + stripSize - 1);


  // local stats for each worker
  localStats.min_value = 99;
  localStats.max_value = 0;
  localStats.min_row   = 0;
  localStats.min_col   = 0;
  localStats.max_row   = 0;
  localStats.max_col   = 0;

  /* sum values in my strip */
  total = 0;
  for (i = first; i <= last; i++) {
    for (j = 0; j < size; j++) {
      total += matrix[i][j];
      if(localStats.max_value < matrix[i][j]){
        localStats.max_value = matrix[i][j];
        localStats.max_row = i;
        localStats.max_col = j;
      }
      if(localStats.min_value > matrix[i][j]){
        localStats.min_value = matrix[i][j];
        localStats.min_row = i;
        localStats.min_col = j;
      }
    }
  }
  
  // updates the global stats, done safely via the mutex locks
  pthread_mutex_lock(&statsMutex);
  if (localStats.max_value > globalStats.max_value) {
    globalStats.max_value = localStats.max_value;
    globalStats.max_row = localStats.max_row;
    globalStats.max_col = localStats.max_col;
  }
  if (localStats.min_value < globalStats.min_value) {
    globalStats.min_value = localStats.min_value;
    globalStats.min_row = localStats.min_row;
    globalStats.min_col = localStats.min_col;
  }
  pthread_mutex_unlock(&statsMutex);

  sums[myid] = total;
  Barrier();

  if (myid == 0) {
    total = 0;
    for (i = 0; i < numWorkers; i++)
      total += sums[i];
    /* get end time */
    end_time = read_timer();
    /* print results */
    printf("The total is %d\n", total);
    printf("The execution time is %g sec\n", end_time - start_time);

    // print the global stats
    printf("Min Value: %d at (%d, %d)\n", globalStats.min_value, globalStats.min_row, globalStats.min_col);
    printf("Max Value: %d at (%d, %d)\n", globalStats.max_value, globalStats.max_row, globalStats.max_col);
  }  

}
