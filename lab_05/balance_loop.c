#include <math.h>
#include <stdatomic.h>

#include "imu.h"
#include "encoder.h"
#include "pwm.h"
#include "balance_loop.h"

#define MAX_DUTY      0.6
#define PITCH_OFFSET  1.5     /* deg: raw pitch at the true balance point */
#define OUTPUT_SIGN   1.0     /* -1.0 if the wheels drive away from the lean */
#define RATE_SIGN     1.0     /* -1.0 if the gyro rate has the opposite sign of the pitch change */

/* Shared state */
_Atomic double g_pitch_deg;
_Atomic double g_motor_rpm[2];
_Atomic double g_rpm_ref;     /* holds the gyro rate (name kept for the header) */
_Atomic double g_duty[2];
_Atomic double g_kp = 0.05;   /* duty per degree */
_Atomic double g_ki = 0.0;    /* unused for now, kept for the GUI */
_Atomic double g_kd = 0.0015; /* duty per (deg/s) */

static double clampd(double v, double lim)
{
    if (v > lim)  return  lim;
    if (v < -lim) return -lim;
    return v;
}

/* Wrap any angle to [-180, 180) */
static double wrap180(double a)
{
    a = fmod(a + 180.0, 360.0);
    if (a < 0.0) a += 360.0;
    return a - 180.0;
}

void balance_loop_set_gains(double kp, double ki, double kd)
{
    atomic_store(&g_kp, kp);
    atomic_store(&g_ki, ki);
    atomic_store(&g_kd, kd);
}

void balance_loop_init(void)
{
}

void balance_loop_step(unsigned int loop_count)
{
    (void)loop_count;

    /* One I2C read gives both the Euler pitch and the gyro rate */
    double raw_pitch, rate;
    get_imu_pitch_and_rate(&raw_pitch, &rate);
    rate *= RATE_SIGN;

    /* Upright = 0, continuous through 0 */
    double pitch = wrap180(raw_pitch + 180.0 - PITCH_OFFSET);

    double kp = atomic_load(&g_kp);
    double kd = atomic_load(&g_kd);

    /* error = target - pitch, with target = 0; d(error)/dt = -rate */
    double error = -pitch;
    double u = OUTPUT_SIGN * (kp * error - kd * rate);

    /* Motor 0 (right): forward = -duty.  Motor 1 (left): forward = +duty. */
    double d0 = clampd(-u, MAX_DUTY);
    double d1 = clampd( u, MAX_DUTY);

    pwm_set_duty(0, (float)d0);
    pwm_set_duty(1, (float)d1);

    /* Publish for console / GUI */
    atomic_store(&g_pitch_deg, pitch);
    atomic_store(&g_rpm_ref, rate);
    atomic_store(&g_duty[0], d0);
    atomic_store(&g_duty[1], d1);
    atomic_store(&g_motor_rpm[0], encoder_get_speed(0));
    atomic_store(&g_motor_rpm[1], encoder_get_speed(1));
}

void balance_loop_cleanup(void)
{
    pwm_set_duty(0, 0.0f);
    pwm_set_duty(1, 0.0f);
}