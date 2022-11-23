#include "spdk/stdinc.h"

#include "spdk/nvme.h"
#include "spdk/vmd.h"
#include "spdk/nvme_zns.h"
#include "spdk/env.h"
#include "spdk/string.h"
#include "spdk/log.h"

struct ctrlr_entry
{
	struct spdk_nvme_ctrlr *ctrlr;/*nvme 控制器*/
	TAILQ_ENTRY(ctrlr_entry) link;/*链表*/
	char name[1024]; /*控制器名*/
};

struct ns_entry
{
	struct spdk_nvme_ctrlr *ctrlr; /*nvme 控制器*/
	struct spdk_nvme_ns *ns; /*nvme namespace*/
	TAILQ_ENTRY(ns_entry) link; /*链表*/
	struct spdk_nvme_qpair *qpair; /* qp */
};

/*控制器链表头*/
static TAILQ_HEAD(, ctrlr_entry) g_controllers = TAILQ_HEAD_INITIALIZER(g_controllers);
/*namespace链表头*/
static TAILQ_HEAD(, ns_entry) g_namespaces = TAILQ_HEAD_INITIALIZER(g_namespaces);
/*类似rdma cmid 代表nvmf的终端*/
static struct spdk_nvme_transport_id g_trid = {};

static bool g_vmd = false;

/*用一个控制器注册ns， 并放入g_namespaces链表末尾*/
static void register_ns(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_ns *ns)
{
	struct ns_entry *entry;

	if (!spdk_nvme_ns_is_active(ns))
	{
		return;
	}

	entry = malloc(sizeof(struct ns_entry));
	if (entry == NULL)
	{
		perror("ns_entry malloc");
		exit(1);
	}

	entry->ctrlr = ctrlr;
	entry->ns = ns;
	TAILQ_INSERT_TAIL(&g_namespaces, entry, link);

	printf("  Namespace ID: %d size: %juGB\n", spdk_nvme_ns_get_id(ns),
				 spdk_nvme_ns_get_size(ns) / 1000'000'000);
}

struct hello_world_sequence
{
	struct ns_entry *ns_entry;
	char *buf;
	unsigned using_cmb_io;
	int is_completed;
};

static void read_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct hello_world_sequence *sequence = arg;

	if (spdk_nvme_cpl_is_error(completion))
	{
		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Read I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}

	printf("%s", sequence->buf);
	spdk_free(sequence->buf);
	sequence->is_completed = 1;
}

static void write_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct hello_world_sequence *sequence = arg;
	struct ns_entry *ns_entry = sequence->ns_entry;
	int rc;

	/* check error status */
	if (spdk_nvme_cpl_is_error(completion))
	{
		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Write I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}
	/* 写IO完成，释放buff，并分配新空间给读*/
	if (sequence->using_cmb_io)
	{
		spdk_nvme_ctrlr_unmap_cmb(ns_entry->ctrlr);
	}
	else
	{
		spdk_free(sequence->buf);
	}
	sequence->buf = spdk_zmalloc(0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

	rc = spdk_nvme_ns_cmd_read(ns_entry->ns, ns_entry->qpair, sequence->buf,
														 0, /* LBA start */
														 1, /* number of LBAs */
														 read_complete, (void *)sequence, 0);
	if (rc != 0)
	{
		fprintf(stderr, "starting read I/O failed\n");
		exit(1);
	}
}

static void reset_zone_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct hello_world_sequence *sequence = arg;

	sequence->is_completed = 1;

	if (spdk_nvme_cpl_is_error(completion))
	{
		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Reset zone I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}
}

static void reset_zone_and_wait_for_completion(struct hello_world_sequence *sequence)
{
	if (spdk_nvme_zns_reset_zone(sequence->ns_entry->ns, sequence->ns_entry->qpair,
															 0,			/* starting LBA of the zone to reset */
															 false, /* don't reset all zones */
															 reset_zone_complete,
															 sequence))
	{
		fprintf(stderr, "starting reset zone I/O failed\n");
		exit(1);
	}
	while (!sequence->is_completed)
	{
		spdk_nvme_qpair_process_completions(sequence->ns_entry->qpair, 0);
	}
	sequence->is_completed = 0;
}

