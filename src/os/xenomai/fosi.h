/***************************************************************************
  tag: Peter Soetens  Wed Jan 18 14:11:39 CET 2006  fosi.h 

                        fosi.h -  description
                           -------------------
    begin                : Wed January 18 2006
    copyright            : (C) 2006 Peter Soetens
    email                : peter.soetens@mech.kuleuven.be
 
 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public                   *
 *   License as published by the Free Software Foundation;                 *
 *   version 2 of the License.                                             *
 *                                                                         *
 *   As a special exception, you may use this file as part of a free       *
 *   software library without restriction.  Specifically, if other files   *
 *   instantiate templates or use macros or inline functions from this     *
 *   file, or you compile this file and link it with other files to        *
 *   produce an executable, this file does not by itself cause the         *
 *   resulting executable to be covered by the GNU General Public          *
 *   License.  This exception does not however invalidate any other        *
 *   reasons why the executable file might be covered by the GNU General   *
 *   Public License.                                                       *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public             *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/
 
 
#ifndef __FOSI_H
#define __FOSI_H

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600   // use all Posix features.
#endif

#define HAVE_FOSI_API

#ifdef __cplusplus
extern "C" {
#endif

	// Orocos Implementation (CPU specific)
#include "os/oro_atomic.h"
#include "os/oro_bitops.h"

#include "../../rtt-config.h"
#if !defined(OROBLD_OS_AGNOSTIC) || defined(OROBLD_OS_INTERNAL)

	//Xenomai headers
	//#include <linux/types.h>
	// xenomai assumes presence of u_long
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>

#include <xeno_config.h> // version number
	// From Xenomai 2.1 on, it defines LOCK_PREFIX itself, undef ours
#if !(CONFIG_XENO_VERSION_MAJOR == 2 && CONFIG_XENO_VERSION_MINOR == 0)
#undef LOCK_PREFIX 
#endif

#include <native/task.h>
#include <native/timer.h>
#include <native/mutex.h>
#include <native/sem.h>

	typedef RT_MUTEX rt_mutex_t;
	typedef RT_MUTEX rt_rec_mutex_t;
	typedef RT_SEM rt_sem_t;

	// Time Related
	// 'S' -> Signed long long
	typedef SRTIME          NANO_TIME;
	typedef SRTIME          TICK_TIME;
	typedef struct timespec TIME_SPEC;
	typedef RT_TASK         RTOS_XENO_TASK;

#else

#include <pthread.h>
	//Xenomai redefinitions.
	// Wrap/typedef Xeno types to Orocos Types.
	typedef unsigned long XENO_rt_handle_t;
	typedef struct {
		XENO_rt_handle_t handle;
	} XENO_HANDLE;
	typedef XENO_HANDLE     rt_mutex_t;
	typedef XENO_HANDLE     rt_rec_mutex_t;
	typedef XENO_HANDLE     rt_sem_t;
	typedef long long       NANO_TIME;
	typedef long long       TICK_TIME;
	typedef struct timespec TIME_SPEC;

	typedef struct {
		XENO_rt_handle_t handle;
		XENO_rt_handle_t handle2;
	} XENO_TASK_HANDLE;

	typedef XENO_TASK_HANDLE RTOS_XENO_TASK;
#endif

	// Thread/Task related.
	typedef struct {
		char * name;
		RTOS_XENO_TASK xenotask;
		RTOS_XENO_TASK* xenoptr;
	} RTOS_TASK;

#define SCHED_XENOMAI_HARD 0 /** Hard real-time */
#define SCHED_XENOMAI_SOFT 1 /** Soft real-time */
#define ORO_SCHED_RT    0 /** Hard real-time */
#define ORO_SCHED_OTHER 1 /** Soft real-time */

	// inline functions if not agnostic.
#ifndef OROBLD_OS_AGNOSTIC
	// hrt is in ticks
inline TIME_SPEC ticks2timespec(TICK_TIME hrt)
{
	TIME_SPEC timevl;
	timevl.tv_sec = rt_timer_tsc2ns(hrt) / 1000000000LL;
	timevl.tv_nsec = rt_timer_tsc2ns(hrt) % 1000000000LL;
	return timevl;
}

	// hrt is in ticks
