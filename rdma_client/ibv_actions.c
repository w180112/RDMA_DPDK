#include "rdma_client.h"
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_memcpy.h>

int poll_cq(conn_info_t *conn_info);
void send_msg(conn_info_t *conn_info, int ibv_send_opcode);
void recv_msg(conn_info_t *conn_info, int ibv_recv_opcode);

ssize_t read_size;

int poll_cq(conn_info_t *conn_info)
{
 	struct ibv_cq	*evt_cq;
   	struct ibv_wc	wc; 
   	int 			ret;

	for(;;) {
		if (ibv_get_cq_event(conn_info->comp_chan,&evt_cq, &(conn_info->cq_context)))
			return 1;
		ibv_ack_cq_events(conn_info->cq, 1);
		if (ibv_req_notify_cq(conn_info->cq, 0))
			return 1;
		do {
			ret = ibv_poll_cq(conn_info->cq, 1, &wc);
			if (ret < 0)
				return -1;
			if (ret == 0)
				continue;
			if (wc.status != IBV_WC_SUCCESS)
				return 1;
			if (wc.opcode == IBV_WC_RECV) {
				if (wc.wr_id == RECV_BUF_ADDR) {
					recv_msg(conn_info,RECV_READY_MSG);
					strcpy(conn_info->file_buf.file_buf,conn_info->filename);
					send_msg(conn_info,SEND_FILE_NAME);
				}
				else if (conn_info->buf_info.buf_info_msg.opcode == RECV_READY_MSG) {
					read_size = fread(conn_info->file_buf.file_buf,1,10485760,conn_info->fp);
					send_msg(conn_info,SEND_FILE_CONTENT);
					recv_msg(conn_info,RECV_READY_MSG);
				}
				else if (conn_info->buf_info.buf_info_msg.opcode == RECV_COMPLETE_MSG) {
					rdma_disconnect(conn_info->cm_id);
				}
			}
		}while(ret);
	}
	return 0;
}

void recv_msg(conn_info_t *conn_info, int ibv_recv_opcode)
{
	struct ibv_recv_wr 	wr, *bad_wr = NULL;
	struct ibv_sge			sge;

	memset(&wr,0,sizeof(wr));
	switch(ibv_recv_opcode)
	{
		case RECV_BUF_ADDR:
			wr.wr_id = ibv_recv_opcode;
			wr.sg_list = &sge;
			wr.num_sge = 1;

			sge.addr = (uintptr_t)&(conn_info->buf_info.buf_info_msg);
			sge.length = sizeof(conn_info->buf_info.buf_info_msg);
			sge.lkey = conn_info->buf_info.buf_info_mr->lkey;
			if (ibv_post_recv(conn_info->qp, &wr, &bad_wr)){
    			exit(0);
    		}
			break;
		case RECV_READY_MSG:
			wr.wr_id = ibv_recv_opcode;
			wr.sg_list = &sge;
			wr.num_sge = 1;

			sge.addr = (uintptr_t)&(conn_info->buf_info.buf_info_msg);
			sge.length = sizeof(conn_info->buf_info.buf_info_msg);
			sge.lkey = conn_info->buf_info.buf_info_mr->lkey;
			if (ibv_post_recv(conn_info->qp, &wr, &bad_wr)){
    			exit(0);
    		}
			break;
		case RECV_COMPLETE_MSG:
			break;
		default:
			;
	}
}

void send_msg(conn_info_t *conn_info, int ibv_send_opcode)
{
	struct ibv_send_wr 	wr, *bad_wr = NULL;
	struct ibv_sge			sge;

	memset(&wr,0,sizeof(wr));
	switch(ibv_send_opcode)
	{
		case SEND_FILE_NAME:
			wr.wr_id = ibv_send_opcode;
			wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
			wr.sg_list = &sge;
			wr.imm_data = htonl(strlen(conn_info->filename) + 1);
			wr.num_sge = 1;
			wr.send_flags = IBV_SEND_SIGNALED;
			wr.wr.rdma.remote_addr = conn_info->buf_info.buf_info_msg.buf_info_data.info.buf_info_addr;
			wr.wr.rdma.rkey = conn_info->buf_info.buf_info_msg.buf_info_data.info.buf_info_rkey;

			sge.addr = (uintptr_t)(conn_info->file_buf.file_buf);
			sge.length = strlen(conn_info->filename) + 1;
			sge.lkey = conn_info->file_buf.mr->lkey;
			if (ibv_post_send(conn_info->qp, &wr, &bad_wr)){
    			exit(0);
    		}
			break;
		case SEND_FILE_CONTENT:
			wr.wr_id = ibv_send_opcode;
			wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
			wr.sg_list = &sge;
			wr.imm_data = htonl(read_size);
			wr.num_sge = 1;
			wr.send_flags = IBV_SEND_SIGNALED;
			wr.wr.rdma.remote_addr = conn_info->buf_info.buf_info_msg.buf_info_data.info.buf_info_addr;
			wr.wr.rdma.rkey = conn_info->buf_info.buf_info_msg.buf_info_data.info.buf_info_rkey;

			sge.addr = (uintptr_t)(conn_info->file_buf.file_buf);
			sge.length = read_size;
			sge.lkey = conn_info->file_buf.mr->lkey;
			if (ibv_post_send(conn_info->qp, &wr, &bad_wr)){
    			exit(0);
    		}
			break;
		default:
			;
	}
}