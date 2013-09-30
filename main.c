
#include <string.h>

#include "eth.h"

int main(int argc, char *argv[]) {
    eth_access_t eth_access;
    
    eth_access.net_addr = "192.168.1.0";
    
    eth_init(&eth_access);
    eth_scan(&eth_access);    
    
    return 0;
}