inline TICK_TIME timespec2ticks(const TIME_SPEC* ts)
{
	return  rt_timer_ns2tsc(ts->tv_nsec + ts->tv_sec*1000000000LL);
}

// turn this on to have maximum detection of valid system calls.
#ifdef OROSEM_OS_XENO_CHECK
#define CHK_XENO_CALL() do { if(rt_task_self() == 0) { \
        printf("RTT: XENO NOT INITIALISED IN THIS THREAD pid=%d,\n\
    BUT TRIES TO INVOKE XENO FUNCTION >>%s<< ANYWAY\n", getpid(), __FUNCTION__ );\
        assert( rt_task_self() != 0 ); }\
        } while(0)
#define CHK_XENO_PTR(ptr) do { if(ptr == 0) { \
        printf("RTT: TRIED TO PASS NULL POINTER TO XENO IN THREAD pid=%d,\n\
    IN TRYING TO INVOKE XENO FUNCTION >>%s<<\n", getpid(), __FUNCTION__ );\
        assert( ptr != 0 ); }\
        } while(0)
#else
#define CHK_XENO_CALL()
#define CHK_XENO_PTR( a )
#endif
    
inline NANO_TIME rtos_get_time_ns(void) { return rt_timer_ticks2ns(rt_timer_read()); }

inline TICK_TIME rtos_get_time_ticks(void) { return rt_timer_tsc(); }

inline TICK_TIME ticksPerSec(void) { return rt_timer_ns2tsc( 1000 * 1000 * 1000 ); }

// WARNING: Orocos 'ticks' are 'Xenomai tsc' and Xenomai `ticks' are not
// used in the Orocos API. Thus Orocos uses `Xenomai tsc' and `Xenomai ns', 
// yet Xenomai requires `Xenomai ticks' at the interface
// ==> do not use nano2ticks to convert to `Xenomai ticks' because it
// converts to `Xenomai tsc'.
inline TICK_TIME nano2ticks(NANO_TIME t) { return rt_timer_ns2tsc(t); }
inline NANO_TIME ticks2nano(TICK_TIME t) { return rt_timer_tsc2ns(t); }

	inline int rtos_nanosleep(const TIME_SPEC *rqtp, TIME_SPEC *rmtp)
	{
		CHK_XENO_CALL();
		RTIME ticks = rqtp->tv_sec * 1000000000LL + rqtp->tv_nsec;
		rt_task_sleep( rt_timer_ns2ticks(ticks) );
		return 0;
	}

    static inline int rtos_sem_init(rt_sem_t* m, int value )
    {
        CHK_XENO_CALL();
		return rt_sem_create( m, 0, value, S_PRIO);
    }

    static inline int rtos_sem_destroy(rt_sem_t* m )
    {
        CHK_XENO_CALL();
        return rt_sem_delete( m );
    }

    static inline int rtos_sem_signal(rt_sem_t* m )
    {
        CHK_XENO_CALL();
        return rt_sem_v( m );
    }

    static inline int rtos_sem_wait(rt_sem_t* m )
    {
        CHK_XENO_CALL();
        return rt_sem_p( m, TM_INFINITE );
    }

    static inline int rtos_sem_trywait(rt_sem_t* m )
    {
        CHK_XENO_CALL();
        return rt_sem_p( m, TM_NONBLOCK);
    }

    static inline int rtos_sem_value(rt_sem_t* m )
    {
        CHK_XENO_CALL();
		RT_SEM_INFO sinfo;
        if (rt_sem_inquire(m, &sinfo) == 0 ) {
			return sinfo.count;
		}
		return -1;
    }

    static inline int rtos_sem_wait_timed(rt_sem_t* m, NANO_TIME delay )
    {
        CHK_XENO_CALL();
        return rt_sem_p(m, rt_timer_ns2ticks(delay) );
    }

    static inline int rtos_mutex_init(rt_mutex_t* m)
    {
        CHK_XENO_CALL();
		// a Xenomai mutex is always recursive
        return rt_mutex_create(m, 0);
    }

    static inline int rtos_mutex_destroy(rt_mutex_t* m )
    {
        CHK_XENO_CALL();
        return rt_mutex_delete(m);
    }

    static inline int rtos_mutex_rec_init(rt_mutex_t* m)
    {
        CHK_XENO_CALL();
		// a Xenomai mutex is always recursive
        return rt_mutex_create(m, 0);
    }

    static inline int rtos_mutex_rec_destroy(rt_mutex_t* m )
    {
        CHK_XENO_CALL();
        return rt_mutex_delete(m);
    }

    static inline int rtos_mutex_lock( rt_mutex_t* m)
    {
        CHK_XENO_CALL();
        return rt_mutex_lock(m, TM_INFINITE );
    }

    static inline int rtos_mutex_trylock( rt_mutex_t* m)
    {
        CHK_XENO_CALL();
        return rt_mutex_lock(m, TM_NONBLOCK);
    }

    static inline int rtos_mutex_unlock( rt_mutex_t* m)
    {
        CHK_XENO_CALL();
        return rt_mutex_unlock(m);
    }

    static inline int rtos_mutex_rec_lock( rt_rec_mutex_t* m)
    {
        CHK_XENO_CALL();
        return rt_mutex_lock(m, TM_INFINITE );
    }

    static inline int rtos_mutex_rec_trylock( rt_rec_mutex_t* m)
    {
        CHK_XENO_CALL();
        return rt_mutex_trylock(m);
    }

    static inline int rtos_mutex_rec_unlock( rt_rec_mutex_t* m)
    {
        CHK_XENO_CALL();
        return rt_mutex_unlock(m);
    }


