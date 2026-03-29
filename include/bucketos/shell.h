// shell.h: Kernel shell API (prompt + input handling).

/*
 * Reading guide:
 * - Purpose: shell.h: Kernel shell API (prompt + input handling).
 * - Start reading at: shell_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_SHELL_H
#define BUCKETOS_SHELL_H

void shell_initialize(void);
void shell_prompt(void);
void shell_handle_char(char c);
bool shell_has_pending_command(void);
void shell_run_pending_command(void);
void shell_request_stop(void);

#endif
