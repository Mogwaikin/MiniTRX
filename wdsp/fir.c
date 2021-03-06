/*  fir.c

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

#define _CRT_SECURE_NO_WARNINGS
#include "comm.h"

float* fftcv_mults (int NM, float* c_impulse)
{
	float* mults        = (float *) malloc0 (NM * sizeof (complex));
	float* cfft_impulse = (float *) malloc0 (NM * sizeof (complex));
	fftwf_plan ptmp = fftwf_plan_dft_1d(NM, (fftwf_complex *) cfft_impulse,
			(fftwf_complex *) mults, FFTW_FORWARD, FFTW_PATIENT);
	memset (cfft_impulse, 0, NM * sizeof (complex));
	// store complex coefs right-justified in the buffer
	memcpy (&(cfft_impulse[NM - 2]), c_impulse, (NM / 2 + 1) * sizeof(complex));
	fftwf_execute (ptmp);
	fftwf_destroy_plan (ptmp);
	_aligned_free (cfft_impulse);
	return mults;
}

float* get_fsamp_window(int N, int wintype)
{
	int i;
	float arg0, arg1;
	float* window = (float *) malloc0 (N * sizeof(float));
	switch (wintype)
	{
	case 0:
		arg0 = 2.0 * PI / ((float)N - 1.0);
		for (i = 0; i < N; i++)
		{
			arg1 = cos(arg0 * (float)i);
			window[i]  =   +0.21747
				+ arg1 *  (-0.45325
				+ arg1 *  (+0.28256
				+ arg1 *  (-0.04672)));
		}
		break;
	case 1:
		arg0 = 2.0 * PI / ((float)N - 1.0);
		for (i = 0; i < N; ++i)
		{
			arg1 = cos(arg0 * (float)i);
			window[i]  =   +6.3964424114390378e-02
				+ arg1 *  (-2.3993864599352804e-01
				+ arg1 *  (+3.5015956323820469e-01
				+ arg1 *  (-2.4774111897080783e-01
				+ arg1 *  (+8.5438256055858031e-02
				+ arg1 *  (-1.2320203369293225e-02
				+ arg1 *  (+4.3778825791773474e-04))))));
		}
		break;
	default:
		for (i = 0; i < N; i++)
			window[i] = 1.0;
	}
	return window;
}

float* fir_fsamp_odd (int N, float* A, int rtype, float scale, int wintype)
{
	int i, j;
	int mid = (N - 1) / 2;
	float mag, phs;
	float* window;
	float *fcoef     = (float *) malloc0 (N * sizeof (complex));
	float *c_impulse = (float *) malloc0 (N * sizeof (complex));
	fftwf_plan ptmp = fftwf_plan_dft_1d(N, (fftwf_complex *)fcoef, (fftwf_complex *)c_impulse, FFTW_BACKWARD, FFTW_PATIENT);
	float local_scale = 1.0 / (float)N;
	for (i = 0; i <= mid; i++)
	{
		mag = A[i] * local_scale;
		phs = - (float)mid * TWOPI * (float)i / (float)N;
		fcoef[2 * i + 0] = mag * cos (phs);
		fcoef[2 * i + 1] = mag * sin (phs);
	}
	for (i = mid + 1, j = 0; i < N; i++, j++)
	{
		fcoef[2 * i + 0] = + fcoef[2 * (mid - j) + 0];
		fcoef[2 * i + 1] = - fcoef[2 * (mid - j) + 1];
	}
	fftwf_execute (ptmp);
	fftwf_destroy_plan (ptmp);
	_aligned_free (fcoef);
	window = get_fsamp_window(N, wintype);
	switch (rtype)
	{
	case 0:
		for (i = 0; i < N; i++)
			c_impulse[i] = scale * c_impulse[2 * i] * window[i];
		break;
	case 1:
		for (i = 0; i < N; i++)
		{
			c_impulse[2 * i + 0] *= scale * window[i];
			c_impulse[2 * i + 1] = 0.0;
		}
		break;
	}
	_aligned_free (window);
	return c_impulse;
}

float* fir_fsamp (int N, float* A, int rtype, float scale, int wintype)
{
	int n, i, j, k;
	float sum;
	float* window;
	float *c_impulse = (float *) malloc0 (N * sizeof (complex));

	if (N & 1)
	{
		int M = (N - 1) / 2;
		for (n = 0; n < M + 1; n++)
		{
			sum = 0.0;
			for (k = 1; k < M + 1; k++)
				sum += 2.0 * A[k] * cos(TWOPI * (n - M) * k / N);
			c_impulse[2 * n + 0] = (1.0 / N) * (A[0] + sum);
			c_impulse[2 * n + 1] = 0.0;
		}
		for (n = M + 1, j = 1; n < N; n++, j++)
		{
			c_impulse[2 * n + 0] = c_impulse[2 * (M - j) + 0];
			c_impulse[2 * n + 1] = 0.0;
		}
	}
	else
	{
		float M = (float)(N - 1) / 2.0;
		for (n = 0; n < N / 2; n++)
		{
			sum = 0.0;
			for (k = 1; k < N / 2; k++)
				sum += 2.0 * A[k] * cos(TWOPI * (n - M) * k / N);
			c_impulse[2 * n + 0] = (1.0 / N) * (A[0] + sum);
			c_impulse[2 * n + 1] = 0.0;
		}
		for (n = N / 2, j = 1; n < N; n++, j++)
		{
			c_impulse[2 * n + 0] = c_impulse[2 * (N / 2 - j) + 0];
			c_impulse[2 * n + 1] = 0.0;
		}
	}
	window = get_fsamp_window (N, wintype);
	switch (rtype)
	{
	case 0:
		for (i = 0; i < N; i++)
			c_impulse[i] = scale * c_impulse[2 * i] * window[i];
		break;
	case 1:
		for (i = 0; i < N; i += 2)
			c_impulse[i] *= scale * window[i];
		break;
	}
	_aligned_free (window);
	return c_impulse;
}

float* fir_bandpass (int N, float f_low, float f_high, float samplerate, int wintype, int rtype, float scale)
{
	float *c_impulse = (float *) malloc0 (N * sizeof (complex));
	float ft = (f_high - f_low) / (2.0 * samplerate);
	float ft_rad = TWOPI * ft;
	float w_osc = PI * (f_high + f_low) / samplerate;
	int i, j;
	float m = 0.5 * (float)(N - 1);
	float delta = PI / m;
	float cosphi;
	float posi, posj;
	float sinc, window, coef;

	if (N & 1)
	{
		switch (rtype)
		{
		case 0:
			c_impulse[N >> 1] = scale * 2.0 * ft;
			break;
		case 1:
			c_impulse[N - 1] = scale * 2.0 * ft;
			c_impulse[  N  ] = 0.0;
			break;
		}
	}
	for (i = (N + 1) / 2, j = N / 2 - 1; i < N; i++, j--)
	{
		posi = (float)i - m;
		posj = (float)j - m;
		sinc = sin (ft_rad * posi) / (PI * posi);
		switch (wintype)
		{
		case 0:	// Blackman-Harris 4-term
			cosphi = cos (delta * i);
			window  =             + 0.21747
					+ cosphi *  ( - 0.45325
					+ cosphi *  ( + 0.28256
					+ cosphi *  ( - 0.04672 )));
			break;
		case 1:	// Blackman-Harris 7-term
			cosphi = cos (delta * i);
			window	=			  + 6.3964424114390378e-02
					+ cosphi *  ( - 2.3993864599352804e-01
					+ cosphi *  ( + 3.5015956323820469e-01
					+ cosphi *	( - 2.4774111897080783e-01
					+ cosphi *  ( + 8.5438256055858031e-02
					+ cosphi *	( - 1.2320203369293225e-02
					+ cosphi *	( + 4.3778825791773474e-04 ))))));
			break;
		}
		coef = scale * sinc * window;
		switch (rtype)
		{
		case 0:
			c_impulse[i] = + coef * cos (posi * w_osc);
			c_impulse[j] = + coef * cos (posj * w_osc);
			break;
		case 1:
			c_impulse[2 * i + 0] = + coef * cos (posi * w_osc);
			c_impulse[2 * i + 1] = - coef * sin (posi * w_osc);
			c_impulse[2 * j + 0] = + coef * cos (posj * w_osc);
			c_impulse[2 * j + 1] = - coef * sin (posj * w_osc);
			break;
		}
	}
	return c_impulse;
}
