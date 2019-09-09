#include "rdma_server.h"
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_memcpy.h>

int poll_cq(conn_info_t *conn_info);
void send_msg(conn_info_t *conn_info, int ibv_send_opcode);
void recv_msg(conn_info_t *conn_info);

int poll_cq(conn_info_t *conn_info)
{
 	struct ibv_cq		*evt_cq;
   	struct ibv_wc		wc; 
   	int 				ret;
   	int 				end_loop = 0;

	for(;;) {
		if (ibv_get_cq_event(conn_info->comp_chan,&evt_cq, &(conn_info->cq_context))) {
			printf("get cq event failed\n");
			return 1;
		}
		ibv_ack_cq_events(conn_info->cq, 1);
		if (ibv_req_notify_cq(conn_info->cq, 0))
			return 1;
		do {
			ret = ibv_poll_cq(conn_info->cq, 1, &wc);
			if (ret < 0) {
				printf("poll cq < 0\n");
				return -1;
			}
			if (ret == 0)
				continue;
			if (wc.status != IBV_WC_SUCCESS){
				printf("wc not success\n");
				return 1;
			}
			if (wc.opcode == IBV_WC_RECV_RDMA_WITH_IMM) {
				uint32_t recv_size = ntohl(wc.imm_data);
				if (conn_info->fp == NULL) {
					recv_size = (recv_size > 256) ? 256 : recv_size;
					rte_memcpy(conn_info->filename,conn_info->file_buf.file_buf,recv_size);
					conn_info->fp = fopen(conn_info->filename,"wb+");
					recv_msg(conn_info);
					send_msg(conn_info,SEND_READY_MSG);
				}
				else if (recv_size == 0) {
					send_msg(conn_info,SEND_COMPLETE_MSG);
					end_loop = 1;
				}
				else {
					fwrite(conn_info->file_buf.file_buf,recv_size,1,conn_info->fp);
					recv_msg(conn_info);
					send_msg(conn_info,SEND_READY_MSG);
				}
			}
		}while(ret);
		if (end_loop == 1)
			break;
	}
	return 0;
}

void send_msg(conn_info_t *conn_info, int ibv_send_opcode)
{
	struct ibv_send_wr 	wr, *bad_wr = NULL;
	struct ibv_sge		sge;

	memset(&wr,0,sizeof(wr));
	switch(ibv_send_opcode)
	{
		case SEND_BUF_ADDR:
			conn_info->buf_info.buf_info_msg.opcode = SEND_BUF_ADDR;
			conn_info->buf_info.buf_info_msg.lcore_id = rte_lcore_id();
			wr.wr_id = ibv_send_opcode;
			wr.opcode = IBV_WR_SEND;
			wr.sg_list = &sge;
			wr.num_sge = 1;
			wr.send_flags = IBV_SEND_SIGNALED;

			sge.addr = (uintptr_t)&(conn_info->buf_info.buf_info_msg);
			sge.length = sizeof(conn_info->buf_info.buf_info_msg);
			sge.lkey = conn_info->buf_info.buf_info_mr->lkey;
			if (ibv_post_send(conn_info->qp, &wr, &bad_wr)){
    			exit(0);
    		}
			break;
		case SEND_READY_MSG:
			conn_info->buf_info.buf_info_msg.opcode = SEND_READY_MSG;
			conn_info->buf_info.buf_info_msg.lcore_id = rte_lcore_id();
			wr.wr_id = ibv_send_opcode;
			wr.opcode = IBV_WR_SEND;
			wr.sg_list = &sge;
			wr.num_sge = 1;
			wr.send_flags = IBV_SEND_SIGNALED;

			sge.addr = (uintptr_t)&(conn_info->buf_info.buf_info_msg);
			sge.length = sizeof(conn_info->buf_info.buf_info_msg);
			sge.lkey = conn_info->buf_info.buf_info_mr->lkey;
			if (ibv_post_send(conn_info->qp, &wr, &bad_wr)){
    			exit(0);
    		}
			break;
		case SEND_COMPLETE_MSG:
			conn_info->buf_info.buf_info_msg.opcode = SEND_COMPLETE_MSG;
			conn_info->buf_info.buf_info_msg.lcore_id = rte_lcore_id();
			wr.wr_id = ibv_send_opcode;
			wr.opcode = IBV_WR_SEND;
			wr.sg_list = &sge;
			wr.num_sge = 1;
			wr.send_flags = IBV_SEND_SIGNALED;

			sge.addr = (uintptr_t)&(conn_info->buf_info.buf_info_msg);
			sge.length = sizeof(conn_info->buf_info.buf_info_msg);
			sge.lkey = conn_info->buf_info.buf_info_mr->lkey;
			if (ibv_post_send(conn_info->qp, &wr, &bad_wr)){
    			exit(0);
    		}
			break;
		default:
			;
	}
}

void recv_msg(conn_info_t *conn_info)
{
	struct ibv_recv_wr wr, *bad_wr = NULL;

	memset(&wr,0,sizeof(wr));

	wr.wr_id = IBV_WC_RECV_RDMA_WITH_IMM;
	wr.sg_list = NULL;
	wr.num_sge = 0;

	if (ibv_post_recv(conn_info->qp,&wr,&bad_wr)) {
        exit(0);
    }
}