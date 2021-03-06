/*  iir.c

This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2014 Warren Pratt, NR0V

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

/********************************************************************************************************
*																										*
*											Bi-Quad Notch												*
*																										*
********************************************************************************************************/

void calc_snotch (SNOTCH a)
{
	float fn, qk, qr, csn;
	fn = a->f / (float)a->rate;
	csn = cos (TWOPI * fn);
	qr = 1.0 - 3.0 * a->bw;
	qk = (1.0 - 2.0 * qr * csn + qr * qr) / (2.0 * (1.0 - csn));
	a->a0 = + qk;
	a->a1 = - 2.0 * qk * csn;
	a->a2 = + qk;
	a->b1 = + 2.0 * qr * csn;
	a->b2 = - qr * qr;
}

SNOTCH create_snotch (int run, int size, float* in, float* out, int rate, float f, float bw)
{
	SNOTCH a = (SNOTCH) malloc0 (sizeof (snotch));
	a->run = run;
	a->size = size;
	a->in = in;
	a->out = out;
	a->rate = rate;
	a->f = f;
	a->bw = bw;
	InitializeCriticalSectionAndSpinCount ( &a->cs_update, 2500 );
	calc_snotch (a);
	return a;
}

void destroy_snotch (SNOTCH a)
{
	DeleteCriticalSection (&a->cs_update);
	_aligned_free (a);
}

void flush_snotch (SNOTCH a)
{
	a->x1 = a->x2 = a->y1 = a->y2 = 0.0;
}

void xsnotch (SNOTCH a)
{
	EnterCriticalSection (&a->cs_update);
	if (a->run)
	{
		int i;
		for (i = 0; i < a->size; i++)
		{
			a->x0 = a->in[2 * i + 0];
			a->out[2 * i + 0] = a->a0 * a->x0 + a->a1 * a->x1 + a->a2 * a->x2 + a->b1 * a->y1 + a->b2 * a->y2;
			a->y2 = a->y1;
			a->y1 = a->out[2 * i + 0];
			a->x2 = a->x1;
			a->x1 = a->x0;
		}
	}
	else if (a->out != a->in)
		memcpy (a->out, a->in, a->size * sizeof (complex));
	LeaveCriticalSection (&a->cs_update);
}

/********************************************************************************************************
*																										*
*											RXA Properties												*
*																										*
********************************************************************************************************/

void SetSNCTCSSFreq (SNOTCH a, float freq)
{
	EnterCriticalSection (&a->cs_update);
	a->f = freq;
	calc_snotch (a);
	LeaveCriticalSection (&a->cs_update);
}

void SetSNCTCSSRun (SNOTCH a, int run)
{
	EnterCriticalSection (&a->cs_update);
	a->run = run;
	LeaveCriticalSection (&a->cs_update);
}


/********************************************************************************************************
*																										*
*											Complex Bi-Quad Peaking										*
*																										*
********************************************************************************************************/

void calc_speak (SPEAK a)
{
	float ratio;	
	float f_corr, g_corr, bw_corr, bw_parm, A, f_min;
	
	switch (a->design)
	{
	case 0:
		ratio = a->bw / a->f;
		switch (a->nstages)
		{
		case 4:
			bw_parm = 2.4;
			f_corr  = 1.0 - 0.160 * ratio + 1.440 * ratio * ratio;
			g_corr = 1.0 - 1.003 * ratio + 3.990 * ratio * ratio;
			break;
		default:
			bw_parm = 1.0;
			f_corr  = 1.0;
			g_corr = 1.0;
			break;
		}
		{
			float fn, qk, qr, csn;
			a->fgain = a->gain / g_corr;
			fn = a->f / (float)a->rate / f_corr;
			csn = cos (TWOPI * fn);
			qr = 1.0 - 3.0 * a->bw / (float)a->rate * bw_parm;
			qk = (1.0 - 2.0 * qr * csn + qr * qr) / (2.0 * (1.0 - csn));
			a->a0 = 1.0 - qk;
			a->a1 = 2.0 * (qk - qr) * csn;
			a->a2 = qr * qr - qk;
			a->b1 = 2.0 * qr * csn;
			a->b2 = - qr * qr;
		}
		break;

	case 1:
		if (a->f < 200.0) a->f = 200.0;
		ratio = a->bw / a->f;
		switch (a->nstages)
		{
		case 4:
			bw_parm = 5.0;
			bw_corr = 1.13 * ratio - 0.956 * ratio * ratio;
			A = 2.5;
			f_min = 50.0;
			break;
		default:
			bw_parm = 1.0;
			bw_corr  = 1.0;
			g_corr = 1.0;
			A = 2.5;
			f_min = 50.0;
			break;
		}
		{
			float w0, sn, c, den;
			if (a->f < f_min) a->f = f_min;
			w0 = TWOPI * a->f / (float)a->rate;
			sn = sin (w0);
			a->cbw = bw_corr * a->f;
			c = sn * sinh(0.5 * log((a->f + 0.5 * a->cbw * bw_parm) / (a->f - 0.5 * a->cbw * bw_parm)) * w0 / sn);
			den = 1.0 + c / A;
			a->a0 = (1.0 + c * A) / den;
			a->a1 = - 2.0 * cos (w0) / den;
			a->a2 = (1 - c * A) / den;
			a->b1 = - a->a1;
			a->b2 = - (1 - c / A ) / den;
			a->fgain = a->gain / pow (A * A, (float)a->nstages);
		}
		break;
	}
}

