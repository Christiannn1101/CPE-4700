#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define BNO055_ADDR 0x28

int write_reg(int fd, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return write(fd, buf, 2);
}

int read16(int fd, uint8_t reg, int16_t *out){
    if (write(fd, &reg, 1) != 1)
        return -1;
    
    uint8_t data[2];
    if (read(fd, data, 2) != 2)
        return -1;
        
    *out = (data[1] << 8) | data[0];
    return 0;
}

int main(void)
{
    int fd = open("/dev/i2c-7", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, BNO055_ADDR) < 0) {
        perror("ioctl");
        return 1;
    }

    write_reg(fd, 0x07, 0x00); // Page 0
    write_reg(fd, 0x3D, 0x0C); // NDOF fusion mode
    usleep(20000);

    while (1) {
        int16_t raw_h, raw_r, raw_p;
        
        read16(fd, 0x1A, &raw_h);
        read16(fd, 0x1C, &raw_r);
        read16(fd, 0x1E, &raw_p);

        float heading = raw_h/16.0f;
        float roll = raw_r/16.0f;
        float pitch = raw_p/16.0f;

        printf("Heading: %8.2f deg, Roll: %8.2f deg, Pitch: %8.2f deg\n", heading, roll, pitch);
        usleep(200000);
    }

    close(fd);
    return 0;
}
