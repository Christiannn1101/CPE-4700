/* 
* Bob Turney
* compile command
*          $ gcc -o game_controller game_controller.c
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <time.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "encoder.h"
#include "pwm.h"

#define CONTROLLER_DEVICE "/dev/input/event6"

atomic_bool running = true; 

static const char *button_name(unsigned short code)
{
    switch(code)
    {
        case BTN_A:        return "A";
        case BTN_B:        return "B";
        case BTN_X:        return "X";
        case BTN_Y:        return "Y";

        case BTN_TL:       return "LEFT_BUMPER";
        case BTN_TR:       return "RIGHT_BUMPER";

        case BTN_TL2:      return "LEFT_TRIGGER";
        case BTN_TR2:      return "RIGHT_TRIGGER";

        case BTN_SELECT:   return "SELECT";
        case BTN_START:    return "START";
        case BTN_MODE:     return "HOME";

        case BTN_THUMBL:   return "LEFT_STICK_BUTTON";
        case BTN_THUMBR:   return "RIGHT_STICK_BUTTON";

        case BTN_DPAD_UP:     return "DPAD_UP";
        case BTN_DPAD_DOWN:   return "DPAD_DOWN";
        case BTN_DPAD_LEFT:   return "DPAD_LEFT";
        case BTN_DPAD_RIGHT:  return "DPAD_RIGHT";

        default:
            return "UNKNOWN_BUTTON";
    }
}

int main(void)
{
    int fd = open(CONTROLLER_DEVICE, O_RDONLY);

    if (fd < 0)
    {
        perror("open");
        return EXIT_FAILURE;
    }

    pwm_init();
    encoder_init();

    printf("Monitoring controller: %s\n", CONTROLLER_DEVICE);
    printf("Press buttons on the controller...\n\n");


    struct input_event ev;
    int abs_values[ABS_MAX] = { 0 };

    while (1)
    {
        ssize_t bytes = read(fd, &ev, sizeof(ev));


        if (ev.type == EV_KEY)


        if (bytes < 0)
        {
            perror("read");
            break;
        }

        if (bytes != sizeof(ev))
            continue;

        if (ev.type == EV_KEY)
        {
            const char *name = button_name(ev.code);

            if (ev.value == 1){
                printf("BUTTON PRESSED  : %s (code=%u)\n",name, ev.code);
                fflush(stdout);
            }
            else if (ev.value == 0){
                printf("BUTTON RELEASED : %s (code=%u)\n",name, ev.code);
                fflush(stdout);
            }
        }

        if (ev.type == EV_ABS)
        {
            if (ev.code < ABS_MAX) {
                abs_values[ev.code] = ev.value;
            }

            int yvalue = abs_values[17];
            int xvalue = abs_values[16];

            printf("AXIS %u VALUE %d Type %u\n",ev.code, ev.value, ev.type);
            fflush(stdout);
            

            switch(yvalue){
                //BWD
                case 1:
                    if(xvalue == -1){
                    pwm_set_duty(1, -0.25);
                    pwm_set_duty(0, 0.1);
                    }

                    else if(xvalue == 1){
                    pwm_set_duty(1, -0.1);
                    pwm_set_duty(0, 0.25);
                    }

                    else{
                    pwm_set_duty(1, -0.25);
                    pwm_set_duty(0, 0.25);
                    }   
                    break;
                //FWD
                case -1:
                    if(xvalue == -1){
                    pwm_set_duty(1, 0.25);
                    pwm_set_duty(0, -0.1);
                    }
                    else if(xvalue == 1){
                    pwm_set_duty(1, 0.1);
                    pwm_set_duty(0, -0.25);
                    }
                    else{
                    pwm_set_duty(1, 0.25);
                    pwm_set_duty(0, -0.25);
                    }
                    break;
                    
                default:
                    pwm_set_duty(1, 0);
                    pwm_set_duty(0, 0); 
                    break;
            }
            




        }
        
    }


    

    close(fd);
    return EXIT_SUCCESS;
}