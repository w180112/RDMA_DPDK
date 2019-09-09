#include "rdma_client.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <rte_per_lcore.h>
#include <rte_lcore.h>

int rdma_init(conn_info_t *conn_info);

int rdma_init(conn_info_t *conn_info)
{
	struct addrinfo				*res, *t; 
	struct addrinfo				hints;
	int 						n, err;
	char						port[12];
	struct rdma_cm_event		*event;

	hints.ai_family = AF_INET;
	hints.ai_socktype  = SOCK_STREAM;

	conn_info->cm_channel = rdma_create_event_channel(); 
	if (!conn_info->cm_channel)  
		return 1; 
	err = rdma_create_id(conn_info->cm_channel, &(conn_info->cm_id), NULL, RDMA_PS_TCP);
	if (err)  
		return err;
	sprintf(port,"%d",20000);
	hints.ai_flags = 0;//AI_PASSIVE;
	n = getaddrinfo(conn_info->ip_addr, port, NULL, &(res));
	if (n < 0)  {
		fprintf(stderr, "%s\n", gai_strerror(n));
		return 1;
	}
	for (t=res; t; t=t->ai_next) {
		err = rdma_resolve_addr(conn_info->cm_id, NULL, t->ai_addr, RESOLVE_TIMEOUT_MS);
		if (!err)
			break;
	}
	err = rdma_get_cm_event(conn_info->cm_channel, &event);
	if (err)
		return err;
	if (event->event != RDMA_CM_EVENT_ADDR_RESOLVED)
		return 1;

	rdma_ack_cm_event(event);

	err = rdma_resolve_route(conn_info->cm_id, RESOLVE_TIMEOUT_MS);
	if (err)
		return err;
	freeaddrinfo(res);
	err = rdma_get_cm_event(conn_info->cm_channel, &event);
	if (err)
		return err;
	if (event->event != RDMA_CM_EVENT_ROUTE_RESOLVED)
		return 1; 

	rdma_ack_cm_event(event);
	return 0;
}