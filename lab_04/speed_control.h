#ifndef SPEED_CONTROL_H
#define SPEED_CONTROL_H

void speed_control_init(void);
void speed_control(int motor, double target_speed);
int speed_control_run(unsigned int loop_count); 
void speed_control_cleanup(void);

#endif