SPEAK create_speak (int run, int size, float* in, float* out, int rate, float f, float bw, float gain, int nstages, int design)
{
	SPEAK a = (SPEAK) malloc0 (sizeof (speak));
	a->run = run;
	a->size = size;
	a->in = in;
	a->out = out;
	a->rate = rate;
	a->f = f;
	a->bw = bw;
	a->gain = gain;
	a->nstages = nstages;
	a->design = design;
	a->x0 = (float *) malloc0 (a->nstages * sizeof (complex));
	a->x1 = (float *) malloc0 (a->nstages * sizeof (complex));
	a->x2 = (float *) malloc0 (a->nstages * sizeof (complex));
	a->y0 = (float *) malloc0 (a->nstages * sizeof (complex));
	a->y1 = (float *) malloc0 (a->nstages * sizeof (complex));
	a->y2 = (float *) malloc0 (a->nstages * sizeof (complex));
	InitializeCriticalSectionAndSpinCount ( &a->cs_update, 2500 );
	calc_speak (a);
	return a;
}

void destroy_speak (SPEAK a)
{
	DeleteCriticalSection (&a->cs_update);
	_aligned_free (a->y2);
	_aligned_free (a->y1);
	_aligned_free (a->y0);
	_aligned_free (a->x2);
	_aligned_free (a->x1);
	_aligned_free (a->x0);
	_aligned_free (a);
}

void flush_speak (SPEAK a)
{
	int i;
	for (i = 0; i < a->nstages; i++)
	{
		a->x1[2 * i + 0] = a->x2[2 * i + 0] = a->y1[2 * i + 0] = a->y2[2 * i + 0] = 0.0;
		a->x1[2 * i + 1] = a->x2[2 * i + 1] = a->y1[2 * i + 1] = a->y2[2 * i + 1] = 0.0;
	}
}

void xspeak (SPEAK a)
{
	EnterCriticalSection (&a->cs_update);
	if (a->run)
	{
		int i, j, n;
		for (i = 0; i < a->size; i++)
		{
			for (j = 0; j < 2; j++)
			{
				a->x0[j] = a->fgain * a->in[2 * i + j];
				for (n = 0; n < a->nstages; n++)
				{
					if (n > 0) a->x0[2 * n + j] = a->y0[2 * (n - 1) + j];
					a->y0[2 * n + j]	= a->a0 * a->x0[2 * n + j] 
										+ a->a1 * a->x1[2 * n + j] 
										+ a->a2 * a->x2[2 * n + j] 
										+ a->b1 * a->y1[2 * n + j] 
										+ a->b2 * a->y2[2 * n + j];
					a->y2[2 * n + j] = a->y1[2 * n + j];
					a->y1[2 * n + j] = a->y0[2 * n + j];
					a->x2[2 * n + j] = a->x1[2 * n + j];
					a->x1[2 * n + j] = a->x0[2 * n + j];
				}
				a->out[2 * i + j] = a->y0[2 * (a->nstages - 1) + j];
			}
		}
	}
	else if (a->out != a->in)
		memcpy (a->out, a->in, a->size * sizeof (complex));
	LeaveCriticalSection (&a->cs_update);
}

/********************************************************************************************************
*																										*
*											RXA Properties												*
*																										*
********************************************************************************************************/

PORT
void SetRXASPCWRun (int channel, int run)
{
	SPEAK a = rxa[channel].speak.p;
	EnterCriticalSection (&a->cs_update);
	a->run = run;
	LeaveCriticalSection (&a->cs_update);
}

PORT
void SetRXASPCWFreq (int channel, float freq)
{
	SPEAK a = rxa[channel].speak.p;
	EnterCriticalSection (&a->cs_update);
	a->f = freq;
	calc_speak (a);
	LeaveCriticalSection (&a->cs_update);
}

PORT
void SetRXASPCWBandwidth (int channel, float bw)
{
	SPEAK a = rxa[channel].speak.p;
	EnterCriticalSection (&a->cs_update);
	a->bw = bw;
	calc_speak (a);
	LeaveCriticalSection (&a->cs_update);
}

PORT
void SetRXASPCWGain (int channel, float gain)
{
	SPEAK a = rxa[channel].speak.p;
	EnterCriticalSection (&a->cs_update);
	a->gain = gain;
	calc_speak (a);
	LeaveCriticalSection (&a->cs_update);
}

/********************************************************************************************************
*																										*
*										Complex Multiple Peaking										*
*																										*
********************************************************************************************************/

