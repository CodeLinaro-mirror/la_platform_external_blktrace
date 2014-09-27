/*
 * blktrace output analysis: generate a timeline & gather statistics
 *
 * (C) Copyright 2009 Hewlett-Packard Development Company, L.P.
 *	Alan D. Brunelle (alan.brunelle@hp.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "globals.h"

unsigned int window_sz = 0, step = 0;

struct files {
	FILE *fp;
	char *nm;
};

struct rstat {
	struct list_head head;
	struct files files[6];
	unsigned long long ios, nblks;
	unsigned long long rios, wios, rblks, wblks;
	double base_msec[3];
	long long unsigned int **tp_sliding_window;
	long long unsigned int **ios_sliding_window;
	long long unsigned int **zs_sliding_window;
};

static struct rstat *sys_info;
static LIST_HEAD(rstats);

static int do_open(struct files *fip, char *bn, char *pn)
{
	fip->nm = malloc(strlen(bn) + 16);
	sprintf(fip->nm, "%s_%s.dat", bn, pn);

	fip->fp = my_fopen(fip->nm, "w");
	if (fip->fp) {
		add_file(fip->fp, fip->nm);
		return 0;
	}

	free(fip->nm);
	return -1;
}

static int init_rsip(struct rstat *rsip, struct d_info *dip)
{
	char *nm = dip ? dip->dip_name : "sys";
	char fname[256];

	rsip->base_msec[0] = rsip->base_msec[1] = rsip->base_msec[2] = -1;
	rsip->ios = rsip->nblks = 0;
	rsip->rios = rsip->wios = rsip->rblks = rsip->wblks = 0;

	if (output_name)
		snprintf(fname, 255, "%s_%s", output_name, nm);
	else snprintf(fname, 255, "%s", nm);

	if (do_open(&rsip->files[0], fname, "iops") ||
		do_open(&rsip->files[1], fname, "mbps") ||
		do_open(&rsip->files[2], fname, "r_iops") ||
		do_open(&rsip->files[3], fname, "r_mbps") ||
		do_open(&rsip->files[4], fname, "w_iops") ||
		do_open(&rsip->files[5], fname, "w_mbps"))
		return -1;

	list_add_tail(&rsip->head, &rstats);
	return 0;
}

static void rstat_window_slide(struct rstat *rsip, unsigned int dir, double *mbps,
			       long long unsigned int *iops,
			       long long unsigned int cur_blk,
			       unsigned int cur_ios, unsigned int num_zeros)
{
	unsigned int num_bins = (unsigned int)(window_sz / step) - 1;
	unsigned int ios = 0, nblks = 0, i, zeros = 0;
	double resolution = MSEC_IN_SEC / (double)step;

	if (!mbps || !iops)
		return;

	/* add up stats and slide window over */
	for (i = 0; i < num_bins - 1; i++) {
		nblks += rsip->tp_sliding_window[dir][i];
		ios += rsip->ios_sliding_window[dir][i];
		zeros += rsip->zs_sliding_window[dir][i];

		rsip->tp_sliding_window[dir][i] = rsip->tp_sliding_window[dir][i + 1];
		rsip->ios_sliding_window[dir][i] = rsip->ios_sliding_window[dir][i + 1];
		rsip->zs_sliding_window[dir][i] = rsip->zs_sliding_window[dir][i+1];
	}

	nblks += rsip->tp_sliding_window[dir][num_bins - 1];
	ios += rsip->ios_sliding_window[dir][num_bins - 1];
	zeros += rsip->zs_sliding_window[dir][num_bins - 1];

	rsip->tp_sliding_window[dir][num_bins - 1] = cur_blk;
	rsip->ios_sliding_window[dir][num_bins - 1] = cur_ios;
	rsip->zs_sliding_window[dir][num_bins - 1] = num_zeros;
	nblks += cur_blk;
	ios += cur_ios;
	zeros += num_zeros;

	/* finally calculate the window-average stats */
	*mbps = (nblks * 512.0 * resolution) /
		((num_bins + 1 + zeros) * 1024.0 * 1024.0);
	*iops = (unsigned long long)((ios * resolution) /
				     (num_bins + 1 + zeros));
}

