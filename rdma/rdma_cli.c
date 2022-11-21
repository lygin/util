#include "rdma_common.h"

const int TIMEOUT_IN_MS = 500; /* ms */

static int on_addr_resolved(struct rdma_cm_id *id);
static int on_connection(struct rdma_cm_id *id);
static int on_disconnect(struct rdma_cm_id *id);
static int on_event(struct rdma_cm_event *event);
static int on_route_resolved(struct rdma_cm_id *id);
static void usage(const char *argv0);

int main(int argc, char **argv)
{
  struct addrinfo *serv_addr;
  struct rdma_cm_event *event = NULL;
  struct rdma_cm_id *conn= NULL;
  struct rdma_event_channel *ec = NULL;

  if (argc != 3)
    usage(argv[0]);

  getaddrinfo(argv[1], argv[2], NULL, &serv_addr);
  ec = rdma_create_event_channel();
  //1.类似socketfd,只不过多了event_channel
  rdma_create_id(ec, &conn, NULL, RDMA_PS_TCP);
  //2.获取server地址，只等待TIMEOUT_IN_MS
  //类似socket connect(clifd, serv_addr)，只不过RDMA在connect前要通过
  //rdma_resolve_addr解析为rdma地址,rdma_resolve_route解析路由地址两步后才能rdma_connect
  rdma_resolve_addr(conn, NULL, serv_addr->ai_addr, TIMEOUT_IN_MS);

  freeaddrinfo(serv_addr);
  while (rdma_get_cm_event(ec, &event) == 0) {
    struct rdma_cm_event event_copy;

    memcpy(&event_copy, event, sizeof(*event));
    //ack这个event，直接回收event资源
    rdma_ack_cm_event(event);
    //开始处理event
    if (on_event(&event_copy))
      break;
  }
  
  rdma_destroy_event_channel(ec);

  return 0;
}

int on_addr_resolved(struct rdma_cm_id *id)
{
  printf("address resolved.\n");
  //已经解析server地址，初始化connid的conn变量
  build_connection(id);
  sprintf(get_local_message_region(id->context), "message from active/client side with pid %d", getpid());
  //2.after rdma_resolve_addr, befor rdma_connect
  rdma_resolve_route(id, TIMEOUT_IN_MS);

  return 0;
}

int on_connection(struct rdma_cm_id *id)
{
  //置connected=1
  on_connect(id->context);
  //把client的remote addr和rkey发给server，方便remote单边写remote_buffer
  send_mr(id->context);

  return 0;
}

int on_disconnect(struct rdma_cm_id *id)
{
  printf("disconnected.\n");
  
  destroy_connection(id->context);
  return 1; /* exit event loop */
}

// client 处理event
int on_event(struct rdma_cm_event *event)
{
  int r = 0;
  //Address resolution (rdma_resolve_addr) completed successfully.
  //一般是路由器发回event给cli
  if (event->event == RDMA_CM_EVENT_ADDR_RESOLVED)
    r = on_addr_resolved(event->id);
  //Route resolution (rdma_resolve_route) completed successfully.
  //一般是路由器发回event给cli
  else if (event->event == RDMA_CM_EVENT_ROUTE_RESOLVED)
    r = on_route_resolved(event->id);
  //Indicates that a connection has been established with the remote end point.
  //接受到server发回的conn_ack event
  else if (event->event == RDMA_CM_EVENT_ESTABLISHED)
    r = on_connection(event->id);
  else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
    r = on_disconnect(event->id);
  else {
    fprintf(stderr, "on_event: %d\n", event->event);
  }

  return r;
}

int on_route_resolved(struct rdma_cm_id *id)
{
  struct rdma_conn_param cm_params;

  printf("route resolved.\n");
  build_params(&cm_params);
  //可以正式connect server了
  rdma_connect(id, &cm_params);

  return 0;
}

void usage(const char *argv0)
{
  fprintf(stderr, "usage: %s <mode> <server-address> <server-port>\n  mode = \"read\", \"write\"\n", argv0);
  exit(1);
}
