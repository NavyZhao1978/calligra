/*
	FILE:		engine.h
	PURPOSE:	Defines the routines for the Paint Engine.
	AUTHOR:		Kevin Waite 
	VERSION:	1.00  (10-May-91)

Copyright 1991, 1992, 2002, 2003 Tunde Cockshott, Kevin Waite, David England. 

Contact David England d.england@livjm.ac.uk
School of Computing and Maths Sciences,
Liverpool John Moores University 
Liverpool L3 3AF
United Kingdom
Phone +44 151 231 2271

Wet and Sticky is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. Wet and Sticky is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with Wet and Sticky; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 

*/

extern void single_step();
/*  This routine defines the paint steps involved in the
    basic cycle of the painting engine.  */

extern brush_stroke();

extern float intensity_value( /* POINT */);

extern float new_intensity_value( /* POINT */);

extern void compute_shade_vectors();


