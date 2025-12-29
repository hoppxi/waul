#pragma once
#include <string>

namespace waul {

int ipc_send_command(const std::string& cmd, bool wait_response = true);
int ipc_server_init();
int ipc_server_accept(int server_fd);

void ipc_handle_client(int client_fd);

}