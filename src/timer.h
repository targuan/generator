/* 
 * File:   timer.h
 * Author: cqdc5422
 *
 * Created on 31 mai 2013, 16:06
 */

#ifndef TIMER_H
#define	TIMER_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <sys/time.h> 
#include "options.h"
    void set_ts(struct timeval* ts, int num,struct options *opts);


#ifdef	__cplusplus
}
#endif

#endif	/* TIMER_H */