MPEAK create_mpeak (int run, int size, float* in, float* out, int rate, int npeaks, int* enable, float* f, float* bw, float* gain, int nstages)
{
	int i;
	MPEAK a = (MPEAK) malloc0 (sizeof (mpeak));
	a->run = run;
	a->size = size;
	a->in = in;
	a->out = out;
	a->rate = rate;
	a->npeaks = npeaks;
	a->nstages = nstages;
	a->enable  = (int *) malloc0 (a->npeaks * sizeof (int));
	a->f    = (float *) malloc0 (a->npeaks * sizeof (float));
	a->bw   = (float *) malloc0 (a->npeaks * sizeof (float));
	a->gain = (float *) malloc0 (a->npeaks * sizeof (float));
	memcpy (a->enable, enable, a->npeaks * sizeof (int));
	memcpy (a->f, f, a->npeaks * sizeof (float));
	memcpy (a->bw, bw, a->npeaks * sizeof (float));
	memcpy (a->gain, gain, a->npeaks * sizeof (float));
	a->tmp = (float *) malloc0 (a->size * sizeof (complex));
	a->mix = (float *) malloc0 (a->size * sizeof (complex));
	a->pfil = (SPEAK *) malloc0 (a->npeaks * sizeof (SPEAK));
	for (i = 0; i < a->npeaks; i++)
	{
		a->pfil[i] = create_speak (	1, 
									a->size, 
									a->in, 
									a->tmp, 
									a->rate, 
									a->f[i], 
									a->bw[i], 
									a->gain[i], 
									a->nstages, 
									1 );
	}
	InitializeCriticalSectionAndSpinCount ( &a->cs_update, 2500 );
	return a;
}

void destroy_mpeak (MPEAK a)
{
	DeleteCriticalSection (&a->cs_update);
	_aligned_free (a->pfil);
	_aligned_free (a->mix);
	_aligned_free (a->tmp);
	_aligned_free (a->gain);
	_aligned_free (a->bw);
	_aligned_free (a->f);
	_aligned_free (a->enable);
	_aligned_free (a);
}

void flush_mpeak (MPEAK a)
{
	int i;
	for (i = 0; i < a->npeaks; i++)
		flush_speak (a->pfil[i]);
}

void xmpeak (MPEAK a)
{
	EnterCriticalSection (&a->cs_update);
	if (a->run)
	{
		int i, j;
		memset (a->mix, 0, a->size * sizeof (complex));
		for (i = 0; i < a->npeaks; i++)
		{
			if (a->enable[i])
			{
				xspeak (a->pfil[i]);
				for (j = 0; j < 2 * a->size; j++)
					a->mix[j] += a->tmp[j];
			}
		}
		memcpy (a->out, a->mix, a->size * sizeof (complex));
	}
	else if (a->in != a->out)
		memcpy (a->out, a->in, a->size * sizeof (complex));
	LeaveCriticalSection (&a->cs_update);
}

/********************************************************************************************************
*																										*
*											RXA Properties												*
*																										*
********************************************************************************************************/

PORT
void SetRXAmpeakRun (int channel, int run)
{
	MPEAK a = rxa[channel].mpeak.p;
	EnterCriticalSection (&a->cs_update);
	a->run = run;
	LeaveCriticalSection (&a->cs_update);
}

PORT
void SetRXAmpeakNpeaks (int channel, int npeaks)
{
	MPEAK a = rxa[channel].mpeak.p;
	EnterCriticalSection (&a->cs_update);
	a->npeaks = npeaks;
	LeaveCriticalSection (&a->cs_update);
}

PORT
void SetRXAmpeakFilEnable (int channel, int fil, int enable)
{
	MPEAK a = rxa[channel].mpeak.p;
	EnterCriticalSection (&a->cs_update);
	a->enable[fil] = enable;
	LeaveCriticalSection (&a->cs_update);
}

PORT
void SetRXAmpeakFilFreq (int channel, int fil, float freq)
{
	MPEAK a = rxa[channel].mpeak.p;
	EnterCriticalSection (&a->cs_update);
	a->f[fil] = freq;
	a->pfil[fil]->f = freq;
	calc_speak(a->pfil[fil]);
	LeaveCriticalSection (&a->cs_update);
}

PORT
void SetRXAmpeakFilBw (int channel, int fil, float bw)
{
	MPEAK a = rxa[channel].mpeak.p;
	EnterCriticalSection (&a->cs_update);
	a->bw[fil] = bw;
	a->pfil[fil]->bw = bw;
	calc_speak(a->pfil[fil]);
	LeaveCriticalSection (&a->cs_update);
}

PORT
void SetRXAmpeakFilGain (int channel, int fil, float gain)
{
	MPEAK a = rxa[channel].mpeak.p;
	EnterCriticalSection (&a->cs_update);
	a->gain[fil] = gain;
	a->pfil[fil]->gain = gain;
	calc_speak(a->pfil[fil]);
	LeaveCriticalSection (&a->cs_update);
}
