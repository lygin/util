#include "rdma_common.h"

static const int RDMA_BUFFER_SIZE = 1024;

struct message {
  enum {
    MSG_MR,
    MSG_DONE
  } type;

  union {
    struct ibv_mr mr;
  } data;
};

struct context {
  struct ibv_context *ctx;
  struct ibv_pd *pd; //pd
  struct ibv_cq *cq; //for send_cq and recv_cq
  struct ibv_comp_channel *comp_channel;

  pthread_t cq_poller_thread;
};

struct connection {
  struct rdma_cm_id *id; //comunicate manager id
  
  struct ibv_qp *qp; //sq and rq

  int connected;
  //注册的内存区域returned by ibv_reg_mr
  //The local key (L_Key) field lkey is used as the lkey field of struct ibv_sge 
  //when posting buffers with ibv_post_* verbs, 
  //and the the remote key (R_Key) field rkey is used by remote processes to 
  //perform Atomic and RDMA operations.
  struct ibv_mr *recv_mr;
  struct ibv_mr *send_mr;
  struct ibv_mr *rdma_local_mr;
  struct ibv_mr *rdma_remote_mr;

  struct ibv_mr peer_mr; //used for单边写

  struct message *recv_msg; //recv buffer
  struct message *send_msg; //send buffer

  char *rdma_local_region; //单边write local sge
  char *rdma_remote_region;//单边write remote addr

  enum {
    SS_INIT,
    SS_MR_SENT,
    SS_RDMA_SENT,
    SS_DONE_SENT
  } send_state;

  enum {
    RS_INIT,
    RS_MR_RECV,
    RS_DONE_RECV
  } recv_state;
};

static void build_context(struct ibv_context *verbs);
static void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
static char * get_peer_message_region(struct connection *conn);
static void on_completion(struct ibv_wc *);
static void * poll_cq(void *);
static void post_receives(struct connection *conn);
static void register_memory(struct connection *conn);
static void send_message(struct connection *conn);

static struct context *s_ctx = NULL;
static enum mode s_mode = M_WRITE;

void die(const char *reason)
{
  fprintf(stderr, "%s\n", reason);
  exit(EXIT_FAILURE);
}

// 为id(类似sockfd)建立conn(初始化conn变量)
void build_connection(struct rdma_cm_id *id)
{
  struct connection *conn;
  struct ibv_qp_init_attr qp_attr;

  //1.创建s_context (include verbs_ctx)，包括为device创建pd, cq
  build_context(id->verbs);
  //2.创建qp_attr
  build_qp_attr(&qp_attr);
  //3.为id创建qp
  rdma_create_qp(id, s_ctx->pd, &qp_attr);
  
  //4.为id创建user context保存连接所需的元信息conn
  id->context = conn = (struct connection *)malloc(sizeof(struct connection));

  conn->id = id;
  conn->qp = id->qp;

  conn->send_state = SS_INIT;
  conn->recv_state = RS_INIT;

  conn->connected = 0;
  //5.注册内存
  register_memory(conn);
  //6.提前放入recv请求,这样server send后能立马接受信息到注册的sge
  post_receives(conn);
}

//创建s_ctx user ctx
void build_context(struct ibv_context *verbs)
{
  if (s_ctx) {
    if (s_ctx->ctx != verbs)
      die("cannot handle events in more than one context.");

    return;
  }

  s_ctx = (struct context *)malloc(sizeof(struct context));
  //1.原来device的ctx
  s_ctx->ctx = verbs;
  //2.创建pd for device ctx
  s_ctx->pd = ibv_alloc_pd(s_ctx->ctx);
  //3.创建cc for device ctx
  s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx);
  //4.创建cq
  //creates a completion queue (CQ) with at least cqe entries 
  // for the RDMA device context.
  s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0); /* cqe=10 is arbitrary */
  //5.放一个cq用于验证
  ibv_req_notify_cq(s_ctx->cq, 0);
  //新建一个线程用于后台poll_cq，处理完成的任务
  pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL);
}

void build_params(struct rdma_conn_param *params)
{
  memset(params, 0, sizeof(*params));

  params->initiator_depth = params->responder_resources = 1;
  params->rnr_retry_count = 7; /* infinite retry */
}

void build_qp_attr(struct ibv_qp_init_attr *qp_attr)
{
  memset(qp_attr, 0, sizeof(*qp_attr));

  qp_attr->send_cq = s_ctx->cq;
  qp_attr->recv_cq = s_ctx->cq;
  qp_attr->qp_type = IBV_QPT_RC;//RC

  qp_attr->cap.max_send_wr = 10;
  qp_attr->cap.max_recv_wr = 10;
  qp_attr->cap.max_send_sge = 1;
  qp_attr->cap.max_recv_sge = 1;
}

