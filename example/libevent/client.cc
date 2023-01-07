extern "C" {
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
}
#define MAX_LINE 4096
char buf[MAX_LINE];

void read_cb(struct bufferevent *bev, void *ctx);
void event_cb(struct bufferevent *bev, short events, void *ctx);
void cli(struct bufferevent *bev);

void usage() {
  printf("usage: ip port\n");
}

int main(int argc, char **argv) {
  if(argc < 3) {
    usage();
    return 1;
  }
	struct event_base *base;
	struct sockaddr_in sin;
	struct bufferevent *bev;
	
	base = event_base_new();
	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &sin.sin_addr);

	memset(sin.sin_zero, 0x00, 0);
	// new event
	bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	// set event call back
	bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
	if (bufferevent_socket_connect(bev, 
				(struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Could not connect!\n");
		bufferevent_free(bev);	
		return -1;
	}
	bufferevent_enable(bev, EV_READ|EV_WRITE);
  /* run loop */
	event_base_dispatch(base);

	return 0;
}

void read_cb(struct bufferevent *bev, void *ctx) {
	int sz = bufferevent_read(bev, buf, MAX_LINE);
	buf[sz] = '\0';
	printf("recv: %s\n", buf);
  cli(bev);
}

void event_cb(struct bufferevent *bev, short events, void *ctx) {
	if (events & BEV_EVENT_CONNECTED) {
		printf("connected\n");
	} else if (events & BEV_EVENT_EOF) {
		printf("Connection closed.\n");
	} else if (events & BEV_EVENT_ERROR) {
		printf("Got an error on the connection: %s\n",
				strerror(errno));
	}
}

void cli(struct bufferevent *bev) {
  printf("> ");
  scanf("%s", buf);
  bufferevent_write(bev, buf, strlen(buf));
}