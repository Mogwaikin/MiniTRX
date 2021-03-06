/*  ammod.h

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

#ifndef _ammod_h
#define _ammod_h

typedef struct _ammod
{
	int run;
	int mode;
	int size;
	float* in;
	float* out;
	float c_level;
	float a_level;
	float mult;
}ammod, *AMMOD;

extern AMMOD create_ammod (int run, int mode, int size, float* in, float* out, float c_level);

extern void destroy_ammod (AMMOD a);

extern void flush_ammod (AMMOD a);

extern void xammod (AMMOD a);

// TXA Properties

extern __declspec (dllexport) void SetTXAAMCarrierLevel (int channel, float c_level);

#endif