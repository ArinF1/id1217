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
#define PARALLEL_THRESHOLD 5000  /* If the sub–array size is less than this value, the serial (non-threaded) sort is used.  */

// Global timing variables
double start_time, end_time; 

// Array 
int matrix[MAXSIZE];
int size, numWorkers;

pthread_mutex_t barrier;  /* mutex lock for the barrier */
pthread_cond_t  go;       /* condition variable for leaving */
int numArrived = 0;       /* number who have arrived */

void Barrier() {
  pthread_mutex_lock(&barrier);
  numArrived++;
  if (numArrived == numWorkers) {
    numArrived = 0;
    pthread_cond_broadcast(&go);
  } else {
    pthread_cond_wait(&go, &barrier);
  }
  pthread_mutex_unlock(&barrier);
}

double read_timer()
{
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if (!initialized) {
        gettimeofday(&start, NULL);
        initialized = true;
    }
    gettimeofday(&end, NULL);
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

// Structure to pass quicksort arguments
typedef struct {
    int  low;
    int  high;
    int* array;
} QuickSortArray;

// Forward declaration for the quickSort thread function.
void *quickSort(void *arg);


// Swap helper function, further use in pivoting and partitioning
static void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

// Median-of–three pivot selection, improves pivot selection for quicksort
static int medianOfThree(int low, int high, int *array) {
    int mid = low + (high - low) / 2; // not always the left most or right most element
    if (array[low] > array[mid])
        swap(&array[low], &array[mid]);
    if (array[low] > array[high])
        swap(&array[low], &array[high]);
    if (array[mid] > array[high])
        swap(&array[mid], &array[high]);
    return mid;
}

// Partition function to divide the array into two parts
static int partition(int *arr, int low, int high) {
    int pivotIndex = medianOfThree(low, high, arr);
    swap(&arr[pivotIndex], &arr[high]);  // move pivot to end
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i+1], &arr[high]);
    return i + 1;
}

// Serial quicksort if array less than threshold, this will be called recursively
static void serialQuickSort(int low, int high, int *arr) {
    if (low < high) {
        int pivotIndex = partition(arr, low, high);
        serialQuickSort(low, pivotIndex - 1, arr);
        serialQuickSort(pivotIndex + 1, high, arr);
    }
}

// The recursive parallel routine that decides whether to spawn a thread or sort serially
static void parallelQuickSort(int low, int high, int *arr) {
    if (low < high) {
        int n = high - low + 1;
        if (n < PARALLEL_THRESHOLD) {
            // For smaller arrays, serially sorted.
            serialQuickSort(low, high, arr);
        } else {
            // For larger arrays, partition function is called and then the left partition is sorted in a new thread.
            int pivotIndex = partition(arr, low, high);
            
            // This will memory allocate a quicksort structure for the left partition,
            QuickSortArray *leftArgs = malloc(sizeof(QuickSortArray));
            leftArgs->low = low;
            leftArgs->high = pivotIndex - 1;
            leftArgs->array = arr;
            
            pthread_t leftThread;

            // Create a new thread to sort the left partition in parallel.
            pthread_create(&leftThread, NULL, quickSort, (void *) leftArgs);
            
            // Sort the right partition in the current thread.
            parallelQuickSort(pivotIndex + 1, high, arr);
            
            // Wait for the left partition thread to finish before proceeding.
            pthread_join(leftThread, NULL);
        }
    }
}

// Thread function for parallel quicksort.
void *quickSort(void *arg) {
    // Void pointer to the argument structure to access parameters.
    QuickSortArray *qsArgs = (QuickSortArray *) arg;

    // Call the parallel quicksort
    parallelQuickSort(qsArgs->low, qsArgs->high, qsArgs->array);

    // Free the allocated memory for the argument structure.
    free(qsArgs);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    pthread_attr_t attr; // Thread attributes variable.
    
    // Initialize the mutex and condition variable for the barrier.
    pthread_mutex_init(&barrier, NULL);
    pthread_cond_init(&go, NULL);


    size       = (argc > 1) ? atoi(argv[1]) : 100;
    numWorkers = (argc > 2) ? atoi(argv[2]) : 2;

    if (size > MAXSIZE) {
        size = MAXSIZE;
    }
    if (numWorkers > MAXWORKERS) {
        numWorkers = MAXWORKERS;
    }

    // Initialize the array with random values.
    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        matrix[i] = rand() % 1000;  
    }

    start_time = read_timer();

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    // Allocate memory for the quicksort argument structure and initialize the parameters.
    QuickSortArray *qsArgs = malloc(sizeof(QuickSortArray));
    qsArgs->low   = 0;
    qsArgs->high  = size - 1;
    qsArgs->array = matrix;

    pthread_t topThread;
    pthread_create(&topThread, &attr, quickSort, (void *) qsArgs);
    pthread_join(topThread, NULL);

    end_time = read_timer();

    /*
    printf("Sorted array:\n");
    for (int i = 0; i < size; i++) {
        printf("%d ", matrix[i]);
    }
    printf("\n\n");
    */

    printf("The execution time is %g sec\n", end_time - start_time);

    return 0;
}
