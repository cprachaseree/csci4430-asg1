#include "myftp.h";

int check_port_num(int arg_num, char *port_number_string);

int main(int argc, char *argv[]) {
  int PORT_NUMBER = check_port_num(argc, argv[1]);
}

int check_port_num(int arg_num, char *port_number_string) {
  int port_number_int;
  char *end_ptr;
  if (arg_num != 2) {
    printf("Invalid arguments. Usage ./server <PORT_NUMBER>\n");
    exit(0);
  }
  port_number_int = (int) strtol(port_number_string, &end_ptr, 0);
  if (*end_ptr != '\0') {
    printf("Invalid argument. Usage ./server <PORT_NUMBER>\n");
    exit(0);
  }
  return port_number_int;
}
