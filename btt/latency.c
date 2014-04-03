/*
 * blktrace output analysis: generate a timeline & gather statistics
 *
 * Copyright (C) 2006 Alan D. Brunelle <Alan.Brunelle@hp.com>
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

#define LAT_FILE_TYPE_PAD_LEN	32
unsigned int min_idx[NUM_LAT_TYPES][N_DIRECTIONS] = {
	{N_LAT_HIST_BKTS, N_LAT_HIST_BKTS, N_LAT_HIST_BKTS},
	{N_LAT_HIST_BKTS, N_LAT_HIST_BKTS, N_LAT_HIST_BKTS},
	{N_LAT_HIST_BKTS, N_LAT_HIST_BKTS, N_LAT_HIST_BKTS}};
unsigned int max_idx[NUM_LAT_TYPES][N_DIRECTIONS] = {{0,0,0},{0,0,0},{0,0,0}};
__u64 total_lats[NUM_LAT_TYPES][N_DIRECTIONS] = {{0,0,0},{0,0,0},{0,0,0}};
char *type_str[NUM_LAT_TYPES] = {"q2d", "q2c", "d2c"};
char *dir_str[N_DIRECTIONS] = {"", "w_", "r_"};

static inline void latency_out(FILE *ofp, __u64 tstamp, __u64 latency)
{
	if (ofp)
		fprintf(ofp, "%lf %lf\n", TO_SEC(tstamp), TO_SEC(latency));
}

FILE *latency_open(struct d_info *dip, char *name, char *post)
{
	FILE *fp = NULL;

	if (name) {
		size_t tlen = strlen(name) + strlen(dip->dip_name)
				 + strlen(post) + LAT_FILE_TYPE_PAD_LEN;
		char oname[tlen];

		sprintf(oname, "%s_%s_%s.dat", name, dip->dip_name, post);
		if ((fp = my_fopen(oname, "w")) == NULL)
			perror(oname);
		else
			add_file(fp, strdup(oname));
	}

	return fp;
}

FILE *latency_hist_open(int type, int dir, int is_cumulative)
{
	FILE *fp = NULL;
	char *suffix_str;

	if (lat_hist_name) {
		suffix_str = is_cumulative ? "cumulative_lat" : "raw_lat_histog";
		size_t tlen = strlen(lat_hist_name) + strlen(type_str[type]) +
			strlen(dir_str[dir]) + strlen(suffix_str) +
			LAT_FILE_TYPE_PAD_LEN;
		char oname[tlen];

		snprintf(oname, tlen, "%s_%s%s_%s.dat", lat_hist_name,
			 dir_str[dir], type_str[type], suffix_str);

		if ((fp = my_fopen(oname, "w")) == NULL)
			perror(oname);
		else
			add_file(fp, strdup(oname));

		if (is_cumulative) {
			fprintf(fp, "%s %s Cumulative Latency - 100us latency bins\n",
				 dir_str[dir], type_str[type]);
		} else {
			fprintf(fp, "%s %s raw latency Histogram - 100us latency bins\n",
				 dir_str[dir], type_str[type]);
		}
	}

	return fp;
}

void latency_alloc(struct d_info *dip)
{
	dip->q2d_ofp	= latency_open(dip, q2d_name, "q2d");
	dip->q2d_rofp	= latency_open(dip, q2d_name, "rq2d");
	dip->q2d_wofp	= latency_open(dip, q2d_name, "wq2d");
	dip->d2c_ofp	= latency_open(dip, d2c_name, "d2c");
	dip->d2c_rofp	= latency_open(dip, d2c_name, "rd2c");
	dip->d2c_wofp	= latency_open(dip, d2c_name, "wd2c");
	dip->q2c_ofp	= latency_open(dip, q2c_name, "q2c");
	dip->q2c_rofp	= latency_open(dip, q2c_name, "rq2c");
	dip->q2c_wofp	= latency_open(dip, q2c_name, "wq2c");
}

void latency_hist_add(__u64 latency, enum latency_type type, int rw)
{
	__u64 u_latency = (__u64)(TO_MSEC(latency) * 1000.0);
	int dir = rw + 1;

	u_latency /= BKT_RESOLOTION_US;	/* buckets of 100us */

	if (u_latency > N_LAT_HIST_BKTS - 2)
		u_latency = N_LAT_HIST_BKTS - 1;

	lat_histos[type][dir][u_latency]++;

	if (u_latency > max_idx[type][dir])
		max_idx[type][dir] = u_latency;

	if (u_latency < min_idx[type][dir])
		min_idx[type][dir] = u_latency;

	total_lats[type][dir]++;

	/* read and write combined stats */
	lat_histos[type][0][u_latency]++;

	if (u_latency > max_idx[type][0])
		max_idx[type][0] = u_latency;

	if (u_latency < min_idx[type][0])
		min_idx[type][0] = u_latency;

	total_lats[type][0]++;

}

