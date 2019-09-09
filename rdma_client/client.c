#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/queue.h>

#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_cycles.h>

#include "rdma_client.h"

extern int poll_cq(conn_info_t *conn_info);
extern void send_msg(conn_info_t *conn_info, int ibv_send_opcode);
extern void recv_msg(conn_info_t *conn_info, int ibv_recv_opcode);
extern int reg_mr(conn_info_t *conn_info);
extern int rdma_init(conn_info_t *conn_info);
extern int ibv_init(conn_info_t *conn_info);

int main(int argc, char *argv[])
{
	int 						ret;
	conn_info_t 				conn_info;
	struct rdma_cm_event		*event;
	int								err;
	struct rdma_conn_param			conn_param = { };
	uint64_t start_cycles, end_cycles;

	memcpy(conn_info.ip_addr,argv[5],strlen(argv[5]));
	conn_info.ip_addr[strlen(argv[5])] = '\0';

	conn_info.fp = fopen(argv[6],"rb");
	rte_memcpy(conn_info.filename,argv[6],strlen(argv[6]));
	conn_info.filename[strlen(argv[6])] = '\0';
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");
	if (rdma_init(&conn_info) > 0)
		return 1;
	start_cycles = rte_get_timer_cycles();
	if (ibv_init(&conn_info) > 0)
		return 1;
	if (reg_mr(&conn_info) > 0)
		return 1;
	conn_param.initiator_depth = conn_param.responder_resources = 1;
	conn_param.retry_count     = 7;
	rte_eal_remote_launch(poll_cq,&conn_info,1);
	conn_info.cm_id->qp = conn_info.qp;
	recv_msg(&conn_info,RECV_BUF_ADDR);
	err = rdma_connect(conn_info.cm_id, &conn_param);
	if (err) {
		perror("connect error");
		return err;
	}

	err = rdma_get_cm_event(conn_info.cm_channel,&event);
	if (err)
		return err;
	if (event->event != RDMA_CM_EVENT_ESTABLISHED)
		return 1;

	rdma_ack_cm_event(event);

	//rte_eal_wait_lcore(1);
	err = rdma_get_cm_event(conn_info.cm_channel, &event);
	if (err)
		return err;
	rdma_ack_cm_event(event);
	ibv_destroy_qp(conn_info.qp);
	if (ibv_dereg_mr(conn_info.file_buf.mr)) {
		perror("ibv_dereg_mr");
		return -1;
	}
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
	fclose(conn_info.fp);
	end_cycles = rte_get_timer_cycles();
	printf("Cost %lu secs on transmission\n", (end_cycles - start_cycles) / rte_get_timer_hz());
	err = rdma_destroy_id(conn_info.cm_id);
	if (err)  {
		perror("rdma_destroy_id");
		return err;
	}
	rdma_destroy_event_channel(conn_info.cm_channel);
	return 0;
}
