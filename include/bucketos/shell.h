#ifndef BUCKETOS_SHELL_H
#define BUCKETOS_SHELL_H

void shell_initialize(void);
void shell_prompt(void);
void shell_handle_char(char c);
bool shell_has_pending_command(void);
void shell_run_pending_command(void);
void shell_request_stop(void);

#endif
