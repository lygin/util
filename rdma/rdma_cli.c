#include "rdma_common.h"
#include "assert.h"

const int TIMEOUT_IN_MS = 500;


int main(int argc, char **argv) {
  struct addrinfo *addr;
  struct rdma_cm_event *event = NULL;
  struct rdma_cm_id *conn_id= NULL;
  struct rdma_event_channel *ec = NULL;

  if (argc != 4)
    usage(argv[0]);

  getaddrinfo(argv[1], argv[2], NULL, &addr);
  ec = rdma_create_event_channel();
  
  rdma_create_id(ec, &conn_id, NULL, RDMA_PS_TCP);
  
  //client find server address
  rdma_resolve_addr(conn_id, NULL, addr->ai_addr, TIMEOUT_IN_MS);


}

void usage(const char *argv0)
{
  fprintf(stderr, "usage: %s <server-address> <server-port>\n  mode = \"read\", \"write\"\n", argv0);
  exit(1);
}
