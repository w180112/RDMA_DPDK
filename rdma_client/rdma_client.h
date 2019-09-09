#ifndef _RDMA_CLIENT_H_
#define _RDMA_CLIENT_H_

#include <rdma/rdma_cma.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_THREAD 2

enum { 
    RESOLVE_TIMEOUT_MS = 5000, 
}; 

struct pdata { 
        uint64_t	buf_va; 
        uint32_t	buf_rkey;
}; 

enum {
	RECV_BUF_ADDR,
	RECV_READY_MSG,
	RECV_COMPLETE_MSG,
	SEND_FILE_NAME,
	SEND_FILE_CONTENT,
};

typedef struct file_buf {
	struct ibv_mr 				*mr;
	char 						*file_buf;
}file_buf_t;

typedef struct buf_info {
	struct buf_info_msg {
		uint8_t					opcode;
		uint8_t					lcore_id;
		union {
			struct {
				uint64_t		buf_info_addr;
				uint32_t		buf_info_rkey;
			}info;
		}buf_info_data;
	}buf_info_msg;
	struct ibv_mr 				*buf_info_mr;
}buf_info_t;

typedef struct conn_info {
	char 						ip_addr[15];
	uint32_t 					val[2];
	struct rdma_event_channel	*cm_channel;
	struct rdma_cm_id			*cm_id;
	struct rdma_cm_id           *listen_id;
	struct ibv_comp_channel		*comp_chan;
	struct ibv_qp 				*qp;
	struct ibv_cq 				*cq;
	struct ibv_mr 				*mr;
	struct ibv_pd 				*pd;
	void						*cq_context;
	file_buf_t					file_buf;
	buf_info_t 					buf_info;
	FILE 						*fp;
	char						filename[256];
}conn_info_t;

#endif