/*
 * Q1: gettimeofday() is precise to about 1 microsecond. A timed section should
 *     take at least about 1 millisecond to be measured well, so repeat the page
 *     accesses many times in a loop and divide by the number of accesses.
 *
 * Q2: The program is below. Usage: ./tlb <num_pages> <num_trials>
 *
 * Q5: Make the array volatile so every access really happens, and use the
 *     values afterward (for example, print them). Compiling with -O0 also helps.
 *
 * Q6: Pin the process to one CPU with sched_setaffinity() (done below), or run
 *     it with "taskset -c 0 ./tlb". If you don't, the process can move to another
 *     CPU with a cold TLB, which adds extra misses and makes the timing noisy.
 *
 * Q7: Yes. The first touch of each page is slow because of page faults and
 *     demand zeroing. Fix it by touching every page once before starting the timer.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <num_pages> <num_trials>\n", argv[0]);
        return 1;
    }

    long num_pages = atol(argv[1]);
    long trials = atol(argv[2]);
    if (num_pages <= 0 || trials <= 0) {
        fprintf(stderr, "num_pages and num_trials must be positive\n");
        return 1;
    }

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0)
        perror("sched_setaffinity");

    long page_size = sysconf(_SC_PAGESIZE);
    long jump = page_size / sizeof(int);
    long n = num_pages * jump;

    volatile int *a = malloc(n * sizeof(int));
    if (a == NULL) {
        perror("malloc");
        return 1;
    }

    for (long i = 0; i < n; i += jump)
        a[i] = 0;

    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (long t = 0; t < trials; t++)
        for (long i = 0; i < n; i += jump)
            a[i] += 1;
    gettimeofday(&end, NULL);

    double total_us = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
    double ns_per_access = total_us * 1000.0 / ((double)trials * num_pages);

    printf("pages: %ld  trials: %ld  time per access: %.2f ns  (check: %d)\n",
           num_pages, trials, ns_per_access, a[0]);

    free((void *)a);
    return 0;
}
