#include "speed_control.h"
#include "pwm.h"
#include "encoder.h"
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

static bool speed_control_active = true;
static FILE *speed_log = NULL;

void speed_control_init(void)
{
    speed_log = fopen("speed_control.csv", "w");
    if (speed_log == NULL)
    {
        perror("speed_control.csv");
        speed_control_active = false;
        return;
    }
    // CSV header required by the instructions
    fprintf(speed_log, "time_ms,target_speed,measured_speed\n");
    speed_control_active = true;
}

// Main control function required by the interface
void speed_control(int motor, double target_speed)
{
    // 1. Get the current measured speed from the encoder
    double measured_speed = encoder_get_speed(motor);

    // 2. Compute control action to determine the PWM duty cycle (max 0.6)
    double duty = target_speed / 100.0;
    if (duty > 0.6) duty = 0.6;
    if (duty < -0.6) duty = -0.6;

    // 3. Apply the computed command to the motor
    pwm_set_duty(motor, duty);
}

// Helper function to handle the triangular cycle inside the real-time loop
int speed_control_run(unsigned int loop_count)
{
    if (!speed_control_active)
    {
        return 0;
    }

    int motor = 0; // Motor channel (0=Right or 1=Left)
    int t_ms = loop_count;
    double elapsed_seconds = t_ms / 1000.0;

    // 3 cycles of 2.0 seconds = 6.0 seconds total (6000 iterations at 1ms)
    if (elapsed_seconds >= 6.0)
    {
        speed_control_active = false;
        return 0; // Breaks the main loop
    }

    // Determine the time within the 2-second cycle
    double cycle_time = fmod(elapsed_seconds, 2.0);
    double target_speed;

    // Triangular reference generation
    if (cycle_time < 1.0)
        target_speed = 50.0 - 100.0 * cycle_time;
    else
        target_speed = -50.0 + 100.0 * (cycle_time - 1.0);
    
    


    // Get the measured speed
    double measured_speed = encoder_get_speed(motor);

    // Call the speed control function
    speed_control(motor, target_speed);

    // Log the data to the CSV file
    if (speed_log != NULL)
    {
        fprintf(speed_log, "%d,%f,%f\n", t_ms, target_speed, measured_speed);
    }

    return 1; // Continues active
}

void speed_control_cleanup(void)
{
    if (speed_log != NULL)
    {
        fflush(speed_log);
        fclose(speed_log);
        speed_log = NULL;
    }
}