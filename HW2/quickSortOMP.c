/* quicksort_openmp_maxworkers.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <time.h>

#define MAXSIZE 1000000 /* maximum array size */
#define MAXWORKERS 8 /* maximum number of workers */
#define PARALLEL_THRESHOLD 1000 /* If the sub–array size is less than this value, the serial (non-threaded) sort is used.  */
#define MEDIAN_CALC 5  /* number of timing trials to calculate median */

// Swap helper function, further use in pivoting and partitioning
static inline void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Median-of–three pivot selection, improves pivot selection for quicksort
int median_of_three(int *arr, int low, int high) {
    int mid = low + (high - low) / 2;
    if (arr[low] > arr[mid])
        swap(&arr[low], &arr[mid]);
    if (arr[low] > arr[high])
        swap(&arr[low], &arr[high]);
    if (arr[mid] > arr[high])
        swap(&arr[mid], &arr[high]);
    return mid;
}

// Partition function to divide the array into two parts
int partition(int *arr, int low, int high) {
    int pivotIndex = median_of_three(arr, low, high);
    swap(&arr[pivotIndex], &arr[high]);  // Move pivot to end
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return i + 1;
}

// Serial quicksort if array less than threshold, this will be called recursively
void serial_quicksort(int *arr, int low, int high) {
    if (low < high) {
        int pivot = partition(arr, low, high);
        serial_quicksort(arr, low, pivot - 1);
        serial_quicksort(arr, pivot + 1, high);
    }
}

// The recursive parallel routine that decides whether to spawn a thread or sort serially
void parallel_quicksort(int *arr, int low, int high) {
    if (low < high) {
        if (high - low < PARALLEL_THRESHOLD) {
            // For smaller arrays, serially sorted.
            serial_quicksort(arr, low, high);
        } else {
            // For larger arrays, partition function is called and then the left partition is sorted in a new thread.
            int pivot = partition(arr, low, high);
            #pragma omp task shared(arr) firstprivate(low, pivot)
            {
                parallel_quicksort(arr, low, pivot - 1);
            }
            #pragma omp task shared(arr) firstprivate(high, pivot)
            {
                parallel_quicksort(arr, pivot + 1, high);
            }
            #pragma omp taskwait
        }
    }
}

// Sort an array for median calculation
void sort_array(double arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (arr[i] > arr[j]) {
                double tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
}

// Compute the median of an array
double median_val(double arr[], int n) {
    sort_array(arr, n);
    return arr[n / 2];
}

int main(int argc, char *argv[]) {
    /* read command line args if any */
    int n = (argc > 1) ? atoi(argv[1]) : MAXSIZE;
    int numWorkers = (argc > 2) ? atoi(argv[2]) : MAXWORKERS;
    if (numWorkers > MAXWORKERS) {
        numWorkers = MAXWORKERS;
    }

    // Allocate memory for original data array and working copy.
    int *orig = (int *)malloc(n * sizeof(int));
    int *arr  = (int *)malloc(n * sizeof(int));
    
    // Initialize the array with random values.
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        orig[i] = rand() % 1000;  
    }

    double seq_times[MEDIAN_CALC], par_times[MEDIAN_CALC];

    //Sequential sorting 
    for (int trial = 0; trial < MEDIAN_CALC; trial++) {
        memcpy(arr, orig, n * sizeof(int));

        double start = omp_get_wtime();
        serial_quicksort(arr, 0, n - 1);
        double end = omp_get_wtime();

        seq_times[trial] = end - start;
    }

    //Parallel sorting
    for (int trial = 0; trial < MEDIAN_CALC; trial++) {
        // Copy the unsorted data into the working array.
        memcpy(arr, orig, n * sizeof(int));

        double start = omp_get_wtime();
        #pragma omp parallel num_threads(numWorkers)
        {
            #pragma omp single nowait
            {
                parallel_quicksort(arr, 0, n - 1);
            }
        }
        double end = omp_get_wtime();

        par_times[trial] = end - start;
    }

    // Compute median execution times and the speedup.
    double seq_median = median_val(seq_times, MEDIAN_CALC);
    double par_median = median_val(par_times, MEDIAN_CALC);
    double speedup = seq_median / par_median;
    for(int i = 0; i<n-1; i++){
        if(arr[i]> arr[i+1]){
            printf("unsorted");
            break;
        }
    }
    printf("Median Sequential Time: %g seconds\n", seq_median);
    printf("Median Parallel Time: %g seconds\n", par_median);
    printf("Speedup (Sequential / Parallel): %g\n", speedup);

    free(orig);
    free(arr);
    return 0;
}