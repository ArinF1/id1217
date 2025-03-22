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
#define MAXSIZE 10000 /* maximum matrix size */
#define MAXWORKERS 10 /* maximum number of workers */

/* pthread_mutex_t barrier; */ /* mutex lock for the barrier */
pthread_cond_t go;             /* condition variable for leaving */
int numWorkers;                /* number of workers */
int numArrived = 0;            /* number who have arrived */

// A structure to store the matrix statistics i.e the min/max values
typedef struct
{
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

// taskb - mutex that protects the global sum
pthread_mutex_t sumMutex;
long globalSum = 0;

// taskC - mutex that protects the counter
pthread_mutex_t rowCounterMutex;
int rowCounter = 0;

/* timer */
double read_timer()
{
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if (!initialized)
    {
        gettimeofday(&start, NULL);
        initialized = true;
    }
    gettimeofday(&end, NULL);
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

double start_time, end_time; /* start and end times */
int size, stripSize;         /* assume size is multiple of numWorkers */
/* int sums[MAXWORKERS]; /* partial sums */
int matrix[MAXSIZE][MAXSIZE]; /* matrix */

void *Worker(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[])
{
    int i, j;
    long l; /* use long in case of a 64-bit system */
    pthread_attr_t attr;
    pthread_t workerid[MAXWORKERS];

    /* set global thread attributes */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    /* initialize mutex and condition variable */
    /*   pthread_mutex_init(&barrier, NULL); */
    pthread_cond_init(&go, NULL);

    // initialize the mutexlock
    pthread_mutex_init(&statsMutex, NULL);

    // taskC - initialize the mutex lock
    pthread_mutex_init(&rowCounterMutex, NULL);

    /* read command line args if any */
    size = (argc > 1) ? atoi(argv[1]) : MAXSIZE;
    numWorkers = (argc > 2) ? atoi(argv[2]) : MAXWORKERS;
    if (size > MAXSIZE)
        size = MAXSIZE;
    if (numWorkers > MAXWORKERS)
        numWorkers = MAXWORKERS;
    stripSize = size / numWorkers;

    /* initialize the matrix */
    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            matrix[i][j] = rand() % 99;
        }
    }

    /* print the matrix */
#ifdef DEBUG
    for (i = 0; i < size; i++)
    {
        printf("[ ");
        for (j = 0; j < size; j++)
        {
            printf(" %d", matrix[i][j]);
        }
        printf(" ]\n");
    }
#endif

    // global stat values are predefined
    globalStats.min_value = 99;
    globalStats.max_value = 0;

    /* do the parallel work: create the workers */
    start_time = read_timer();

    // Taskb - create thread workers
    for (long l = 0; l < numWorkers; l++)
    {
        pthread_create(&workerid[l], &attr, Worker, (void *)l);
    }

    // Taskb - waits for threads to finish, then main therad prints the results
    for (long l = 0; l < numWorkers; l++)
    {
        pthread_join(workerid[l], NULL);
    }

    // TaskB - main thread prints everything once
    end_time = read_timer();
    printf("The total is %ld\n", globalSum);
    printf("The execution time is %g sec\n", end_time - start_time);
    printf("Min Value: %d at (%d, %d)\n", globalStats.min_value, globalStats.min_row, globalStats.min_col);
    printf("Max Value: %d at (%d, %d)\n", globalStats.max_value, globalStats.max_row, globalStats.max_col);

    return 0;
}

/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg)
{
    MatrixStats localStats;
    long myid = (long)arg;
    int total = 0, i, j, first, last;

#ifdef DEBUG
    printf("worker %d (pthread id %d) has started\n", myid, pthread_self());
#endif

    // local stats for each worker
    localStats.min_value = 99;
    localStats.max_value = 0;
    localStats.min_row = 0;
    localStats.min_col = 0;
    localStats.max_row = 0;
    localStats.max_col = 0;

    // taskC - get the row to work on safely using mutex locks
    while (1)
    {
        pthread_mutex_lock(&rowCounterMutex);
        int row = rowCounter;
        rowCounter++;
        pthread_mutex_unlock(&rowCounterMutex);

        if (row >= size)
        {
            break;
        }

        for (int j = 0; j < size; j++)
        {
            int val = matrix[row][j];
            total += val;
            /* update local min */
            if (val < localStats.min_value)
            {
                localStats.min_value = val;
                localStats.min_row = row;
                localStats.min_col = j;
            }
            /* update local max */
            if (val > localStats.max_value)
            {
                localStats.max_value = val;
                localStats.max_row = row;
                localStats.max_col = j;
            }
        }
    }

    // Taskb - update the global sum safely
    pthread_mutex_lock(&sumMutex);
    globalSum += total;
    pthread_mutex_unlock(&sumMutex);

    // updates the global stats, done safely via the mutex locks
    pthread_mutex_lock(&statsMutex);
    if (localStats.max_value > globalStats.max_value)
    {
        globalStats.max_value = localStats.max_value;
        globalStats.max_row = localStats.max_row;
        globalStats.max_col = localStats.max_col;
    }
    if (localStats.min_value < globalStats.min_value)
    {
        globalStats.min_value = localStats.min_value;
        globalStats.min_row = localStats.min_row;
        globalStats.min_col = localStats.min_col;
    }
    pthread_mutex_unlock(&statsMutex);

    return NULL;
}