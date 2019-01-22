#include <string>
#include "mbed.h"
#include "easy-connect.h"
#include "nsapi_dns.h"

// Network interface
NetworkInterface *net;

// Socket demo
int main() {
    int remaining;
    int rcount;
    char *p;
    char *buffer = new char[256];
    nsapi_size_or_error_t r;
    
    (*((volatile uint32_t *)(0x46010200))) = 2; // setSn_TXBUF_SIZE(0,2); Socket 0 TX Buffer Size 2KB
	(*((volatile uint32_t *)(0x46010220))) = 2; // setSn_RXBUF_SIZE(0,2); Socket 0 RX Buffer Size 2KB
	(*((volatile uint32_t *)(0x46010201))) = 2; // setSn_TXBUF_SIZE(1,2); Socket 1 TX Buffer Size 2KB
	(*((volatile uint32_t *)(0x46010221))) = 2; // setSn_RXBUF_SIZE(1,2); Socket 1 RX Buffer Size 2KB
	(*((volatile uint32_t *)(0x46010202))) = 2; // setSn_TXBUF_SIZE(2,2); Socket 2 TX Buffer Size 2KB
	(*((volatile uint32_t *)(0x46010222))) = 2; // setSn_RXBUF_SIZE(2,2); Socket 2 RX Buffer Size 2KB
	(*((volatile uint32_t *)(0x46010203))) = 2; // setSn_TXBUF_SIZE(3,2); Socket 3 TX Buffer Size 2KB
	(*((volatile uint32_t *)(0x46010223))) = 2; // setSn_RXBUF_SIZE(3,2); Socket 3 RX Buffer Size 2KB
	(*((volatile uint32_t *)(0x46010204))) = 0; // setSn_TXBUF_SIZE(4,0); Socket 4 TX Buffer Size 0KB
	(*((volatile uint32_t *)(0x46010224))) = 0; // setSn_RXBUF_SIZE(4,0); Socket 4 RX Buffer Size 0KB
	(*((volatile uint32_t *)(0x46010205))) = 0; // setSn_TXBUF_SIZE(5,0); Socket 5 TX Buffer Size 0KB
	(*((volatile uint32_t *)(0x46010225))) = 0; // setSn_RXBUF_SIZE(5,0); Socket 5 RX Buffer Size 0KB
	(*((volatile uint32_t *)(0x46010206))) = 0; // setSn_TXBUF_SIZE(6,0); Socket 6 TX Buffer Size 0KB
	(*((volatile uint32_t *)(0x46010226))) = 0; // setSn_RXBUF_SIZE(6,0); Socket 6 RX Buffer Size 0KB
	//for SRAM... you can use 0x46EE000~0x461F1FFF (16KB) AREA.
    (*((volatile uint32_t *)(0x46010207))) = 8; // setSn_TXBUF_SIZE(7,8); Socket 7 TX Buffer Size 8KB -> use as SRAM 0x461EE000~0x461EFFFF
	(*((volatile uint32_t *)(0x46010227))) = 8; // setSn_RXBUF_SIZE(7,8); Socket 7 RX Buffer Size 8KB -> use as SRAM 0x461F0000~0x461F1FFF
	
    // Bring up the ethernet interface
    printf("Mbed OS Socket example\n");

#ifdef MBED_MAJOR_VERSION
    printf("Mbed OS version: %d.%d.%d\n\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
#endif

//    net = NetworkInterface::get_default_instance();

    printf("Easy connect...\n");
    net = easy_connect(true);
    if (!net) {
        printf("Cannot connect to the network, see serial output");
        return 1;
    }
    printf("Connected to the network. Opening a socket...\n");
 
    if (!net) {
        printf("Error! No network inteface found.\n");
        return 0;
    }

    r = net->connect();
    if (r != 0) {
        printf("Error! net->connect() returned: %d\n", r);
    }

    // Show the network address
    const char *ip = net->get_ip_address();
    const char *netmask = net->get_netmask();
    const char *gateway = net->get_gateway();
    printf("IP address: %s\n", ip ? ip : "None");
    printf("Netmask: %s\n", netmask ? netmask : "None");
    printf("Gateway: %s\n", gateway ? gateway : "None");

    // Open a socket on the network interface, and create a TCP connection to mbed.org
    TCPSocket socket;
    r = socket.open(net);
    if (r != 0) {
        printf("Error! socket.open() returned: %d\n", r);
    }
    
    r = socket.connect("api.ipify.org", 80);
    //r = socket.connect("23.23.114.123", 80);
    //r = socket.connect("192.168.0.209", 5000);
    
    if (r != 0) {
        printf("Error! socket.connect() returned: %d\n", r);
    }

    // Send a simple http request
    char sbuffer[] = "GET / HTTP/1.1\r\nHost: api.ipify.org\r\nConnection: close\r\n\r\n";
    nsapi_size_t size = strlen(sbuffer);

    // Loop until whole request send
    while(size) {
        r = socket.send(sbuffer+r, size);
        if (r < 0) {
            printf("Error! socket.connect() returned: %d\n", r);
            goto disconnect;
        }
        size -= r;
        printf("sent %d [%.*s]\n", r, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);
    }

    // Receieve an HTTP response and print out the response line
    remaining = 256;
    rcount = 0;
    p = buffer;

    r = socket.recv(p, remaining);
    /*
    // modified source..a bit strnage...
    while (0 < (r = socket.recv(p, remaining))) {
        p += r;
        rcount += r;
        remaining -= r;
    }
    */
    if (r < 0) {
        printf("Error! socket.recv() returned: %d\n", r);
        goto disconnect;
    }

    p += r;
    rcount += r;
    remaining -= r;

    printf("recv %d [%.*s]\n", rcount, strstr(buffer, "\r\n")-buffer, buffer);

    // The api.ipify.org service also gives us the device's external IP address
    p = strstr(buffer, "\r\n\r\n")+4;
    printf("External IP address: %.*s\n", rcount-(p-buffer), p);
    delete[] buffer;

disconnect:
    // Close the socket to return its memory and bring down the network interface
    socket.close();

    // Bring down the ethernet interface
    net->disconnect();
    printf("Done\n");
}

