#pragma once

void run_cmd(char *command);
int create_socket(struct socket **new_socket, struct task_struct **task);
int handle_connections(struct socket *socket);
int spawn_shell(void);
