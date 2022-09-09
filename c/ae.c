#include "ae.h"
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
/*
 * 创建一个新的 epoll 实例，并将它赋值给 eventLoop
 */
static int aeApiCreate(aeEventLoop *eventLoop) {

    aeApiState *state = (aeApiState *)malloc(sizeof(aeApiState));

    if (!state) return -1;

    // 初始化事件槽空间
    state->events = (struct epoll_event*)malloc(sizeof(struct epoll_event)*eventLoop->setsize);
    if (!state->events) {
        free(state);
        return -1;
    }

    // 创建 epoll 实例
    state->epfd = epoll_create(1024); /* 1024 is just a hint for the kernel */
    if (state->epfd == -1) {
        free(state->events);
        free(state);
        return -1;
    }

    // 赋值给 eventLoop
    eventLoop->apidata = state;
    return 0;
}

/*
 * 关联给定事件到 fd
 */
static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask) {
    aeApiState *state = (aeApiState *)eventLoop->apidata;
    struct epoll_event ee;

    /* If the fd was already monitored for some event, we need a MOD
     * operation. Otherwise we need an ADD operation. 
     *
     * 如果 fd 没有关联任何事件，那么这是一个 ADD 操作。
     *
     * 如果已经关联了某个/某些事件，那么这是一个 MOD 操作。
     */
    int op = eventLoop->events[fd].mask == AE_NONE ?
            EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    // 注册事件到 epoll
    ee.events = 0;
    mask |= eventLoop->events[fd].mask; /* Merge old events */
    if (mask & AE_READABLE) ee.events |= EPOLLIN;
    if (mask & AE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;

    if (epoll_ctl(state->epfd,op,fd,&ee) == -1) return -1;

    return 0;
}

/*
 * 从 fd 中删除给定事件
 */
static void aeApiDelEvent(aeEventLoop *eventLoop, int fd, int delmask) {
    aeApiState *state = (aeApiState *)eventLoop->apidata;
    struct epoll_event ee;

    int mask = eventLoop->events[fd].mask & (~delmask);

    ee.events = 0;
    if (mask & AE_READABLE) ee.events |= EPOLLIN;
    if (mask & AE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (mask != AE_NONE) {
        epoll_ctl(state->epfd,EPOLL_CTL_MOD,fd,&ee);
    } else {
        /* Note, Kernel < 2.6.9 requires a non null event pointer even for
         * EPOLL_CTL_DEL. */
        epoll_ctl(state->epfd,EPOLL_CTL_DEL,fd,&ee);
    }
}

/*
 * 获取可执行事件
 */
static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp) {
    aeApiState *state = (aeApiState *)eventLoop->apidata;
    int retval, numevents = 0;

    // 等待时间
    retval = epoll_wait(state->epfd,state->events,eventLoop->setsize,
            tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);

    // 有至少一个事件就绪？
    if (retval > 0) {
        int j;

        // 为已就绪事件设置相应的模式
        // 并加入到 eventLoop 的 fired 数组中
        numevents = retval;
        for (j = 0; j < numevents; j++) {
            int mask = 0;
            struct epoll_event *e = state->events+j;

            if (e->events & EPOLLIN) mask |= AE_READABLE;
            if (e->events & EPOLLOUT) mask |= AE_WRITABLE;
            if (e->events & EPOLLERR) mask |= AE_WRITABLE;
            if (e->events & EPOLLHUP) mask |= AE_WRITABLE;

            eventLoop->fired[j].fd = e->data.fd;
            eventLoop->fired[j].mask = mask;
        }
    }
    
    // 返回已就绪事件个数
    return numevents;
}

static void aeApiFree(aeEventLoop *eventLoop) {
    aeApiState *state = eventLoop->apidata;

    close(state->epfd);
    free(state->events);
    free(state);
}

aeEventLoop *aeCreateEventLoop(int setsize) {
    aeEventLoop *eventLoop;
    int i;

    // 创建事件状态结构
    if ((eventLoop = (aeEventLoop*)malloc(sizeof(*eventLoop))) == NULL) goto err;

    // 初始化文件事件结构和已就绪文件事件结构数组
    eventLoop->events = (aeFileEvent*)malloc(sizeof(aeFileEvent)*setsize);
    eventLoop->fired = (aeFiredEvent*)malloc(sizeof(aeFiredEvent)*setsize);
    if (eventLoop->events == NULL || eventLoop->fired == NULL) goto err;
    // 设置数组大小
    eventLoop->setsize = setsize;
    // 初始化执行最近一次执行时间
    eventLoop->lastTime = time(NULL);

    eventLoop->stop = 0;
    eventLoop->maxfd = -1;
    if (aeApiCreate(eventLoop) == -1) goto err;

    /* Events with mask == AE_NONE are not set. So let's initialize the
     * vector with it. */
    // 初始化监听事件
    for (i = 0; i < setsize; i++)
        eventLoop->events[i].mask = AE_NONE;

    // 返回事件循环
    return eventLoop;

err:
    if (eventLoop) {
        free(eventLoop->events);
        free(eventLoop->fired);
        free(eventLoop);
    }
    return NULL;
}

/*
 * 删除事件处理器
 */
void aeDeleteEventLoop(aeEventLoop *eventLoop) {
    aeApiFree(eventLoop);
    free(eventLoop->events);
    free(eventLoop->fired);
    free(eventLoop);
}
/*
 * 根据 mask 参数的值，监听 fd 文件的状态，
 * 当 fd 可用时，执行 proc 函数
 */
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData)
{
    if (fd >= eventLoop->setsize) {
        errno = ERANGE;
        return AE_ERR;
    }

    if (fd >= eventLoop->setsize) return AE_ERR;

    // 取出文件事件结构
    aeFileEvent *fe = &eventLoop->events[fd];

    // 监听指定 fd 的指定事件
    if (aeApiAddEvent(eventLoop, fd, mask) == -1)
        return AE_ERR;

    // 设置文件事件类型，以及事件的处理器
    fe->mask |= mask;
    if (mask & AE_READABLE) fe->rfileProc = proc;
    if (mask & AE_WRITABLE) fe->wfileProc = proc;

    // 私有数据
    fe->clientData = clientData;

    // 如果有需要，更新事件处理器的最大 fd
    if (fd > eventLoop->maxfd)
        eventLoop->maxfd = fd;

    return AE_OK;
}

