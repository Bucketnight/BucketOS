// usertest.h: User test loader API (loads and runs a test program in ring 3).

/*
 * Reading guide:
 * - Purpose: usertest.h: User test loader API (loads and runs a test program in ring 3).
 * - Start reading at: usertest_load
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_USERTEST_H
#define BUCKETOS_USERTEST_H

bool usertest_load(void);

#endif
