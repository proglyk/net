#ifndef USERLIB_H
#define USERLIB_H

#include "FreeRTOS.h"
#include "task.h"

typedef enum {
  PRIO_IDLE          = -3,          ///< priority: idle (lowest)
  PRIO_LOW           = -2,          ///< priority: low
  PRIO_LOWNORM       = -1,          ///< priority: below normal
  PRIO_NORM          =  0,          ///< priority: normal (default)
  PRIO_ABNORM        = +1,          ///< priority: above normal
  PRIO_HIGH          = +2,          ///< priority: high
  PRIO_RT            = +3,          ///< priority: realtime (highest)
  PRIO_ERROR         =  0x84        ///< system cannot determine priority or thread has illegal priority
} prio_t;

#define uxGetPriority(PRIO) (PRIO != PRIO_ERROR) ?                                 \
  ((tskIDLE_PRIORITY + (PRIO - PRIO_IDLE))) : (tskIDLE_PRIORITY)

#endif //USERLIB_H