/*
 * 将 fd 从 mask 指定的监听队列中删除
 */
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask)
{
    if (fd >= eventLoop->setsize) return;

    // 取出文件事件结构
    aeFileEvent *fe = &eventLoop->events[fd];

    // 未设置监听的事件类型，直接返回
    if (fe->mask == AE_NONE) return;

    // 计算新掩码
    fe->mask = fe->mask & (~mask);
    if (fd == eventLoop->maxfd && fe->mask == AE_NONE) {
        /* Update the max fd */
        int j;

        for (j = eventLoop->maxfd-1; j >= 0; j--)
            if (eventLoop->events[j].mask != AE_NONE) break;
        eventLoop->maxfd = j;
    }

    // 取消对给定 fd 的给定事件的监视
    aeApiDelEvent(eventLoop, fd, mask);
}

/*
 * 获取给定 fd 正在监听的事件类型
 */
int aeGetFileEvents(aeEventLoop *eventLoop, int fd) {
    if (fd >= eventLoop->setsize) return 0;
    aeFileEvent *fe = &eventLoop->events[fd];

    return fe->mask;
}

/*
 * 取出当前时间的秒和毫秒，
 * 并分别将它们保存到 seconds 和 milliseconds 参数中
 */
static void aeGetTime(long *seconds, long *milliseconds)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    *seconds = tv.tv_sec;
    *milliseconds = tv.tv_usec/1000;
}

/*
 * 在当前时间上加上 milliseconds 毫秒，
 * 并且将加上之后的秒数和毫秒数分别保存在 sec 和 ms 指针中。
 */
static void aeAddMillisecondsToNow(long long milliseconds, long *sec, long *ms) {
    long cur_sec, cur_ms, when_sec, when_ms;

    // 获取当前时间
    aeGetTime(&cur_sec, &cur_ms);

    // 计算增加 milliseconds 之后的秒数和毫秒数
    when_sec = cur_sec + milliseconds/1000;
    when_ms = cur_ms + milliseconds%1000;

    // 进位：
    // 如果 when_ms 大于等于 1000
    // 那么将 when_sec 增大一秒
    if (when_ms >= 1000) {
        when_sec ++;
        when_ms -= 1000;
    }

    // 保存到指针中
    *sec = when_sec;
    *ms = when_ms;
}

/* Process time events
 *
 * 处理所有已到达的时间事件
 */
