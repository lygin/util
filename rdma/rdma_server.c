#include "rdma_common.h"

#define LISTEN_PORT 10077
static int on_connect_request(struct rdma_cm_id *id);
static int on_connection(struct rdma_cm_id *id);
static int on_disconnect(struct rdma_cm_id *id);
static int on_event(struct rdma_cm_event *event);
static void usage(const char *argv0);

int main(int argc, char **argv)
{
  struct sockaddr_in6 addr;
  struct rdma_cm_event *event = NULL;
  struct rdma_cm_id *listener = NULL;
  struct rdma_event_channel *ec = NULL;
  uint16_t port = 0;


  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(LISTEN_PORT);
  
  ec = rdma_create_event_channel();
  //1.类似于socketfd，先创建socketfd，只不过多了event_channel
  rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP);
  //2.类似socket的bind(fd, addr)
  rdma_bind_addr(listener, (struct sockaddr *)&addr);
  //3.类似socket listen(fd, backlog)backlog 可以排队的最大连接个数
  rdma_listen(listener, 10); /* backlog=10 is arbitrary */

  port = ntohs(rdma_get_src_port(listener));

  printf("listening on port %d.\n", port);
  //Retrieves a communication event. 
  while (rdma_get_cm_event(ec, &event) == 0) {
    struct rdma_cm_event event_copy;

    memcpy(&event_copy, event, sizeof(*event));
    rdma_ack_cm_event(event);

    if (on_event(&event_copy))
      break;
  }

  rdma_destroy_id(listener);
  rdma_destroy_event_channel(ec);

  return 0;
}

//接受client的rdma_connect
int on_connect_request(struct rdma_cm_id *id)
{
  struct rdma_conn_param cm_params;

  printf("received connection request.\n");
  /*初始化conn*/
  build_connection(id);
  build_params(&cm_params);
  /*初始化单边写的local buffer*/
  sprintf(get_local_message_region(id->context), "message from passive/server side with pid %d", getpid());
  //类似socket accept(listenfd)返回connfd
  rdma_accept(id, &cm_params);

  return 0;
}

//三次握手,cli->server server->cli cli->server
int on_connection(struct rdma_cm_id *id)
{
  on_connect(id->context);

  return 0;
}

int on_disconnect(struct rdma_cm_id *id)
{
  printf("peer disconnected.\n");

  destroy_connection(id->context);
  return 0;
}

int on_event(struct rdma_cm_event *event)
{
  int r = 0;
  //接受到cli的rdma_connect
  if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST)
    r = on_connect_request(event->id);
  else if (event->event == RDMA_CM_EVENT_ESTABLISHED)
    r = on_connection(event->id);
  else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
    r = on_disconnect(event->id);
  else
    die("on_event: unknown event.");

  return r;
}
