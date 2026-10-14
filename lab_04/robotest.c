#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <math.h>
#include <signal.h>


#include "encoder.h"
#include "pwm.h"
#include "step_test.h"
#include "speed_control.h"


atomic_bool running = true;

#define PWMAX 0.6
#define INTERVAL_NS 1000000 

//MAX PWM = 0.6

int main(void){

    struct timespec next_time;
    pwm_init();
    encoder_init();
    
        // Setter method for PWM duty cycle (0.0 to 1.0)
        //Motor 0 = R OR 1 = L
        //RIGHT: -Duty = Forward  +Duty = Backward
        //LEFT: +Duty = Forward  -Duty = Backward
        //pwm_set_duty(int motor, double duty);
        //double encoder_get_speed(int encoder_id);
        //long encoder_get_count(int encoder_id);
        //double encoder_get_direction(int encoder_id);
        //encoder_get_speed(int motor );
        //encoder_get_position(int motor);
           // 3. Get current monotonic clock time
    if (clock_gettime(CLOCK_MONOTONIC, &next_time) == -1)
    {
        perror("clock_gettime failed");
        exit(EXIT_FAILURE);
    }
         
    //step_test_init();
    speed_control_init();
    for (unsigned int loop_count = 0; running && loop_count < 1000000; ++loop_count){
        // Calculate next absolute wake-up time
        next_time.tv_nsec += INTERVAL_NS;
        if (next_time.tv_nsec >= 1000000000){
            next_time.tv_sec += 1;
            next_time.tv_nsec -= 1000000000;
        }
        // Sleep until the exact calculated absolute time
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL) != 0){
            perror("clock_nanosleep failed");
            break;
        }
        //balance_loop(loop_count);
        //if(step_test(loop_count) == 0){
            // Step test is still active
        //    break;
        //}
        if (speed_control_run(loop_count) == 0){
            // The 3 cycles have finished
            break;
        }

    }  
            
    atomic_store(&running, false); // Signal other threads to stop
    pwm_cleanup();
    encoder_cleanup();
    speed_control_cleanup();
    //step_test_cleanup();

    return 0;
}