static void hello_world(void)
{
	struct ns_entry *ns_entry;
	struct hello_world_sequence sequence;
	int rc;
	size_t sz;

	TAILQ_FOREACH(ns_entry, &g_namespaces, link)
	{
		/* 在控制器上alloc一个qp用来提交read write请求到namespace */
		/* 支持一个控制器多个qp， 在一个控制器上的qp可以提交IO请求到任何相同控制器上的namespace*/
		/* 必须确保只有一个线程提交IO请求到qp，而且必须之后check completion*/
		ns_entry->qpair = spdk_nvme_ctrlr_alloc_io_qpair(ns_entry->ctrlr, NULL, 0);
		if (ns_entry->qpair == NULL)
		{
			printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
			return;
		}

		/*
		 * 为spdk io请求分配buffer
		 */
		/* 把之前ctrlr预留的buff映射出来，sz是buff大小，这个空间不是host的，是ssd的*/
		sequence.using_cmb_io = 1;
		sequence.buf = spdk_nvme_ctrlr_map_cmb(ns_entry->ctrlr, &sz);
		/* 用spdk_zmalloc分配4K对齐的4K大小buff，已经初始化为0*/
		if (sequence.buf == NULL || sz < 0x1000)
		{
			sequence.using_cmb_io = 0;
			sequence.buf = spdk_zmalloc(0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
		}
		if (sequence.buf == NULL)
		{
			printf("ERROR: write buffer allocation failed\n");
			return;
		}
		if (sequence.using_cmb_io)
		{
			printf("INFO: using controller memory buffer for IO\n");
		}
		else
		{
			printf("INFO: using host memory buffer for IO\n");
		}
		sequence.is_completed = 0;
		sequence.ns_entry = ns_entry;

		/*
		 * If the namespace is a Zoned Namespace, rather than a regular
		 * NVM namespace, we need to reset the first zone, before we
		 * write to it. This not needed for regular NVM namespaces.
		 */
		if (spdk_nvme_ns_get_csi(ns_entry->ns) == SPDK_NVME_CSI_ZNS)
		{
			reset_zone_and_wait_for_completion(&sequence);
		}

		/*
		 * write_buf先写入tgt的LBA0，然后从tgt的LBA0读到read_buf
		 */
		snprintf(sequence.buf, 0x1000, "%s", "Hello world!\n");


		/* 把buff写入ns的LBA0，写完后调用回调write_complete，sequence为回调的参数，
		 * 这样就可以为不同的路线设置不同的回调，并知道是哪个IO完成了*/
		/* spdk driver 只会检测是否完成 当调用spdk_nvme_qpair_process_completions类似poll cq*/
		rc = spdk_nvme_ns_cmd_write(ns_entry->ns, ns_entry->qpair, sequence.buf,
																0, /* LBA start */
																1, /* number of LBAs */
																write_complete, &sequence, 0);
		if (rc != 0)
		{
			fprintf(stderr, "starting write I/O failed\n");
			exit(1);
		}

		/* 类似poll cq，如果没有wc不会阻塞会立即返回,返回值是完成的数量*/
		/* 当IO完成时，对应回调write_complete会提交一个新IO请求读LBA到buff，
		 * 然后指定read_complete作为自己的回调，当读IO完成，read_complete会调用，然后打印buff内容，
		 * 然后设置sequence.is_completed = 1，退出循环*/
		while (!sequence.is_completed)
		{
			spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
		}

		/*在free qp之前要保证所有IO请求已经完成*/
		spdk_nvme_ctrlr_free_io_qpair(ns_entry->qpair);
	}
}

static bool probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
				 struct spdk_nvme_ctrlr_opts *opts)
{
	printf("Attaching to %s\n", trid->traddr);

	return true;
}

static void attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
					struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
	int nsid;
	struct ctrlr_entry *entry;
	struct spdk_nvme_ns *ns;
	const struct spdk_nvme_ctrlr_data *cdata;

	entry = malloc(sizeof(struct ctrlr_entry));
	if (entry == NULL)
	{
		perror("ctrlr_entry malloc");
		exit(1);
	}

	printf("Attached to %s\n", trid->traddr);

	/*
	 * spdk_nvme_ctrlr is the logical abstraction in SPDK for an NVMe
	 *  controller.  During initialization, the IDENTIFY data for the
	 *  controller is read using an NVMe admin command, and that data
	 *  can be retrieved using spdk_nvme_ctrlr_get_data() to get
	 *  detailed information on the controller.  Refer to the NVMe
	 *  specification for more details on IDENTIFY for NVMe controllers.
	 */
	cdata = spdk_nvme_ctrlr_get_data(ctrlr);

	snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);

	entry->ctrlr = ctrlr;
	TAILQ_INSERT_TAIL(&g_controllers, entry, link);

	/*
	 * Each controller has one or more namespaces.  An NVMe namespace is basically
	 *  equivalent to a SCSI LUN.  The controller's IDENTIFY data tells us how
	 *  many namespaces exist on the controller.  For Intel(R) P3X00 controllers,
	 *  it will just be one namespace.
	 *
	 * Note that in NVMe, namespace IDs start at 1, not 0.
	 */
	for (nsid = spdk_nvme_ctrlr_get_first_active_ns(ctrlr); nsid != 0;
			 nsid = spdk_nvme_ctrlr_get_next_active_ns(ctrlr, nsid))
	{
		ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
		if (ns == NULL)
		{
			continue;
		}
		register_ns(ctrlr, ns);
	}
}

