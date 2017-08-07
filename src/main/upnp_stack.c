#include "upnp_stack.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/igmp.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"

#include <string.h>

#define SSDP_MULTICAST_ADDRESS "239.255.255.250"
#define SSDP_MULTICAST_PORT 1900

#define UPNP_MULTICAST_ADDRESS SSDP_MULTICAST_ADDRESS

//below is a chunked http message structure. IMPORTANT, the end chunk is 0\r\n\r\n and all sizes are in hex
#define TCP_MESSAGE "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\n\r\n5C\r\n<!DOCTYPE html><html><body><h1>My First Heading</h1><p>My first paragraph.</p></body></html>\r\n"
#define TCP_MESSAGE_CHUNK_1 "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\n\r\n34\r\n<!DOCTYPE html><html><body><h1>My First Heading</h1>\r\n"
#define TCP_MESSAGE_CHUNK_2 "28\r\n<p>My first paragraph.</p></body></html>\r\n"
#define HTTP_CONTINUE "HTTP/1.1 100 Continue\r\n\r\n"
#define HTTP_END "0\r\n\r\n"

//UDP socket variables
static struct udp_pcb* pcb;
static ip_addr_t upnp_discovery_address;
static struct pbuf packet_buffer;

//TCP socket variables
static struct tcp_pcb* http_pcb;
static ip_addr_t http_address;
static struct pbuff http_packet_buffer;
static uint8_t tcp_buffer[256];
static uint8_t tcp_buffer2[256];

void UpnpStack_RxCallback(void* arg, struct udp_pcb* upcb, struct pbuf* p, const ip_addr_t* remote_addr, u16_t port){
	//printf("UPnP RX Callback\r\n");
	//printf((char*)p->payload);
	//is this a discovery packet?
	if(memcmp(p->payload, "M-SEARCH", sizeof("M-SEARCH")-1) == 0){
		printf("GOT M-SEARCH PACKET\r\n");
	}
	else if(memcmp(p->payload, "NOTIFY", sizeof("NOTIFY")-1) == 0){
		printf("GOT NOTIFY PACKET\r\n");
	}
	else if(memcmp(p->payload, "HTTP/1.1 200 OK", sizeof("HTTP/1.1 200 OK")-1) == 0){
		printf("GOT HTTP OK PACKET\r\n");
	}
	else
		printf("Unknown UPnP start line!\r\n");

	pbuf_free(p);
}

err_t sent_callback(void* arg, struct tcp_pcb* tpcb, u16_t len){
	
	memcpy(&tcp_buffer, TCP_MESSAGE_CHUNK_2, sizeof(TCP_MESSAGE_CHUNK_2));
	tcp_write(tpcb, &tcp_buffer, sizeof(TCP_MESSAGE_CHUNK_2)-1, 0);

	memcpy(&tcp_buffer2, HTTP_END, sizeof(HTTP_END));
	tcp_write(tpcb, &tcp_buffer2, sizeof(HTTP_END)-1, 0);

	return ERR_OK;
}

err_t UpnpStack_TcpRxCallback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err){
	printf("TCP rx callback\r\n");

	//memcpy(tcp_buffer, TCP_MESSAGE, sizeof(TCP_MESSAGE));
	//tcp_write(tpcb, tcp_buffer, sizeof(TCP_MESSAGE)-1, 0);

	memcpy(&tcp_buffer, TCP_MESSAGE_CHUNK_1, sizeof(TCP_MESSAGE_CHUNK_1));
	tcp_write(tpcb, &tcp_buffer, sizeof(TCP_MESSAGE_CHUNK_1)-1, 0);

	//memcpy(&tcp_buffer2, HTTP_END, sizeof(HTTP_END));
	//tcp_write(tpcb, &tcp_buffer2, sizeof(HTTP_END)-1, 0);
	
	//p can be null if this is a FIN from client!
	if(p){
		printf("Tcp message: %s", (char*)p->payload);
		tcp_recved(tpcb, p->len);
	}else{
		tcp_close(tpcb);
	}

	pbuf_free(p);

	return ERR_OK;
}

err_t UpnpStack_TcpAcceptCallback(void *arg, struct tcp_pcb *newpcb, err_t err){
	printf("TCP accept callback\r\n");

	//accept the connection on the listener pcb
	tcp_accepted(http_pcb);
	
	//setup rcv callback on the NEW pcb
	tcp_recv(newpcb, UpnpStack_TcpRxCallback);
	//setup sent callback on the NEW pcb
	tcp_sent(newpcb, sent_callback);

	return ERR_OK;
}

bool UpnpStack_Create(void){
	//we need to create udp socket to handle multicast device discoveries
	pcb = udp_new();

	upnp_discovery_address.u_addr.ip4.addr = inet_addr(UPNP_MULTICAST_ADDRESS);

    udp_bind(pcb, &upnp_discovery_address, SSDP_MULTICAST_PORT);

    udp_recv(pcb, UpnpStack_RxCallback, NULL);

    igmp_joingroup(NULL, &upnp_discovery_address.u_addr.ip4);

    //create http web socket on port 80 to handle unicast traffic
   	http_pcb = tcp_new();
   	http_address.u_addr.ip4.addr = htonl(INADDR_ANY);
   	tcp_bind(http_pcb, &http_address, 80);
   	http_pcb = tcp_listen(http_pcb);
   	tcp_accept(http_pcb, UpnpStack_TcpAcceptCallback);
   	tcp_recv(http_pcb, UpnpStack_TcpRxCallback);

	return true;
}

bool UpnpStack_Destroy(void){

	return true;
}