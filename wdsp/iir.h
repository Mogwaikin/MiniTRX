/*  iir.h

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

/********************************************************************************************************
*																										*
*											Bi-Quad Notch												*
*																										*
********************************************************************************************************/

#ifndef _snotch_h
#define _snotch_h

typedef struct _snotch
{
	int run;
	int size;
	float* in;
	float* out;
	float rate;
	float f;
	float bw;
	float a0, a1, a2, b1, b2;
	float x0, x1, x2, y1, y2;
	CRITICAL_SECTION cs_update;
} snotch, *SNOTCH;

extern SNOTCH create_snotch (int run, int size, float* in, float* out, int rate, float f, float bw);

extern void destroy_snotch (SNOTCH a);

extern void flush_snotch (SNOTCH a);

extern void xsnotch (SNOTCH a);

extern void SetSNCTCSSFreq (SNOTCH a, float freq);

extern void SetSNCTCSSRun (SNOTCH a, int run);

#endif

/********************************************************************************************************
*																										*
*											Complex Bi-Quad Peaking										*
*																										*
********************************************************************************************************/

#ifndef _speak_h
#define _speak_h

typedef struct _speak
{
	int run;
	int size;
	float* in;
	float* out;
	float rate;
	float f;
	float bw;
	float cbw;
	float gain;
	float fgain;
	int nstages;
	int design;
	float a0, a1, a2, b1, b2;
	float *x0, *x1, *x2, *y0, *y1, *y2;
	CRITICAL_SECTION cs_update;
} speak, *SPEAK;

extern SPEAK create_speak (int run, int size, float* in, float* out, int rate, float f, float bw, float gain, int nstages, int design);

extern void destroy_speak (SPEAK a);

extern void flush_speak (SPEAK a);

extern void xspeak (SPEAK a);

#endif

/********************************************************************************************************
*																										*
*										Complex Multiple Peaking										*
*																										*
********************************************************************************************************/

#ifndef _mpeak_h
#define _mpeak_h

typedef struct _mpeak
{
	int run;
	int size;
	float* in;
	float* out;
	int rate;
	int npeaks;
	int* enable;
	float* f;
	float* bw;
	float* gain;
	int nstages;
	SPEAK* pfil;
	float* tmp;
	float* mix;
	CRITICAL_SECTION cs_update;
} mpeak, *MPEAK;

extern MPEAK create_mpeak (int run, int size, float* in, float* out, int rate, int npeaks, int* enable, float* f, float* bw, float* gain, int nstages);

extern void destroy_mpeak (MPEAK a);

extern void flush_mpeak (MPEAK a);

extern void xmpeak (MPEAK a);

#endif