inline
int rtos_printf(const char *fmt, ...)
{
    va_list list;
    char printkbuf [2000];
    printkbuf[0] = '\0';
    va_start (list, fmt);
    vsprintf(printkbuf, fmt, list);
    va_end (list);
    return printf(printkbuf);
}

#else  // OSBLD_OS_AGNOSTIC

/* 	typedef RT_MUTEX rt_mutex_t; */
/* 	typedef RT_SEM rt_sem_t; */
/* 	// Time Related */
	
/* 	// 'S' -> Signed long long */
/* 	typedef SRTIME NANO_TIME; */
/* 	typedef SRTIME TICK_TIME; */
/* 	typedef struct timespec TIME_SPEC; */

#endif

/**
 * Fosi Interface
 */

TIME_SPEC ticks2timespec(TICK_TIME hrt);

NANO_TIME rtos_get_time_ns(void);

TICK_TIME rtos_get_time_ticks(void);

TICK_TIME ticksPerSec(void);

TICK_TIME nano2ticks(NANO_TIME t);

NANO_TIME ticks2nano(TICK_TIME t);

int rtos_nanosleep(const TIME_SPEC *rqtp, TIME_SPEC *rmtp) ;

int rtos_mutex_init(rt_mutex_t* m);

int rtos_mutex_destroy(rt_mutex_t* m );

int rtos_mutex_rec_init(rt_mutex_t* m);

int rtos_mutex_rec_destroy(rt_mutex_t* m );

int rtos_mutex_lock( rt_mutex_t* m);

int rtos_mutex_trylock( rt_mutex_t* m);

int rtos_mutex_unlock( rt_mutex_t* m);

int rtos_mutex_rec_lock( rt_rec_mutex_t* m);

int rtos_mutex_rec_trylock( rt_rec_mutex_t* m);

int rtos_mutex_rec_unlock( rt_rec_mutex_t* m);

int rtos_printf(const char *fmt, ...);

int rtos_sem_init(rt_sem_t* m, int value );
int rtos_sem_destroy(rt_sem_t* m );
int rtos_sem_signal(rt_sem_t* m );
int rtos_sem_wait(rt_sem_t* m );
int rtos_sem_trywait(rt_sem_t* m );
int rtos_sem_value(rt_sem_t* m );
int rtos_sem_wait_timed(rt_sem_t* m, NANO_TIME delay );

#ifdef __cplusplus
}
#endif

#endif