void destroy_connection(void *context)
{
  struct connection *conn = (struct connection *)context;

  rdma_destroy_qp(conn->id);
  //dereg mr消除保护
  ibv_dereg_mr(conn->send_mr);
  ibv_dereg_mr(conn->recv_mr);
  ibv_dereg_mr(conn->rdma_local_mr);
  ibv_dereg_mr(conn->rdma_remote_mr);
  //释放空间
  free(conn->send_msg);
  free(conn->recv_msg);
  free(conn->rdma_local_region);
  free(conn->rdma_remote_region);

  rdma_destroy_id(conn->id);

  free(conn);
}

void * get_local_message_region(void *context)
{
  if (s_mode == M_WRITE)
    return ((struct connection *)context)->rdma_local_region;
  else
    return ((struct connection *)context)->rdma_remote_region;
}

char * get_peer_message_region(struct connection *conn)
{
  if (s_mode == M_WRITE)
    return conn->rdma_remote_region;
  else
    return conn->rdma_local_region;
}

void on_completion(struct ibv_wc *wc)
{
  // user ctx
  struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;
  /* Status of the operation */
  if (wc->status != IBV_WC_SUCCESS)
    die("on_completion: status is not IBV_WC_SUCCESS.");

  if (wc->opcode & IBV_WC_RECV) {
    conn->recv_state++;//切换状态

    if (conn->recv_msg->type == MSG_MR) {
      memcpy(&conn->peer_mr, &conn->recv_msg->data.mr, sizeof(conn->peer_mr));
      post_receives(conn); /*已经接受到mr了，准备接受recv_buf数据了 post_recv only rearm for MSG_MR */

      if (conn->send_state == SS_INIT) /* received peer's MR before sending ours, so send ours back */
        send_mr(conn);
    }

  } else {//IBV_WC_SEND
    conn->send_state++;//切换状态
    printf("send completed successfully.\n");
  }
  /*send recv已经交换mr了，开始单边写*/
  if (conn->send_state == SS_MR_SENT && conn->recv_state == RS_MR_RECV) {
    
    struct ibv_send_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    if (s_mode == M_WRITE)
      printf("received MSG_MR. writing message to remote memory...\n");
    else
      printf("received MSG_MR. reading message from remote memory...\n");

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = (uintptr_t)conn;
    wr.opcode = (s_mode == M_WRITE) ? IBV_WR_RDMA_WRITE : IBV_WR_RDMA_READ;//单边write wr
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;
    //单边write 需要对方buf的addr和rkey 无需对方post_receive 
    wr.wr.rdma.remote_addr = (uintptr_t)conn->peer_mr.addr;//remote_addr
    wr.wr.rdma.rkey = conn->peer_mr.rkey;//rkey
    
    sge.addr = (uintptr_t)conn->rdma_local_region; //local_addr
    sge.length = RDMA_BUFFER_SIZE; //length
    sge.lkey = conn->rdma_local_mr->lkey; //lkey
    //单边也是用post_send，只是opcode=IBV_WR_RDMA_WRITE
    ibv_post_send(conn->qp, &wr, &bad_wr);
    //写完了数据，可以send MSG_DONE信息关闭连接了
    conn->send_msg->type = MSG_DONE;
    send_message(conn);

  } else if (conn->send_state == SS_DONE_SENT && conn->recv_state == RS_DONE_RECV) {
    //打印远端单边写到remote_buf的信息
    printf("remote buffer: %s\n", get_peer_message_region(conn));
    //Disconnects a connection and transitions any associated QP to the error state,
    //which will flush any posted work requests to the completion queue.
    //This routine should be called by both the client and server side of a connection. 
    //After successfully disconnecting, an RDMA_CM_EVENT_DISCONNECTED event will be generated
    //on both sides of the connection.
    rdma_disconnect(conn->id);
  }
}

void on_connect(void *context)
{
  ((struct connection *)context)->connected = 1;
}

//后台线程轮询poll_cq,内置状态机用于处理每次poll到的wc
void * poll_cq(void *ctx)
{
  struct ibv_cq *cq;
  struct ibv_wc wc;

  while (1) {
    //waits for the next completion event in the completion event channel channel.
    // Fills the arguments cq with the CQ that got the event 
    //cq_context with the CQ's context.
    ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx);
    // may be relatively expensive in the datapath since it must take a mutex.
    // better acking several completion events in one call to ibv_ack_cq_events().
    ibv_ack_cq_events(cq, 1);
    //再放一个cq
    ibv_req_notify_cq(cq, 0);
    //polls the CQ cq for work completions and returns the first num_entries
    //in the array wc
    //拿一个cq到wc work completion
    //ibv_poll_cq() returns a non-negative value equal to the number of completions found
    while (ibv_poll_cq(cq, 1, &wc))
      //如果wr是软件下发给硬件的“任务书”的话，那么wc就是硬件完成任务之后返回给
      //软件的“任务报告”。wc中描述了某个任务是被正确无误的执行，还是遇到了错误，
      //如果遇到了错误，那么错误的原因是什么。
      on_completion(&wc);
  }

  return NULL;
}

