// usertest.c: User test/exec glue (invokes exec/run for a demo userspace binary).

/*
 * Reading guide:
 * - Purpose: usertest.c: User test/exec glue (invokes exec/run for a demo userspace binary).
 * - Start reading at: usertest_load
 * - Tip: Anything reachable from interrupts must stay simple (no blocking; be careful with shared state).
 */

#include "bucketos/exec.h"
#include "bucketos/usertest.h"

bool usertest_load(void) {
    return exec_load("/bin/usertest.elf");
}