void latency_q2d(struct d_info *dip, __u64 tstamp, __u64 latency, int rw)
{
	plat_x2c(dip->q2d_plat_handle, tstamp, latency);
	latency_hist_add(latency, Q2D_LATENCY, rw);
	latency_out(dip->q2d_ofp, tstamp, latency);
	if (rw)
		latency_out(dip->q2d_rofp, tstamp, latency);
	else
		latency_out(dip->q2d_wofp, tstamp, latency);
}

void latency_d2c(struct d_info *dip, __u64 tstamp, __u64 latency, int rw)
{
	plat_x2c(dip->d2c_plat_handle, tstamp, latency);
	latency_hist_add(latency, D2C_LATENCY, rw);
	latency_out(dip->d2c_ofp, tstamp, latency);
	if (rw)
		latency_out(dip->d2c_rofp, tstamp, latency);
	else
		latency_out(dip->d2c_wofp, tstamp, latency);
}

void latency_q2c(struct d_info *dip, __u64 tstamp, __u64 latency, int rw)
{
	plat_x2c(dip->q2c_plat_handle, tstamp, latency);
	latency_hist_add(latency, Q2C_LATENCY, rw);
	latency_out(dip->q2c_ofp, tstamp, latency);
	if (rw)
		latency_out(dip->q2c_rofp, tstamp, latency);
	else
		latency_out(dip->q2c_wofp, tstamp, latency);
}

void latency_build_cumulative()
{
	unsigned int type, j, dir;

	for (type = 0; type < NUM_LAT_TYPES; type++) {
		for (dir = 0; dir < N_DIRECTIONS; dir++) {
			j = min_idx[type][dir];
			cumulative_lat[type][dir][j] = lat_histos[type][dir][j] /
				(double)total_lats[type][dir];
			for (j = min_idx[type][dir] + 1; j <= max_idx[type][dir]; j++)
				cumulative_lat[type][dir][j] +=
					(cumulative_lat[type][dir][j - 1] +
					(lat_histos[type][dir][j] /
					(double)total_lats[type][dir]));
		}
	}
}

void output_latency_hists(void)
{
	unsigned int i, type, dir;

	latency_build_cumulative();

	for (type = 0; type < NUM_LAT_TYPES; type++)
		for (dir = 0; dir < N_DIRECTIONS; dir++) {
			lat_hist_ofs[type][dir] = latency_hist_open(type, dir, 0);
			cum_lat_ofs[type][dir] = latency_hist_open(type, dir, 1);
			if (min_idx[type][dir] > 0) {
				fprintf(cum_lat_ofs[type][dir], "%d %lf\n",
					min_idx[type][dir] - 1, 0.0);
			}
			for (i = min_idx[type][dir]; i <= max_idx[type][dir]; i++) {
				fprintf(lat_hist_ofs[type][dir], "%d %llu\n", i,
					lat_histos[type][dir][i]);
				fprintf(cum_lat_ofs[type][dir], "%d %lf\n", i,
					cumulative_lat[type][dir][i]);
			}
			fclose(lat_hist_ofs[type][dir]);
			fclose(cum_lat_ofs[type][dir]);
		}
}
