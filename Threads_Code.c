#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#define MAX_THREADS 800

/*This program utilizes threads to compute operations on an array in order to improve efficiency*/

/*Structure to store argument information of methods*/
struct data {
   int *arr, start, end;
};

struct timeval tv_delta(struct timeval start, struct timeval end) {
   struct timeval delta = end;

   delta.tv_sec -= start.tv_sec;
   delta.tv_usec -= start.tv_usec;
   if (delta.tv_usec < 0) {
      delta.tv_usec += 1000000;
      delta.tv_sec--;
   }
   return delta;
}

/*method to find max element of an array*/
void *max_num(void *data) {
   struct data arg = *(struct data *) data; 
   int i, *max = malloc(sizeof(int));

   *max = arg.arr[arg.start];
   for (i = arg.start; i < arg.end; i++) {
      if (arg.arr[i] > *max) {
	 *max = arg.arr[i];
      }
   }
   return max;
}

/*method to find sum using formula of array*/
void *array_sum(void *data) {
   struct data arg = *(struct data *) data; 
   int i, *sum = malloc(sizeof(int));
   
   *sum = 0;
   for (i = arg.start; i < arg.end; i++) {
      *sum = (*sum + arg.arr[i]) % 1000000;
   }
   return sum;
}

int main(int argc, char* argv[]) {
   int i, num_elem, num_thread, seed, task, *arr, 
       max, max_test, sum = 0, sum_test = 0;
   pthread_t tids[MAX_THREADS + 1];
   struct data arg[MAX_THREADS  + 1];
   void *result_ptr = NULL;
   struct rusage start_ru, end_ru;
   struct timeval start_wall, end_wall;
   struct timeval diff_ru_utime, diff_wall, diff_ru_stime;

   if (argc != 6) {
      perror("Not enough arguments provided");
      return 1;
   }
   num_elem = atoi(argv[1]);
   num_thread = atoi(argv[2]);
   seed = atoi(argv[3]);
   task = atoi(argv[4]);
   srand(seed);
   arr = malloc(num_elem * sizeof(int));
   for (i = 0; i < num_elem; i++) {
      int random_value = rand();
      
      arr[i] = random_value;
   }
   /*max*/
   /*start clock*/
   getrusage(RUSAGE_SELF, &start_ru);
   gettimeofday(&start_wall, NULL);
   if (task == 1) {
      for (i = 0; i < num_thread; i++) {
	 /*utilize an array for arguments for each thread
	   and fill them with information about the arugments
	   depending on start/end point of thread*/
         arg[i].arr = arr;
         arg[i].start = (num_elem / num_thread) * i;
	 if (i == num_thread - 1) {
	    /*prevent out of bound array access*/
	    arg[i].end = num_elem;
	 } 
	 else {
	    arg[i].end = (num_elem / num_thread) * (i + 1);
	 }
         pthread_create(&tids[i], NULL, max_num, &arg[i]);
      }
      max = arr[0];
      for (i = 0; i < num_thread; i++) {
	 /*reap all threads*/
         pthread_join(tids[i], &result_ptr);
	 /*compare returned value of thread to max*/
         if(*(int *) result_ptr > max) {
	    max = *(int *) result_ptr;
	 }
	 /*free memory*/
         free(result_ptr);
      } 
   }
   /*sum*/
   else if (task == 2) {
      for (i = 0; i < num_thread; i++) {
         arg[i].arr = arr;
         arg[i].start = (num_elem / num_thread) * i;
	 if (i == num_thread - 1) {
	    arg[i].end = num_elem;
	 } 
	 else {
	    arg[i].end = (num_elem / num_thread) * (i + 1);
	 }
         pthread_create(&tids[i], NULL, array_sum, &arg[i]);
      }
      for (i = 0; i < num_thread; i++) {
         pthread_join(tids[i], &result_ptr);
	 /*add result of each thread to total sum*/
         sum = (sum + *(int *) result_ptr) % 1000000;
         free(result_ptr);
      }
   }
   else {
      perror("Invalid task");
      return 1;
   }
   /*end clock*/
   gettimeofday(&end_wall, NULL);
   getrusage(RUSAGE_SELF, &end_ru);
   diff_ru_utime = tv_delta(start_ru.ru_utime, end_ru.ru_utime);
   diff_ru_stime = tv_delta(start_ru.ru_stime, end_ru.ru_stime);
   diff_wall = tv_delta(start_wall, end_wall);
   /*check if results were requested*/
   if (argv[5][0] == 'Y') {
      if (task == 1) {
	 /*check if result of threads was correct*/
	 max_test = arr[0];
         for (i = 0; i < num_elem; i++) {
	    if(arr[i] > max_test) {
	       max_test = arr[i];
	    }
	 }
	 printf("Max elem using for-loop is: %d\n", max_test);
         printf("Maximum value found by threads: %d\n", max);
      }
      else {
	 for (i = 0; i < num_elem; i++) {
	    sum_test = (sum_test + arr[i]) % 1000000;
	 }
	 printf("Sum using for-loop is: %d\n", sum_test);
         printf("Sum found by threads: %d\n", sum);
      }
   }
   /*print time results*/
   printf("User time: %ld.%06ld\n", diff_ru_utime.tv_sec,
          diff_ru_utime.tv_usec);
   printf("System time: %ld.%06ld\n", diff_ru_stime.tv_sec,
          diff_ru_stime.tv_usec);
   printf("Wall time: %ld.%06ld\n", diff_wall.tv_sec, diff_wall.tv_usec);
   free(arr);
   return 0;
}
