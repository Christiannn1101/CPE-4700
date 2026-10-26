#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <sched.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "encoder.h"
#include "pwm.h"
#include "imu.h"
#include "balance_loop.h"
#include "gui.h"

#define INTERVAL_NS 1000000L

atomic_bool running = true;  

static void on_sigint(int s) { (void)s; running = false; }

/* 1 ms balance loop, runs in its own thread */
static void *control_thread(void *arg)
{
    (void)arg;
    struct timespec next_time;

    /* Real-time priority (needs root; silently skipped otherwise) */
    struct sched_param sp = { .sched_priority = 50 };
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);

    clock_gettime(CLOCK_MONOTONIC, &next_time);

    for (unsigned int n = 0; running; ++n) {
        next_time.tv_nsec += INTERVAL_NS;
        if (next_time.tv_nsec >= 1000000000L) {
            next_time.tv_sec += 1;
            next_time.tv_nsec -= 1000000000L;
        }
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL) != 0)
            break;

        balance_loop_step(n);
    }

    running = false;          /* tells the GUI to close */
    return NULL;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, on_sigint);

    pwm_init();
    encoder_init();
    imu_init();
    balance_loop_init();

    pthread_t tid;
    pthread_create(&tid, NULL, control_thread, NULL);

    gui_init(argc, argv);     

    running = false;
    pthread_join(tid, NULL);  

    balance_loop_cleanup();   /* zero motors first */
    pwm_cleanup();
    encoder_cleanup();
    imu_cleanup();
    return 0;
}