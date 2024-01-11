#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/queue.h>
#include "rte_ring.h"

/* true if x is a power of 2 */
#define POWEROF2(x) ((((x)-1) & (x)) == 0)

/* create the ring */
struct rte_ring *
rte_ring_create(const char *name, unsigned count, unsigned flags)
{
	struct rte_ring *r;
	size_t ring_size;

	/* count must be a power of 2 */
	if ((!POWEROF2(count)) || (count > RTE_RING_SZ_MASK )) {
		errno = EINVAL;
		return NULL;
	}

	ring_size = count * sizeof(void *) + sizeof(struct rte_ring);

	r = (struct rte_ring *)malloc(ring_size);
	if (r != NULL) {

		/* init the ring structure */
		memset(r, 0, sizeof(*r));
		snprintf(r->name, sizeof(r->name), "%s", name);
		r->flags = flags;
		r->prod.watermark = count; //最大水位就是传入的capacity
		r->prod.sp_enqueue = !!(flags & RING_F_SP_ENQ);
		r->cons.sc_dequeue = !!(flags & RING_F_SC_DEQ);
		r->prod.size = r->cons.size = count;
		r->prod.mask = r->cons.mask = count-1;
		r->prod.head = r->cons.head = 0;
		r->prod.tail = r->cons.tail = 0;
	} else {
		errno = ENOBUFS;
	}
	
	return r;
}

/*
 * change the high water mark. If *count* is 0, water marking is
 * disabled
 */
int
rte_ring_set_water_mark(struct rte_ring *r, unsigned count)
{
	if (count >= r->prod.size)
		return -EINVAL;

	/* if count is 0, disable the watermarking */
	if (count == 0)
		count = r->prod.size;

	r->prod.watermark = count;
	return 0;
}

/* dump the status of the ring on the console */
void
rte_ring_dump(const struct rte_ring *r)
{
	printf("ring <%s>@%p\n", r->name, r);
	printf("  flags=%x\n", r->flags);
	printf("  size=%"PRIu32"\n", r->prod.size);
	printf("  ct=%"PRIu32"\n", r->cons.tail);
	printf("  ch=%"PRIu32"\n", r->cons.head);
	printf("  pt=%"PRIu32"\n", r->prod.tail);
	printf("  ph=%"PRIu32"\n", r->prod.head);
	printf("  used=%u\n", rte_ring_count(r));
	printf("  avail=%u\n", rte_ring_free_count(r));
	if (r->prod.watermark == r->prod.size)
		printf("  watermark=0\n");
	else
		printf("  watermark=%"PRIu32"\n", r->prod.watermark);
}

void
rte_ring_free(struct rte_ring *r)
{
   free(r); 
}