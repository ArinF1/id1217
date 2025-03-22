/* matrix summation using OpenMP
usage with gcc (version 4.2 or higher required):
gcc -O -fopenmp -o matrixSum-openmp matrixSum-openmp.c
./matrixSum-openmp size numWorkers
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>  // for INT_MAX and INT_MIN                 
#include <omp.h>

#define MAXSIZE 10000    /* maximum matrix size */
#define MAXWORKERS 8     /* maximum number of workers */
#define MEDIAN_CALC 5   /* number of timing trials to calculate median */

// Sort an array for median calculation
void sort_array(double arr[], int n) {
    int i, j;
    for (i = 0; i < n - 1; i++) {
        for (j = i + 1; j < n; j++) {
            if (arr[i] > arr[j]) {
                double temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
}

// Compute the median of an array
double median(double arr[], int n) {
    sort_array(arr, n);
    return arr[n / 2];
}

int numWorkers;
int size;
int matrix[MAXSIZE][MAXSIZE];

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
    int i, j;
    double start_time, end_time;
    double seq_times[MEDIAN_CALC], par_times[MEDIAN_CALC];
    
    /* read command line args if any */
    size = (argc > 1) ? atoi(argv[1]) : MAXSIZE;
    numWorkers = (argc > 2) ? atoi(argv[2]) : MAXWORKERS;
    if (size > MAXSIZE) size = MAXSIZE;
    if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
    
    /* initialize the matrix */
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            matrix[i][j] = rand() % 99;
        }
    }
    
    // Variables to store results after computing the matrix.
    int total, global_min, global_max;
    int global_min_i, global_min_j, global_max_i, global_max_j;
    
    for (int trial = 0; trial < MEDIAN_CALC; trial++) {
        // Reset results.
        total = 0;
        global_min = INT_MAX;
        global_max = INT_MIN;
        global_min_i = global_min_j = -1;
        global_max_i = global_max_j = -1;
        
        // for sequential, forced 1 thread
        omp_set_num_threads(1); 
        
        start_time = omp_get_wtime();
        #pragma omp parallel
        {
            int local_sum = 0;
            int local_min = INT_MAX, local_max = INT_MIN;
            int local_min_i = -1, local_min_j = -1;
            int local_max_i = -1, local_max_j = -1;
            
            #pragma omp for private(j)
            for (i = 0; i < size; i++) {
                for (j = 0; j < size; j++) {
                    int value = matrix[i][j];
                    local_sum += value;
                    if (value < local_min) {
                        local_min = value;
                        local_min_i = i;
                        local_min_j = j;
                    }
                    if (value > local_max) {
                        local_max = value;
                        local_max_i = i;
                        local_max_j = j;
                    }
                }
            }
            #pragma omp critical
            {
                total += local_sum;
                if (local_min < global_min) {
                    global_min = local_min;
                    global_min_i = local_min_i;
                    global_min_j = local_min_j;
                }
                if (local_max > global_max) {
                    global_max = local_max;
                    global_max_i = local_max_i;
                    global_max_j = local_max_j;
                }
            }
        } 
        end_time = omp_get_wtime();
        
        seq_times[trial] = end_time - start_time;
    }
    
    double seq_median = median(seq_times, MEDIAN_CALC);
    
    for (int trial = 0; trial < MEDIAN_CALC; trial++) {
        // Reset results.
        total = 0;
        global_min = INT_MAX;
        global_max = INT_MIN;
        global_min_i = global_min_j = -1;
        global_max_i = global_max_j = -1;
        
        // specified number of threads
        omp_set_num_threads(numWorkers);  
        
        start_time = omp_get_wtime();
        #pragma omp parallel
        {
            int local_sum = 0;
            int local_min = INT_MAX, local_max = INT_MIN;
            int local_min_i = -1, local_min_j = -1;
            int local_max_i = -1, local_max_j = -1;
            
            #pragma omp for private(j)
            for (i = 0; i < size; i++) {
                for (j = 0; j < size; j++) {
                    int value = matrix[i][j];
                    local_sum += value;
                    if (value < local_min) {
                        local_min = value;
                        local_min_i = i;
                        local_min_j = j;
                    }
                    if (value > local_max) {
                        local_max = value;
                        local_max_i = i;
                        local_max_j = j;
                    }
                }
            }
            #pragma omp critical
            {
                total += local_sum;
                if (local_min < global_min) {
                    global_min = local_min;
                    global_min_i = local_min_i;
                    global_min_j = local_min_j;
                }
                if (local_max > global_max) {
                    global_max = local_max;
                    global_max_i = local_max_i;
                    global_max_j = local_max_j;
                }
            }
        } 
        end_time = omp_get_wtime();
        
        par_times[trial] = end_time - start_time;
    }
    
    double par_median = median(par_times, MEDIAN_CALC);
    double speedup = seq_median / par_median;
    
    // Print results
    printf("Total Sum: %d\n", total);
    printf("Minimum element: %d at position [%d][%d]\n", global_min, global_min_i, global_min_j);
    printf("Maximum element: %d at position [%d][%d]\n", global_max, global_max_i, global_max_j);
    printf("\nMedian Sequential Time: %g seconds\n", seq_median);
    printf("Median Parallel Time: %g seconds\n", par_median);
    printf("Speedup (Sequential / Parallel): %g\n", speedup);
    
    return 0;
}
