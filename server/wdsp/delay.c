/*  delay.c

This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2013 Warren Pratt, NR0V

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

The author can be reached by email at  

warren@wpratt.com

*/

#include "comm.h"

DELAY create_delay (int run, int size, float* in, float* out, int rate, float tdelta, float tdelay)
{
	DELAY a = (DELAY) malloc0 (sizeof (delay));
	a->run = run;
	a->size = size;
	a->in = in;
	a->out = out;
	a->rate = rate;
	a->tdelta = tdelta;
	a->tdelay = tdelay;
	a->L = (int)(0.5 + 1.0 / (a->tdelta * (float)a->rate));
	a->ft = 0.45 / (float)a->L;
	a->ncoef = (int)(60.0 / a->ft);
	a->ncoef = (a->ncoef / a->L + 1) * a->L;
	a->cpp = a->ncoef / a->L;
	a->phnum = (int)(0.5 + a->tdelay / a->tdelta);
	a->snum = a->phnum / a->L;
	a->phnum %= a->L;
	a->idx_in = 0;
	a->adelta = 1.0 / (a->rate * a->L);
	a->adelay = a->adelta * (a->snum * a->L + a->phnum);
	a->h = fir_bandpass (a->ncoef,-a->ft, +a->ft, 1.0, 1, 0, (float)a->L);	
	a->rsize = a->cpp + (WSDEL - 1);
	a->ring = (float *) malloc0 (a->rsize * sizeof (complex));
	return a;
}

void destroy_delay (DELAY a)
{
	free (a->ring);
	free (a->h);
	free (a);
}

void flush_delay (DELAY a)
{
	memset (a->ring, 0, a->cpp * sizeof (complex));
	a->idx_in = 0;
}

void xdelay (DELAY a)
{
	if (a->run)
	{
		int i, j, k, idx, n;
		float Itmp, Qtmp;
		for (i = 0; i < a->size; i++)
		{
			a->ring[2 * a->idx_in + 0] = a->in[2 * i + 0];
			a->ring[2 * a->idx_in + 1] = a->in[2 * i + 1];
			Itmp = 0.0;
			Qtmp = 0.0;
			if ((n = a->idx_in + a->snum) >= a->rsize) n -= a->rsize;
			for (j = 0, k = 0; j < a->cpp; j++, k += a->L)
			{
				if ((idx = n - j) < 0) idx += a->rsize;
				Itmp += a->ring[2 * idx + 0] * a->h[k + a->phnum];
				Qtmp += a->ring[2 * idx + 1] * a->h[k + a->phnum];
			}
			a->out[2 * i + 0] = Itmp;
			a->out[2 * i + 1] = Qtmp;
			if (--a->idx_in < 0) a->idx_in = a->rsize - 1;
		}
	}
	else if (a->out != a->in)
		memcpy (a->out, a->in, a->size * sizeof (complex));
}

/********************************************************************************************************
*																										*
*											  Properties												*
*																										*
********************************************************************************************************/

void SetDelayRun (DELAY a, int run)
{
	a->run = run;
}

float SetDelayValue (DELAY a, float tdelay)
{
	float adelay;
	a->tdelay = tdelay;
	a->phnum = (int)(0.5 + a->tdelay / a->tdelta);
	a->snum = a->phnum / a->L;
	a->phnum %= a->L;
	a->adelay = a->adelta * (a->snum * a->L + a->phnum);
	adelay = a->adelay;
	return adelay;
}

void SetDelayBuffs (DELAY a, int size, float* in, float* out)
{
	a->size = size;
	a->in = in;
	a->out = out;
}