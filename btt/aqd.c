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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"

struct aqd_info {
	FILE *d_ofp;
	FILE *q_ofp;
	FILE *rq_ofp;
	FILE *wq_ofp;
	int na;		/* # active */
};

FILE *aqd_open(struct d_info *dip, char *name, char *post)
{
	FILE *fp = NULL;
	size_t tlen = strlen(name) + strlen(dip->dip_name)
					   + strlen(post) + 32;
	char oname[tlen];

	snprintf(oname, tlen-1, "%s_%s_%s.dat", name, dip->dip_name, post);
	fp = my_fopen(oname, "w");
	if (fp == NULL)
		perror(oname);
	else
		add_file(fp, strdup(oname));

	return fp;
}

void *aqd_alloc(struct d_info *dip)
{
	struct aqd_info *ap;

	if (aqd_name == NULL) return NULL;

	ap = malloc(sizeof(*ap));
	ap->na = 0;

	ap->d_ofp = aqd_open(dip, aqd_name, "d_aqd");
	ap->q_ofp = aqd_open(dip, aqd_name, "q_aqd");
	ap->rq_ofp = aqd_open(dip, aqd_name, "rq_aqd");
	ap->wq_ofp = aqd_open(dip, aqd_name, "wq_aqd");

	if (!ap->d_ofp || !ap->q_ofp || !ap->rq_ofp || !ap->wq_ofp) {
		free(ap);
		ap = NULL;
	}

	return ap;
}

void aqd_free(void *info)
{
	free(info);
}

void aqd_queue(struct d_info *dip, int rw)
{
	if (dip->aqd_handle) {
		struct aqd_info *ap = dip->aqd_handle;

		fprintf(ap->q_ofp, "%llu %llu\n", dip->n_qs, dip->n_act_q);
		if (rw)
			fprintf(ap->rq_ofp, "%llu %llu\n", dip->n_rqs, dip->n_act_q);
		else
			fprintf(ap->wq_ofp, "%llu %llu\n", dip->n_wqs, dip->n_act_q);
	}
}

void aqd_issue(void *info, double ts)
{
	if (info) {
		struct aqd_info *ap = info;

		fprintf(ap->d_ofp, "%lf %d\n%lf %d\n", ts, ap->na, ts, ap->na + 1);
		ap->na += 1;
	}
}

void aqd_complete(void *info, double ts)
{
	if (info) {
		struct aqd_info *ap = info;

		if (ap->na > 0) {
			fprintf(ap->d_ofp, "%lf %d\n%lf %d\n",
					ts, ap->na, ts, ap->na - 1);
			ap->na -= 1;
		}
	}
}
