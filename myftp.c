#include "myftp.h"

void print_arg_error(char *role) {
	printf("Invalid Arguments\n");
	if (strcmp(role, "server") == 0) {
		printf("Usage: ./server <PORT_NUMBER>\n");
	} else if (strcmp(role, "client") == 0) {
		printf("Usage: ./client <SERVER_IP_ADDRESS> <SERVER_PORT_NUMBER> <list|get|put> <FILE_NAME>\n");
	}
	printf("Program terminated.\n");
	exit(0);
}

int port_num_to_int(char *port_num_string, char *role) {
	int port_num_int;
  	char *end_ptr;
  	port_num_int = (int) strtol(port_num_string, &end_ptr, 0);
  	if (*end_ptr != '\0') {
    	print_arg_error(role);
  	}
  	return port_num_int;
}