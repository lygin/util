#ifndef _C_COROUTINE_H_
#define _C_COROUTINE_H_

#include <ucontext.h>
#include <stddef.h>


#define STACK_SIZE (1024*1024) //1MB
#define DEFAULT_COROUTINE 16 //schedule cap

#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

struct coroutine;
typedef void (*coroutine_func)(struct schedule *, void *ud);

struct schedule {
	char stack[STACK_SIZE];
	ucontext_t main;
	int nco; //coroutine count
	int cap; //max coroutine size
	int running; //which coroutine is running, -1 if no coroutine running, otherwise running=co_id
	struct coroutine **co;
};

struct coroutine {
	coroutine_func func;
	void *ud; //func args
	ucontext_t ctx;
	struct schedule * sch;
	ptrdiff_t cap;
	ptrdiff_t size;
	int status;
	char *stack;
};

/* API */
struct schedule * coroutine_open(void);
void coroutine_close(struct schedule *);

int coroutine_new(struct schedule *, coroutine_func, void *ud);
void coroutine_resume(struct schedule *, int id);
int coroutine_status(struct schedule *, int id);
int coroutine_running(struct schedule *);
void coroutine_yield(struct schedule *);

#endif