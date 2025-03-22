/******************************************************************************
 Unisex Bathroom Simulation 
 ******************************************************************************/

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <time.h>
 #include <pthread.h>
 #include <semaphore.h>
 
 #define NUM_MEN    25
 #define NUM_WOMEN  25
 
 // Global flag: while true, threads continue; when false, they finish up.
 static volatile int keepRunning = 1;
 
 // Turn indicator: 0 = no one, 1 = men, 2 = women.
 int turn = 0;
 int menInside = 0, womenInside = 0;
 int menWaiting = 0, womenWaiting = 0;
 
 // Fairness counters: Thread counter increments, when a thread enters the bathroom
 int manCounter[NUM_MEN] = {0};
 int womanCounter[NUM_WOMEN] = {0};
 
 // Semaphores
 sem_t lock;
 sem_t menQueue;
 sem_t womenQueue;
 
 // Simulate "working" outside the bathroom by sleeping for a random time.
 void do_work(const char *who, int id) {
     usleep(rand() % 4000000);  // change value here to increase/decrease time outside the bathroom, up to 4sec
 }
 
 // Simulate using the bathroom by sleeping for a shorter random time.
 void use_bathroom(const char *who, int id) {
     usleep(rand() % 500000);  //up to 0.5 seconds
 }
 
//Man thread
 void *man(void *arg) {
     int id = *(int *)arg;
     while (keepRunning) {
         do_work("Man", id);
 
         sem_wait(&lock);
         if (womenInside > 0 || (turn == 2 && womenWaiting > 0)) {
             menWaiting++;
             printf("Man %d is waiting.\n", id);
             sem_post(&lock);
 
             sem_wait(&menQueue);  // Block until signaled.
             sem_wait(&lock);
             menWaiting--;
         }
         menInside++;
         turn = 1;  // Set turn to men.
         manCounter[id]++;  // Count this bathroom visit.
         printf("Man %d enters (menInside=%d).\n", id, menInside);
         sem_post(&lock);
 
         use_bathroom("Man", id);
 
         sem_wait(&lock);
         menInside--;
         printf("Man %d leaves (menInside=%d).\n", id, menInside);
         if (menInside == 0 && womenWaiting > 0) {
             turn = 2;
             sem_post(&womenQueue);
         }
         sem_post(&lock);
     }
     return NULL;
 }
 
//Woman thread
 void *woman(void *arg) {
     int id = *(int *)arg;
     while (keepRunning) {
         do_work("Woman", id);
        
         sem_wait(&lock);
         if (menInside > 0 || (turn == 1 && menWaiting > 0)) {
             womenWaiting++;
             printf("Woman %d is waiting.\n", id);
             sem_post(&lock);
 
             sem_wait(&womenQueue);
             sem_wait(&lock);
             womenWaiting--;
         }
         womenInside++;
         turn = 2;  // Set turn to women.
         womanCounter[id]++;  // Count this bathroom visit.
         printf("Woman %d enters (womenInside=%d).\n", id, womenInside);
         sem_post(&lock);
 
         use_bathroom("Woman", id);
 
         sem_wait(&lock);
         womenInside--;
         printf("Woman %d leaves (womenInside=%d).\n", id, womenInside);
         if (womenInside == 0 && menWaiting > 0) {
             turn = 1;
             sem_post(&menQueue);
         }
         sem_post(&lock);
     }
     return NULL;
 }
 
 
 int main() {
     srand(time(NULL));
 
     // Initialize semaphores.
     sem_init(&lock, 0, 1);
     sem_init(&menQueue, 0, 0);
     sem_init(&womenQueue, 0, 0);
 
     pthread_t menThreads[NUM_MEN];
     pthread_t womenThreads[NUM_WOMEN];
     int menIDs[NUM_MEN];
     int womenIDs[NUM_WOMEN];
 
     // Create man threads
     for (int i = 0; i < NUM_MEN; i++) {
         menIDs[i] = i;
         pthread_create(&menThreads[i], NULL, man, &menIDs[i]);
     }
     // Create woman threads
     for (int i = 0; i < NUM_WOMEN; i++) {
         womenIDs[i] = i;
         pthread_create(&womenThreads[i], NULL, woman, &womenIDs[i]);
     }

     //simulation timer
     sleep(30);
 
     // Signal threads to stop
     keepRunning = 0;
 
     // Unblock any threads waiting on the semaphores.
     for (int i = 0; i < NUM_MEN; i++) {
         sem_post(&menQueue);
     }
     for (int i = 0; i < NUM_WOMEN; i++) {
         sem_post(&womenQueue);
     }
 
     // Join all threads.
     for (int i = 0; i < NUM_MEN; i++) {
         pthread_join(menThreads[i], NULL);
     }
     for (int i = 0; i < NUM_WOMEN; i++) {
         pthread_join(womenThreads[i], NULL);
     }
 
     printf("\nSimulation complete.\n");

     // Final counters printed.
     for (int i = 0; i < NUM_MEN; i++) {
         printf("Man %d entered the bathroom %d times.\n", i, manCounter[i]);
     }
     for (int i = 0; i < NUM_WOMEN; i++) {
         printf("Woman %d entered the bathroom %d times.\n", i, womanCounter[i]);
     }
 
     // Clean up semaphores.
     sem_destroy(&lock);
     sem_destroy(&menQueue);
     sem_destroy(&womenQueue);
 
     return 0;
 }
 