static void cleanup(void)
{
	struct ns_entry *ns_entry, *tmp_ns_entry;
	struct ctrlr_entry *ctrlr_entry, *tmp_ctrlr_entry;
	struct spdk_nvme_detach_ctx *detach_ctx = NULL;

	TAILQ_FOREACH_SAFE(ns_entry, &g_namespaces, link, tmp_ns_entry)
	{
		TAILQ_REMOVE(&g_namespaces, ns_entry, link);
		free(ns_entry);
	}

	TAILQ_FOREACH_SAFE(ctrlr_entry, &g_controllers, link, tmp_ctrlr_entry)
	{
		TAILQ_REMOVE(&g_controllers, ctrlr_entry, link);
		spdk_nvme_detach_async(ctrlr_entry->ctrlr, &detach_ctx);
		free(ctrlr_entry);
	}

	if (detach_ctx)
	{
		spdk_nvme_detach_poll(detach_ctx);
	}
}

static void
usage(const char *program_name)
{
	printf("%s [options]", program_name);
	printf("\t\n");
	printf("options:\n");
	printf("\t[-d DPDK huge memory size in MB]\n");
	printf("\t[-g use single file descriptor for DPDK memory segments]\n");
	printf("\t[-i shared memory group ID]\n");
	printf("\t[-r remote NVMe over Fabrics target address]\n");
	printf("\t[-V enumerate VMD]\n");
#ifdef DEBUG
	printf("\t[-L enable debug logging]\n");
#else
	printf("\t[-L enable debug logging (flag disabled, must reconfigure with --enable-debug)]\n");
#endif
}

static int
parse_args(int argc, char **argv, struct spdk_env_opts *env_opts)
{
	int op, rc;

	spdk_nvme_trid_populate_transport(&g_trid, SPDK_NVME_TRANSPORT_PCIE);
	snprintf(g_trid.subnqn, sizeof(g_trid.subnqn), "%s", SPDK_NVMF_DISCOVERY_NQN);

	while ((op = getopt(argc, argv, "d:gi:r:L:V")) != -1)
	{
		switch (op)
		{
		case 'V':
			g_vmd = true;
			break;
		case 'i':
			env_opts->shm_id = spdk_strtol(optarg, 10);
			if (env_opts->shm_id < 0)
			{
				fprintf(stderr, "Invalid shared memory ID\n");
				return env_opts->shm_id;
			}
			break;
		case 'g':
			env_opts->hugepage_single_segments = true;
			break;
		case 'r':
			if (spdk_nvme_transport_id_parse(&g_trid, optarg) != 0)
			{
				fprintf(stderr, "Error parsing transport address\n");
				return 1;
			}
			break;
		case 'd':
			env_opts->mem_size = spdk_strtol(optarg, 10);
			if (env_opts->mem_size < 0)
			{
				fprintf(stderr, "Invalid DPDK memory size\n");
				return env_opts->mem_size;
			}
			break;
		case 'L':
			rc = spdk_log_set_flag(optarg);
			if (rc < 0)
			{
				fprintf(stderr, "unknown flag\n");
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
#ifdef DEBUG
			spdk_log_set_print_level(SPDK_LOG_DEBUG);
#endif
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int rc;
	struct spdk_env_opts opts;

	/* 初始化env用于处理内存alloc和pci操作*/
	spdk_env_opts_init(&opts);
	/* [-r remote NVMe over Fabrics target address] */
	rc = parse_args(argc, argv, &opts);
	if (rc != 0)
	{
		return rc;
	}

	opts.name = "hello_world"; /* env name */
	if (spdk_env_init(&opts) < 0)
	{
		fprintf(stderr, "Unable to initialize SPDK env\n");
		return 1;
	}
  printf("initialize SPDK env done\n");
	printf("Initializing NVMe Controllers\n");


	/* 探查spdk ctrlr， 如果找到就回调probe_cb，return true表示接受这个ctrlr*/
	/* 如果nvme driver初始化这个ctrlr之后就会回调attach_cb，把ctrlr和它活跃的ns注册到APP g_ctrlr g_ns*/
	rc = spdk_nvme_probe(&g_trid, NULL, probe_cb, attach_cb, NULL);
	if (rc != 0)
	{
		fprintf(stderr, "spdk_nvme_probe() failed\n");
		rc = 1;
		goto exit;
	}

	if (TAILQ_EMPTY(&g_controllers))
	{
		fprintf(stderr, "no NVMe controllers found\n");
		rc = 1;
		goto exit;
	}

	printf("Initialization complete.\n");
	hello_world();
	cleanup();

exit:
	cleanup();
	spdk_env_fini();
	return rc;
}
