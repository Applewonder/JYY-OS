#ifndef TEST
#ifndef COMMON_H
#define COMMON_H

#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

#define SPIN_LOCK_INIT 0

// static void spin_lock(int *lk) {
//   while (1) {
//     int value = atomic_xchg(lk, 1);
//     if (value == 0) {
//       break;
//     }
//   }
// }

// static void spin_unlock(int *lk) {
//   atomic_xchg(lk, 0);
// }

#endif

#else

#include_next <test/common.h>

#endif

