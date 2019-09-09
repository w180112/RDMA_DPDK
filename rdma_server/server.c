#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#include <infiniband/arch.h>
#include <rdma/rdma_cma.h> 
#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_cycles.h>

#include "rdma_server.h"

extern int poll_cq(conn_info_t *conn_info);
extern void send_msg(conn_info_t *conn_info, int ibv_send_opcode);
extern int reg_mr(conn_info_t *conn_info);
extern int rdma_init(conn_info_t *conn_info);
extern int ibv_init(conn_info_t *conn_info);
extern void recv_msg(conn_info_t *conn_info);

enum { 
    RESOLVE_TIMEOUT_MS = 5000,
};

int main(int argc, char *argv[])
{
	int ret;
	conn_info_t conn_info;
	struct rdma_cm_event        *event; 
    int                     err;
    struct rdma_conn_param      conn_param = { };
    uint64_t start_cycles, end_cycles;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");
    if (rdma_init(&conn_info) > 0)
    	return 1;
    while(1) {
    	start_cycles = rte_get_timer_cycles();
    	conn_info.fp = NULL;
    	err = rdma_get_cm_event(conn_info.cm_channel, &event);
    	if (err)
        	return err;
    	if (event->event != RDMA_CM_EVENT_CONNECT_REQUEST && event->event != 10)
        	return 1;
    	conn_info.cm_id = event->id;
    	rdma_ack_cm_event(event);
    	if (ibv_init(&conn_info) > 0)
    		return 1;
    	if (reg_mr(&conn_info) > 0)
    		return 1;
		
		memset(&conn_param,0,sizeof(conn_param));
    	conn_param.responder_resources = 1;  

		conn_info.cm_id->qp = conn_info.qp;
    	if (rte_eal_remote_launch(poll_cq,&conn_info,1) < 0)
    		perror("rte eal launch failed");
		conn_info.buf_info.buf_info_msg.buf_info_data.info.buf_info_addr = conn_info.file_buf.file_buf;
		conn_info.buf_info.buf_info_msg.buf_info_data.info.buf_info_rkey = conn_info.file_buf.mr->rkey;
		recv_msg(&conn_info);
   		err = rdma_accept(conn_info.cm_id, &conn_param); 
    	if (err) 
        	return 1;
    	err = rdma_get_cm_event(conn_info.cm_channel, &event);
    	if (err) 
        	return err;
    	if (event->event != RDMA_CM_EVENT_ESTABLISHED)
        	return 1;
    	rdma_ack_cm_event(event);
    	send_msg(&conn_info,SEND_BUF_ADDR);
		rte_eal_wait_lcore(1);
		err = rdma_get_cm_event(conn_info.cm_channel, &event);
    	if (err)
        	return err;
		rdma_ack_cm_event(event);
		ibv_destroy_qp(conn_info.qp);

		ibv_dereg_mr(conn_info.file_buf.mr);
		free(conn_info.file_buf.file_buf);
		ibv_dereg_mr(conn_info.buf_info.buf_info_mr);

		ibv_destroy_cq(conn_info.cq);
		err = ibv_destroy_comp_channel(conn_info.comp_chan);
		if (err) {
			perror("ibv_destroy_comp_channel");
            return err;
		}
		err = ibv_dealloc_pd(conn_info.pd);
		if (err) {
			perror("ibv_dealloc_pd");
            return err;
		}
  		err = rdma_destroy_id(conn_info.cm_id);
		if (err != 0)
			perror("destroy fail");
		fclose(conn_info.fp);
		end_cycles = rte_get_timer_cycles();
		//printf("Cost %lu secs on transmission\n", (end_cycles - start_cycles) / rte_get_timer_hz());
	}
	rdma_destroy_id(conn_info.listen_id);
	rdma_destroy_event_channel(conn_info.cm_channel);
	return 0;
}