static void rstat_emit(struct rstat *rsip, double cur, int rw)
{
	double mbps, base_sec[3];
	int num_zeros, i;
	long long unsigned int iops;
	double delta, cur_sec;

	/* round base down to closest multiple of step */
	for (i = 0; i < 3; i++) {
		base_sec[i] = (unsigned int)(rsip->base_msec[i] -
					((unsigned int)rsip->base_msec[i] % step)) /
								(double)MSEC_IN_SEC;
	}
	cur_sec = (unsigned int)(TO_MSEC(cur) -
				((unsigned int)TO_MSEC(cur) % step)) /
							(double)MSEC_IN_SEC;

	delta = cur_sec - base_sec[0];
	num_zeros = (int)((delta * (MSEC_IN_SEC / step)) - 1);
	if (num_zeros < 0)
		num_zeros = 0;

	rstat_window_slide(rsip, 0, &mbps, &iops, (double)rsip->nblks,
				   (double)rsip->ios, num_zeros);
	/*
	 * Read/Write combined
	 */
	fprintf(rsip->files[0].fp, "%.3lf %llu\n", base_sec[0], iops);
	fprintf(rsip->files[1].fp, "%.3lf %.5lf\n", base_sec[0], mbps);

	/* Read/Write specific IOPS and MBPS */
	if (rw) {
		delta = cur_sec - base_sec[1];
		num_zeros = (int)((delta * (MSEC_IN_SEC / step)) - 1);
		if (num_zeros < 0)
			num_zeros = 0;

		rstat_window_slide(rsip, 1, &mbps, &iops, (double)rsip->nblks,
				   (double)rsip->ios, num_zeros);

		fprintf(rsip->files[2].fp, "%.3lf %llu\n", base_sec[1], iops);
		fprintf(rsip->files[3].fp, "%.3lf %.2lf\n", base_sec[1], mbps);
		rsip->rios = rsip->rblks = 0;
		rsip->base_msec[1] = TO_MSEC(cur);

	} else {
		delta = cur_sec - base_sec[2];
		num_zeros = (int)((delta * (MSEC_IN_SEC / step)) - 1);
		if (num_zeros < 0)
			num_zeros = 0;

		rstat_window_slide(rsip, 2, &mbps, &iops, (double)rsip->nblks,
				   (double)rsip->ios, num_zeros);
		fprintf(rsip->files[4].fp, "%.3lf %llu\n", base_sec[2], iops);
		fprintf(rsip->files[5].fp, "%.3lf %.2lf\n", base_sec[2], mbps);
		rsip->wios = rsip->wblks = 0;
		rsip->base_msec[2] = TO_MSEC(cur);
	}

	rsip->base_msec[0] = TO_MSEC(cur);
	rsip->ios = rsip->nblks = 0;
}

static void __add(struct rstat *rsip, double cur, unsigned long long nblks, int rw)
{
	if (rsip->base_msec[0] < 0)
		rsip->base_msec[0] = TO_MSEC(cur);
	else if ((TO_MSEC(cur) - rsip->base_msec[0]) >= (double)step)
		rstat_emit(rsip, cur, rw);

	if (rw) {
		rsip->rios++;
		rsip->rblks += nblks;
	} else {
		rsip->wios++;
		rsip->wblks += nblks;
	}
	rsip->ios++;
	rsip->nblks += nblks;
}

void *rstat_alloc(struct d_info *dip)
{
	struct rstat *rsip = malloc(sizeof(*rsip));
	unsigned int num_bins = (int)(window_sz / step) - 1;
	int i;

	if (!num_bins || num_bins > 1000) {
		/* something went wrong with the params, set default values */
		window_sz = 100;
		step = 10;
		num_bins = 9;
	}

	rsip->tp_sliding_window =
		(long long unsigned int **)malloc(3 * sizeof(long long unsigned int *));
	if (!rsip->tp_sliding_window)
		return NULL;
	for (i = 0; i < 3; i++) {
		rsip->tp_sliding_window[i] =
			(long long unsigned int *)malloc(num_bins * sizeof(long long unsigned int));
		memset(rsip->tp_sliding_window[i], 0,
		       num_bins * sizeof(long long unsigned int));
	}

	rsip->ios_sliding_window =
		(long long unsigned int **)malloc(3 * sizeof(long long unsigned int *));
	if (!rsip->ios_sliding_window) {
		free(rsip->tp_sliding_window);
		return NULL;
	}
	for (i = 0; i < 3; i++) {
		rsip->ios_sliding_window[i] =
		(long long unsigned int *)malloc(num_bins * sizeof(long long unsigned int));
		memset(rsip->ios_sliding_window[i], 0,
		       num_bins * sizeof(long long unsigned int));
	}

	rsip->zs_sliding_window =
		(long long unsigned int **)malloc(3 * sizeof(long long unsigned int *));
	if (!rsip->zs_sliding_window) {
		free(rsip->tp_sliding_window);
		free(rsip->ios_sliding_window);
		return NULL;
	}
	for (i = 0; i < 3; i++) {
		rsip->zs_sliding_window[i] =
			(long long unsigned int *)malloc(num_bins * sizeof(long long unsigned int));
		memset(rsip->zs_sliding_window[i], 0,
		       num_bins * sizeof(long long unsigned int));
	}

	if (!init_rsip(rsip, dip))
		return rsip;

	free(rsip);
	return NULL;
}

void rstat_free(void *ptr)
{
	struct rstat *rsip = ptr;

	rstat_emit(rsip, last_t_seen, 1);
	list_del(&rsip->head);
	free(rsip->tp_sliding_window);
	free(rsip->ios_sliding_window);
	free(rsip->zs_sliding_window);
	free(rsip);
}

void rstat_add(void *ptr, double cur, unsigned long long nblks, int rw)
{
	if (ptr != NULL)
		__add((struct rstat *)ptr, cur, nblks, rw);
	if (sys_info != NULL)
		__add(sys_info, cur, nblks, rw);
}

int rstat_init(void)
{
	sys_info = rstat_alloc(NULL);
	return sys_info != NULL;
}

void rstat_exit(void)
{
	struct list_head *p, *q;

	list_for_each_safe(p, q, &rstats) {
		struct rstat *rsip = list_entry(p, struct rstat, head);
		rstat_free(rsip);
	}
}
