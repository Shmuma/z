#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>

#include "log.h"
#include "hfs.h"

/* Stupid Zabbix begin */
char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };

static void print_help(int ret)  {
        printf("Usage: hfs-test <options>\n"
               "Options:\n"
               "  -h, --help                  Output a brief help message.\n"
	       "  -r, --hfs-root=PATH         HFS base directory (default: /tmp/hfs).\n"
	       "  -s, --site-id=ID            Site id (default: Default).\n"
	       "  -p, --type=(h|t)            Item type h=history, t=trend (default: h).\n"
	       "  -i, --itemid=NUM            Itemid\n"
	       "  -x, --size=NUM              Graph width (default: 908).\n"
	       "\n"
	       "  -f, --from=FROM-TIMESTAMP   Start data.\n"
	       "  -t, --to=TO-TIMESTAMP       Stop data.\n"
	       "\n"
	       "  -F, --graph-from=TIMESTAMP  Graph start (default: FROM-TIMESTAMP).\n"
	       "  -T, --graph-to=TIMESTAMP    Graph stop (default: TO-TIMESTAMP).\n"
               "\n");
        exit(ret);
}

static struct option long_options[] = {
	{ "help",	no_argument,		0,	'h'},
	{"hfs-root",	required_argument,	0,	'r'},
	{"site-id",	required_argument,	0,	's'},
	{"type",	required_argument,	0,	'p'},
	{"itemid",	required_argument,	0,	'i'},
	{"size",	required_argument,	0,	'x'},
	{"graph-from",	required_argument,	0,	'F'},
	{"graph-to",	required_argument,	0,	'T'},
	{"from",	required_argument,	0,	'f'},
	{"to",		required_argument,	0,	't'},
	{0, 0, 0, 0}
};

char *hfs_root = NULL, *site_id = NULL;
int item_type = 0;
long long itemid;
size_t sizex = 908;
time_t graph_from = 0, graph_to = 0, from = 0, to = 0;

int main(int argc, char **argv) {
	int i, c;
	size_t n;
	hfs_item_value_t *res = NULL;

	if (argc == 1)
		print_help(EXIT_SUCCESS);

        while ((c = getopt_long (argc, argv, "hr:s:p:i:x:F:T:f:t:", long_options, NULL)) != -1) {
		switch (c) {
			case 'h': print_help(EXIT_SUCCESS);
				break;
			case 'r': hfs_root = strdup(optarg);
				break;
			case 's': site_id = strdup(optarg);
				break;
			case 'p':
				if (optarg[0] == 'h' || optarg[0] == 'H')
					item_type = 0;
				else if (optarg[0] == 't' || optarg[0] == 'T')
					item_type = 1;
				break;
			case 'i': itemid = atoll(optarg);
				break;
			case 'x': sizex = atoi(optarg);
				break;
			case 'F': graph_from = atoi(optarg);
				break;
			case 'T': graph_to = atoi(optarg);
				break;
			case 'f': from = atoi(optarg);
				break;
			case 't': to = atoi(optarg);
				break;
		}
	}

	if (itemid <= 0) {
		fprintf(stderr, "Error: 'itemid' should be specified.\n");
		exit(EXIT_FAILURE);
	}
	if (sizex <= 0) {
		fprintf(stderr, "Error: 'sizex' should be specified.\n");
		exit(EXIT_FAILURE);
	}
	if (from <= 0) {
		fprintf(stderr, "Error: 'from' should be specified.\n");
		exit(EXIT_FAILURE);
	}
	if (to <= 0) {
		fprintf(stderr, "Error: 'to' should be specified.\n");
		exit(EXIT_FAILURE);
	}

	if (hfs_root == NULL)
		hfs_root = "/tmp/hfs";

	if (site_id == NULL)
	    site_id = "Default";

	if (graph_from == 0)
		graph_from = from;

	if (graph_to == 0)
		graph_to = to;

	printf("Run: HFSread_item(%s, %s, %d, %lld, %d, %d, %d, %d, %d, &res)\n",
		hfs_root, site_id, item_type, itemid, sizex,
		graph_from, graph_to, from, to);

	n = HFSread_item(hfs_root, site_id, item_type, itemid, sizex, graph_from, graph_to, from, to, &res);

	for (i=0; i<n; i++) {
		if (res[i].type == IT_DOUBLE ||
		    res[i].type == IT_TRENDS) {
			printf("res[%d] group=%d count=%d clock=%d max=%f min=%f avg=%f\n",
				i, res[i].group, res[i].count, res[i].clock, res[i].value.max.d, res[i].value.min.d, res[i].value.avg.d);
		}
		else if (res[i].type == IT_UINT64) {
			printf("res[%d] group=%d count=%d clock=%d max=%lld min=%lld avg=%lld\n",
				i, res[i].group, res[i].count, res[i].clock, res[i].value.max.l, res[i].value.min.l, res[i].value.avg.l);
		}
	}

	printf("results = %d\n", n);

	if (res)
		free(res);

	return EXIT_SUCCESS;
}