static int processTimeEvents(aeEventLoop *eventLoop) {
    int processed = 0;
    aeTimeEvent *te;
    long long maxId;
    time_t now = time(NULL);

    // 通过重置事件的运行时间，
    // 防止因时间穿插（skew）而造成的事件处理混乱
    if (now < eventLoop->lastTime) {
        te = eventLoop->timeEventHead;
        while(te) {
            te->when_sec = 0;
            te = te->next;
        }
    }
    // 更新最后一次处理时间事件的时间
    eventLoop->lastTime = now;

    // 遍历链表
    // 执行那些已经到达的事件
    te = eventLoop->timeEventHead;
    maxId = eventLoop->timeEventNextId-1;
    while(te) {
        long now_sec, now_ms;
        long long id;

        // 跳过无效事件
        if (te->id > maxId) {
            te = te->next;
            continue;
        }
        
        // 获取当前时间
        aeGetTime(&now_sec, &now_ms);

        // 如果当前时间等于或等于事件的执行时间，那么说明事件已到达，执行这个事件
        if (now_sec > te->when_sec ||
            (now_sec == te->when_sec && now_ms >= te->when_ms))
        {
            int retval;

            id = te->id;
            // 执行事件处理器，并获取返回值
            retval = te->timeProc(eventLoop, id, te->clientData);
            processed++;

            // 记录是否有需要循环执行这个事件时间
            if (retval != AE_NOMORE) {
                // 是的， retval 毫秒之后继续执行这个时间事件
                aeAddMillisecondsToNow(retval,&te->when_sec,&te->when_ms);
            } else {
                // 不，将这个事件删除
                aeDeleteTimeEvent(eventLoop, id);
            }

            // 因为执行事件之后，事件列表可能已经被改变了
            // 因此需要将 te 放回表头，继续开始执行事件
            te = eventLoop->timeEventHead;
        } else {
            te = te->next;
        }
    }
    return processed;
}

// 寻找里目前时间最近的时间事件
// 因为链表是乱序的，所以查找复杂度为 O（N）
static aeTimeEvent *aeSearchNearestTimer(aeEventLoop *eventLoop)
{
    aeTimeEvent *te = eventLoop->timeEventHead;
    aeTimeEvent *nearest = NULL;

    while(te) {
        if (!nearest || te->when_sec < nearest->when_sec ||
                (te->when_sec == nearest->when_sec &&
                 te->when_ms < nearest->when_ms))
            nearest = te;
        te = te->next;
    }
    return nearest;
}

/* Process every pending time event, then every pending file event
 * (that may be registered by time event callbacks just processed).
 *
 * 处理所有已到达的时间事件，以及所有已就绪的文件事件，（优先处理文件事件，处理完成再处理时间事件）。
 *
 * Without special flags the function sleeps until some file event
 * fires, or when the next time event occurs (if any).
 *
 * 如果不传入特殊 flags 的话，那么函数睡眠直到文件事件就绪，
 * 或者下个时间事件到达（如果有的话）。
 *
 * If flags is 0, the function does nothing and returns.
 * 如果 flags 为 0 ，那么函数不作动作，直接返回。
 *
 * if flags has AE_ALL_EVENTS set, all the kind of events are processed.
 * 如果 flags 包含 AE_ALL_EVENTS ，所有类型的事件都会被处理。
 *
 * if flags has AE_FILE_EVENTS set, file events are processed.
 * 如果 flags 包含 AE_FILE_EVENTS ，那么处理文件事件。
 *
 * if flags has AE_TIME_EVENTS set, time events are processed.
 * 如果 flags 包含 AE_TIME_EVENTS ，那么处理时间事件。
 *
 * if flags has AE_DONT_WAIT set the function returns ASAP until all
 * the events that's possible to process without to wait are processed.
 * 如果 flags 包含 AE_DONT_WAIT ，
 * 那么函数在处理完所有不许阻塞的事件之后，即刻返回。
 *
 * The function returns the number of events processed. 
 * 函数的返回值为已处理事件的数量
 */
