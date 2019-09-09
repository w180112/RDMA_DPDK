#include "rdma_server.h"

int rdma_init(conn_info_t *conn_info);

int rdma_init(conn_info_t *conn_info)
{
	struct sockaddr_in      sin;
	int                     err;

	conn_info->cm_channel = rdma_create_event_channel();
    if (!conn_info->cm_channel) 
        return 1;
    err = rdma_create_id(conn_info->cm_channel,&(conn_info->listen_id),NULL,RDMA_PS_TCP); 
    if (err) 
        return 1;

    sin.sin_family = AF_INET; 
    sin.sin_port = htons(20000);
    sin.sin_addr.s_addr = INADDR_ANY;

    err = rdma_bind_addr(conn_info->listen_id,(struct sockaddr *)&sin);
    if (err) 
        return 1;
    err = rdma_listen(conn_info->listen_id,1);
    if (err)
        return 1;
    return 0;
}