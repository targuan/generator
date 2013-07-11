#include "timer.h"


void set_ts(struct timeval* ts, int num,struct  options* opts) {
    struct timeval adder = {0,1000};
    struct timeval old = {ts->tv_sec,ts->tv_usec};
    
    
    
    timeradd(&old,&adder,ts);
    
    
}
