#include "rdma_common.hh"
#define DEV_NAME "mlx5_0"

static int on_connect_request(struct rdma_cm_id *id);
static int on_connection(struct rdma_cm_id *id);
static int on_disconnect(struct rdma_cm_id *id);
static int on_event(struct rdma_cm_event *event);

int main(int argc, char **argv)
{
  struct rdma_device *dev = create_rdma_device(DEV_NAME);
  int ret = -1;
  ret = rdma_create_id(dev->event_channel, &dev->listen_id, NULL, RDMA_PS_TCP);
  LOG_ASSERT(ret == 0, "rdma_create_id failed");

  addrinfo server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.ai_family =AF_INET;

  ret = rdma_bind_addr(dev->listen_id, (sockaddr*)&server_addr);
  LOG_ASSERT(ret == 0, "rdma_bind_addr failed");

  ret = rdma_listen(dev->listen_id, 10);
  int port = rdma_get_src_port(dev->listen_id);
  printf("listen on port: %d\b", port);
  
  rdma_cm_event *event;
  while(rdma_get_cm_event(dev->event_channel, &event) == 0) {
    rdma_cm_event event_copy;
    memcpy(&event_copy, event, sizeof(rdma_cm_event));
    rdma_ack_cm_event(event);
    if(on_event(&event_copy)) {
      break;
    }
  }
  rdma_destroy_id(dev->listen_id);
  rdma_destroy_event_channel(dev->event_channel);
  
  return 0;
}

int on_event(struct rdma_cm_event *event)
{
  int r = 0;

  if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST)
    r = on_connect_request(event->id);
  else if (event->event == RDMA_CM_EVENT_ESTABLISHED)
    r = on_connection(event->id);
  else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
    r = on_disconnect(event->id);
  else
    LOG_ASSERT(0, "on_event: unknown event.");

  return r;
}

int on_connect_request(struct rdma_cm_id *id)
{
  struct rdma_conn_param cm_params;

  printf("received connection request.\n");
  build_connection(id);
  build_params(&cm_params);
  sprintf(get_local_message_region(id->context), "message from passive/server side with pid %d", getpid());
  TEST_NZ(rdma_accept(id, &cm_params));

  return 0;
}