void post_receives(struct connection *conn)
{
  struct ibv_recv_wr wr, *bad_wr = NULL;
  struct ibv_sge sge;

  wr.wr_id = (uintptr_t)conn;/* User defined WR ID */
  wr.next = NULL;  /* Pointer to next WR in list, NULL if last WR */
  wr.sg_list = &sge;/* Pointer to the s/g array */
  wr.num_sge = 1;/* Size of the s/g array */

  sge.addr = (uintptr_t)conn->recv_msg;/* Start address of the local memory buffer */
  sge.length = sizeof(struct message);/* Length of the buffer */
  sge.lkey = conn->recv_mr->lkey;/* Key of the local Memory Region, is the return value by ibv_reg_mr */
  
  //ibv_post_recv() posts the linked list of work requests (WRs) 
  // starting with wr to the receive queue of the queue pair qp. 
  //接收端软件需要给硬件下发一个表示接收任务的wr
  //这样硬件才知道收到数据之后放到内存中的哪个位置。
  //我们提到的Post操作，对于SQ来说称为Post Send，对于RQ来说称为Post Receive。
  //returns 0 on success, or the value of errno on failure 
  ibv_post_recv(conn->qp, &wr, &bad_wr);
  //The buffers used by a WR(recv buf) can only be safely reused 
  //after a work completion has been retrieved from the corresponding (CQ).
}

void register_memory(struct connection *conn)
{
  //1.先为本地的memory buffer alloc空间
  conn->send_msg = malloc(sizeof(struct message));
  conn->recv_msg = malloc(sizeof(struct message));

  conn->rdma_local_region = malloc(RDMA_BUFFER_SIZE);
  conn->rdma_remote_region = malloc(RDMA_BUFFER_SIZE);

  //2.再为alloc的buffer注册保护域pd
  //ibv_reg_mr() registers a memory region (MR) associated with the protection domain pd.
  //The argument access describes the desired memory protection attributes
  //returns a pointer to the registered MR, or NULL if the request fails.
  conn->send_mr = ibv_reg_mr(
    s_ctx->pd, 
    conn->send_msg, 
    sizeof(struct message), 
    0/*只允许用户rw，不允许nic rw，因为用户放入数据到send_buf*/);

  conn->recv_mr = ibv_reg_mr(
    s_ctx->pd, 
    conn->recv_msg, 
    sizeof(struct message), 
    IBV_ACCESS_LOCAL_WRITE/*允许本地nic write，因为recv，需要把数据从nic搬到recv_buf*/);

  conn->rdma_local_mr = ibv_reg_mr(
    s_ctx->pd, 
    conn->rdma_local_region, 
    RDMA_BUFFER_SIZE, 
    ((s_mode == M_WRITE) ? 0 : IBV_ACCESS_LOCAL_WRITE));

  conn->rdma_remote_mr = ibv_reg_mr(
    s_ctx->pd, 
    conn->rdma_remote_region, 
    RDMA_BUFFER_SIZE, 
    ((s_mode == M_WRITE) ? (IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE) : IBV_ACCESS_REMOTE_READ));
}

void send_message(struct connection *conn)
{
  struct ibv_send_wr wr, *bad_wr = NULL;
  struct ibv_sge sge;

  memset(&wr, 0, sizeof(wr));

  wr.wr_id = (uintptr_t)conn;/* User defined WR ID */
  wr.opcode = IBV_WR_SEND;//send wr
  wr.sg_list = &sge;
  wr.num_sge = 1;
  wr.send_flags = IBV_SEND_SIGNALED;/* Flags of the WR properties */

  sge.addr = (uintptr_t)conn->send_msg;//send addr
  sge.length = sizeof(struct message);//send length
  sge.lkey = conn->send_mr->lkey;//lkey

  while (!conn->connected);
  //发送sge里的内容
  ibv_post_send(conn->qp, &wr, &bad_wr);
}

void send_mr(void *context)
{
  struct connection *conn = (struct connection *)context;
  //把remote_mr(包含rkey)发送
  conn->send_msg->type = MSG_MR;
  memcpy(&conn->send_msg->data.mr, conn->rdma_remote_mr, sizeof(struct ibv_mr));

  send_message(conn);
}

void set_mode(enum mode m)
{
  s_mode = m;
}