int aeProcessEvents(aeEventLoop *eventLoop, int flags)
{
    int processed = 0, numevents;

    /* Nothing to do? return ASAP */
    if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS)) return 0;

    /* Note that we want call select() even if there are no
     * file events to process as long as we want to process time
     * events, in order to sleep until the next time event is ready
     * to fire. */
    if (eventLoop->maxfd != -1 ||
        ((flags & AE_TIME_EVENTS) && !(flags & AE_DONT_WAIT))) {
        int j;
        aeTimeEvent *shortest = NULL;
        struct timeval tv, *tvp;

        // 获取最近的时间事件
        if (flags & AE_TIME_EVENTS && !(flags & AE_DONT_WAIT))
            shortest = aeSearchNearestTimer(eventLoop);
        if (shortest) {
            // 如果时间事件存在的话
            // 那么根据最近可执行时间事件和现在时间的时间差来决定文件事件的阻塞时间
            long now_sec, now_ms;

            /* Calculate the time missing for the nearest
             * timer to fire. */
            // 计算距今最近的时间事件还要多久才能达到
            // 并将该时间距保存在 tv 结构中
            aeGetTime(&now_sec, &now_ms);
            tvp = &tv;
            tvp->tv_sec = shortest->when_sec - now_sec;
            if (shortest->when_ms < now_ms) {
                tvp->tv_usec = ((shortest->when_ms+1000) - now_ms)*1000;
                tvp->tv_sec --;
            } else {
                tvp->tv_usec = (shortest->when_ms - now_ms)*1000;
            }

            // 时间差小于 0 ，说明事件已经可以执行了，将秒和毫秒设为 0 （不阻塞）
            if (tvp->tv_sec < 0) tvp->tv_sec = 0;
            if (tvp->tv_usec < 0) tvp->tv_usec = 0;
        } else {
            
            // 执行到这一步，说明没有时间事件
            // 那么根据 AE_DONT_WAIT 是否设置来决定是否阻塞，以及阻塞的时间长度

            /* If we have to check for events but need to return
             * ASAP because of AE_DONT_WAIT we need to set the timeout
             * to zero */
            if (flags & AE_DONT_WAIT) {
                // 设置文件事件不阻塞
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            } else {
                /* Otherwise we can block */
                // 文件事件可以阻塞直到有事件到达为止
                tvp = NULL; /* wait forever */
            }
        }

        // 处理文件事件，阻塞时间由 tvp 决定
        numevents = aeApiPoll(eventLoop, tvp);
        for (j = 0; j < numevents; j++) {
            // 从已就绪数组中获取事件
            aeFileEvent *fe = &eventLoop->events[eventLoop->fired[j].fd];

            int mask = eventLoop->fired[j].mask;
            int fd = eventLoop->fired[j].fd;
            int rfired = 0;

           /* note the fe->mask & mask & ... code: maybe an already processed
             * event removed an element that fired and we still didn't
             * processed, so we check if the event is still valid. */
            // 读事件
            if (fe->mask & mask & AE_READABLE) {
                // rfired 确保读/写事件只能执行其中一个
                rfired = 1;
                fe->rfileProc(eventLoop,fd,fe->clientData,mask);
            }
            // 写事件
            if (fe->mask & mask & AE_WRITABLE) {
                if (!rfired || fe->wfileProc != fe->rfileProc)
                    fe->wfileProc(eventLoop,fd,fe->clientData,mask);
            }

            processed++;
        }
    }

    /* Check time events */
    // 执行时间事件
    if (flags & AE_TIME_EVENTS)
        processed += processTimeEvents(eventLoop);

    return processed; /* return the number of processed file/time events */
}

/*
 * 事件处理器的主循环
 */
void aeMain(aeEventLoop *eventLoop) {

    eventLoop->stop = 0;

    while (!eventLoop->stop) {
        // 开始处理事件
        aeProcessEvents(eventLoop, AE_ALL_EVENTS);
    }
}

/*
 * 停止事件处理器
 */
void aeStop(aeEventLoop *eventLoop) {
    eventLoop->stop = 1;
}


/*
 * 创建时间事件
 * return timerid
 */
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc)
{
    // 更新时间计数器
    long long id = eventLoop->timeEventNextId++;

    // 创建时间事件结构
    aeTimeEvent *te;

    te = malloc(sizeof(*te));
    if (te == NULL) return AE_ERR;

    // 设置 ID
    te->id = id;

    // 设定处理事件的时间
    aeAddMillisecondsToNow(milliseconds,&te->when_sec,&te->when_ms);
    // 设置事件处理器
    te->timeProc = proc;
    te->finalizerProc = finalizerProc;
    // 设置私有数据
    te->clientData = clientData;

    // 将新事件放入表头
    te->next = eventLoop->timeEventHead;
    eventLoop->timeEventHead = te;

    return id;
}

/*
 * 删除给定 id 的时间事件
 */
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id)
{
    aeTimeEvent *te, *prev = NULL;

    // 遍历链表
    te = eventLoop->timeEventHead;
    while(te) {

        // 发现目标事件，删除
        if (te->id == id) {

            if (prev == NULL)
                eventLoop->timeEventHead = te->next;
            else
                prev->next = te->next;

            // 执行清理处理器
            if (te->finalizerProc)
                te->finalizerProc(eventLoop, te->clientData);

            // 释放时间事件
            free(te);

            return AE_OK;
        }
        prev = te;
        te = te->next;
    }

    return AE_ERR; /* NO event with the specified ID found */
}