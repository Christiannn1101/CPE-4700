/*
 * Bob Turney
 * compile command
 *          $ gcc -o imu_test1 imu_test1.c bno055.c
 */

#include <stdio.h>
#include <unistd.h>

#include "bno055.h"

static int fd;

int imu_init(void)
{

    fd = bno055_open("/dev/i2c-7");

    if (fd < 0)
    {
        perror("BNO055 open");
        return 1;
    }

    if (bno055_init(fd))
    {
        fprintf(stderr,
                "Failed to initialize BNO055\n");
        return 1;
    }

    printf("BNO055 initialized\n");
    return 0;
}

double get_imu_roll()
{
    bno055_euler_t e;

    if(!bno055_read_euler(fd, &e))
    {
        return e.roll;
    }
    fprintf(stderr, "Failed to read IMU data\n");
    return 0.0; // Return a default value or handle the error as needed
}

double get_imu_pitch()
{
    bno055_euler_t e;

    if(!bno055_read_euler(fd, &e))
    {
        return e.pitch;
    }
    fprintf(stderr, "Failed to read IMU data\n");
    return 0.0; // Return a default value or handle the error as needed
}

double get_imu_heading()
{
    bno055_euler_t e;

    if(!bno055_read_euler(fd, &e))
    {
        return e.heading;
    }
    fprintf(stderr, "Failed to read IMU data\n");
    return 0.0; // Return a default value or handle the error as needed
}


void get_imu_pitch_and_rate(double *pitch_deg, double *rate_dps)
{
    static double last_p = 0.0, last_r = 0.0;
    bno055_euler_t e;
    float g[3];

    if (bno055_read_gyro_euler(fd, g, &e) == 0) {
        last_p = e.pitch;
        last_r = g[0];          /* X axis = pitch rate; check the sign (below) */
    }
    *pitch_deg = last_p;
    *rate_dps  = last_r;
}

void imu_cleanup(void)
{
    bno055_close(fd);
}