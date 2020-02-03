#include "myftp.h"

void check_arg(int argc);
int check_port_num(int arg_num, char *port_number_string);

int main(int argc, char *argv[]) {
    check_arg(argc);
    int PORT_NUMBER = port_num_to_int(argv[1], "server");
	int sd = socket(AF_INET, SOCK_STREAM, 0)
	if (fd == -1) {
	// fail to open socket
	return -1
	}
	
}

void check_arg(int argc) {
    if (argc != 2) {
    print_arg_error("server");
  }
}
