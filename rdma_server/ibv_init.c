#include "rdma_server.h"

int ibv_init(conn_info_t *conn_info);
int reg_mr(conn_info_t *conn_info);

int ibv_init(conn_info_t *conn_info)
{
	struct ibv_qp_init_attr 	qp_init_attr = { };
	struct ibv_qp_attr 			qp_attr;
	int 						qp_flags;
	int 						ret;

	conn_info->comp_chan = ibv_create_comp_channel(conn_info->cm_id->verbs);
    if (!conn_info->comp_chan)
        return 1;
	conn_info->pd = ibv_alloc_pd(conn_info->cm_id->verbs);
   	if (!conn_info->pd) 
        return 1;
	conn_info->cq = ibv_create_cq(conn_info->cm_id->verbs, 100, NULL, conn_info->comp_chan, 0); 
    if (!conn_info->cq)
      	return 1;
    if (ibv_req_notify_cq(conn_info->cq, 0))
       	return 1;
   
	memset(&qp_init_attr,0,sizeof(qp_init_attr));
    qp_init_attr.cap.max_send_wr = 8;
    qp_init_attr.cap.max_send_sge = 8;
    qp_init_attr.cap.max_recv_wr = 8;
    qp_init_attr.cap.max_recv_sge = 8;

    qp_init_attr.send_cq = conn_info->cq;
    qp_init_attr.recv_cq = conn_info->cq;

    qp_init_attr.qp_type = IBV_QPT_RC;
    conn_info->qp = ibv_create_qp(conn_info->pd,&qp_init_attr);
	if (!conn_info->qp) {
		perror("qp create fail");
		exit(1);
	}
	memset(&qp_attr,0,sizeof(qp_attr));
	qp_flags = IBV_QP_STATE | IBV_QP_PORT;
	qp_attr.qp_state = IBV_QPS_INIT;
	qp_attr.port_num = 1;
	ret = ibv_modify_qp(conn_info->qp,&qp_attr,qp_flags);
	if (ret < 0) {
		perror("qp modify fail to init");
		exit(1);
	}
	memset(&qp_attr,0,sizeof(qp_attr));
	qp_flags = IBV_QP_STATE;
	qp_attr.qp_state = IBV_QPS_RTR;
	ret = ibv_modify_qp(conn_info->qp,&qp_attr,qp_flags);
	if (ret < 0) {
		perror("qp modify fail to recv");
		exit(1);
	}
	memset(&qp_attr,0,sizeof(qp_attr));
	qp_flags = IBV_QP_STATE;
	qp_attr.qp_state = IBV_QPS_RTS;
	ret = ibv_modify_qp(conn_info->qp,&qp_attr,qp_flags);
	if (ret < 0) {
		perror("qp modify fail to send");
		exit(1);
	}
	return 0;
}

int reg_mr(conn_info_t *conn_info)
{
    conn_info->buf_info.buf_info_mr = ibv_reg_mr(conn_info->pd, &(conn_info->buf_info.buf_info_msg),sizeof(struct buf_info_msg), 
        IBV_ACCESS_LOCAL_WRITE | 
        IBV_ACCESS_REMOTE_READ | 
        IBV_ACCESS_REMOTE_WRITE); 
    if (!conn_info->buf_info.buf_info_mr) 
        return 1;
    conn_info->file_buf.file_buf = calloc(10485760,sizeof(uint8_t));
    if (!conn_info->file_buf.file_buf)
        return 1;
    conn_info->file_buf.mr = ibv_reg_mr(conn_info->pd, conn_info->file_buf.file_buf,10485760, 
        IBV_ACCESS_LOCAL_WRITE | 
        IBV_ACCESS_REMOTE_READ | 
        IBV_ACCESS_REMOTE_WRITE); 
   	if (!conn_info->file_buf.mr) 
       	return 1;
    return 0;
}