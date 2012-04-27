/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "mcubes.hpp"
#include "base/system.hpp"

/****************************************************************************/
/****************************************************************************/
/*
static const int edgeTable[256]={
0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   };
*/
static const int triTableBase[256][16] =
{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

#define s MarchingCubes::CellSize
const static sInt trans[12] =
{
  0*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 0*(s+1) + 0,
  1*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 0*(s+1) + 1,
  0*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 1*(s+1) + 0,
  1*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 0*(s+1) + 0,

  0*(s+1)*(s+1)*(s+1) + 1*(s+1)*(s+1) + 0*(s+1) + 0,
  1*(s+1)*(s+1)*(s+1) + 1*(s+1)*(s+1) + 0*(s+1) + 1,
  0*(s+1)*(s+1)*(s+1) + 1*(s+1)*(s+1) + 1*(s+1) + 0,
  1*(s+1)*(s+1)*(s+1) + 1*(s+1)*(s+1) + 0*(s+1) + 0,

  2*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 0*(s+1) + 0,
  2*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 0*(s+1) + 1,
  2*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 1*(s+1) + 1,
  2*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 1*(s+1) + 0,
};
#undef s

static const sInt table[27][4] =
{
  { 0x07,-1,-1,-1 },
  { 0x06,-1,-1, 0 },
  { 0x16,-1,-1, 1 },

  { 0x05,-1, 0,-1 },
  { 0x04,-1, 0, 0 },
  { 0x14,-1, 0, 1 },

  { 0x25,-1, 1,-1 },
  { 0x24,-1, 1, 0 },
  { 0x34,-1, 1, 1 },


  { 0x03, 0,-1,-1 },
  { 0x02, 0,-1, 0 },
  { 0x12, 0,-1, 1 },

  { 0x01, 0, 0,-1 },
  { 0x00, 0, 0, 0 },
  { 0x10, 0, 0, 1 },

  { 0x21, 0, 1,-1 },
  { 0x20, 0, 1, 0 },
  { 0x30, 0, 1, 1 },



  { 0x43, 1,-1,-1 },
  { 0x42, 1,-1, 0 },
  { 0x52, 1,-1, 1 },

  { 0x41, 1, 0,-1 },
  { 0x40, 1, 0, 0 },
  { 0x50, 1, 0, 1 },

  { 0x61, 1, 1,-1 },
  { 0x60, 1, 1, 0 },
  { 0x70, 1, 1, 1 },
};

/****************************************************************************/
/****************************************************************************/

MarchingCubes::Container::Container()
{
  Count = 0;
  Next = 0;
}

/****************************************************************************/

MarchingCubes::MarchingCubes(sInt vertices)
{
  Influence = 4;
  IsoValue = 0.25f;
  Scale = 0.5f;

  FreeContainers = 0;
  sClear(Grid);
  ActiveCellsDB = 0;

  for(sInt i=0;i<256;i++)
  {
    sInt n = 0;
    while(triTableBase[i][n]!=-1) n++;
    MyTriTable[i][0] = n;
    for(sInt j=0;j<n;j++)
      MyTriTable[i][j+1] = trans[triTableBase[i][j]];
  }

  if(0)
  {
    for(sInt i=0;i<256;i++)
    {
      sInt n = 0;
      sInt ii = (i & 0x33) | ((i & 0x88)>>1) | ((i & 0x44)<<1);
      while(triTableBase[ii][n]!=-1) n++;
      sDPrintF(L"  { %2d , ",n);
      for(sInt j=0;j<15;j++)
        sDPrintF(L"%2d,",triTableBase[ii][j]);
      sDPrintF(L" },\n");
    }
  }

  sInt vpb = 64*1024;

  sInt n = sMax(16,vertices/vpb+8);
  GeoBuffers.AddMany(n);
  for(sInt i=0;i<n;i++)
  {
    GeoBuffer *gb = &GeoBuffers[i];
    gb->Geo = new sGeometry(sGF_INDEX16|sGF_TRILIST,sVertexFormatStandard);
    gb->VertexMax = vpb;
    gb->IndexMax = vpb*4;
    gb->Vertex = 0;
    gb->Index = 0;
  }
  GeoBufferLock = new sThreadLock;
  GeoBufferUseIndex = 0;

  MaxThreads = 0;
  ThreadBuffers = 0;
}


MarchingCubes::~MarchingCubes()
{
  Recycle();
  ActiveCellsDB = 1-ActiveCellsDB;
  Recycle();
  Container *c = FreeContainers;
  while(c)
  {
    Container *n = c->Next;
    delete c;
    c = n;
  }

  GeoBuffer *gb;
  sFORALL(GeoBuffers,gb)
    delete gb->Geo;
  delete GeoBufferLock;

  delete[] ThreadBuffers;
}

/****************************************************************************/

void MarchingCubes::Recycle()
{
  Container *c;
  sFORALL(ActiveCells[ActiveCellsDB],c)
  {
    while(c)
    {
      Container *oc = c->Next;
      c->Next = FreeContainers;
      FreeContainers = c;
      c = oc;
    }
  }
  ActiveCells[ActiveCellsDB].Clear();
}

MarchingCubes::Container *MarchingCubes::GetContainer()
{
  Container *c = FreeContainers;
  if(c)
  {
    FreeContainers = c->Next;
    c->Next = 0;
    c->Count = 0;
  }
  else
  {
    c = new Container;
  }
  return c;
}

MarchingCubes::GeoBuffer *MarchingCubes::GetGeoBuffer()
{
  GeoBuffer *gb = 0; 
  sU32 n = sAtomicInc(&GeoBufferUseIndex)-1;
  if(sInt(n)<GeoBuffers.GetCount())
    gb = &GeoBuffers[n];
  return gb;
}

void MCTask(sStsManager *,sStsThread *thread,sInt start,sInt count,void *data)
{
  sInt ti = thread->GetIndex();
  sSchedMon->Begin(ti,0xffff00);
  MarchingCubes *mc = (MarchingCubes *) data;
  for(sInt i=0;i<count;i++)
    mc->MCTask(start+i,ti);
  sSchedMon->End(ti);
}

void MarchingCubes::Begin(sStsManager *sched,sStsWorkload *wl,sInt granularity)
{
  ActiveCellsDB = 1-ActiveCellsDB;

  // lock geometry

  GeoBuffer *gb;
  sFORALL(GeoBuffers,gb)
  {
    gb->Vertex = 0;
    gb->Index = 0;
    gb->Geo->BeginLoadVB(gb->VertexMax,sGD_FRAME,&gb->VP);
    gb->Geo->BeginLoadIB(gb->IndexMax,sGD_FRAME,&gb->IP);
  }
  GeoBufferUseIndex = 0;

  // prepare scheduling

  if(sched && sched->GetThreadCount() != MaxThreads)
  {
    delete[] ThreadBuffers;
    MaxThreads = sched->GetThreadCount();
    ThreadBuffers = new GeoBuffer *[MaxThreads];
  }

  // schdedule

  if(sched)
  {
    for(sInt i=0;i<MaxThreads;i++)
      ThreadBuffers[i] = 0;

    if(ActiveCells[ActiveCellsDB].GetCount()>0)
    {
      sInt n = ActiveCells[ActiveCellsDB].GetCount();
      sStsTask *task = wl->NewTask(::MCTask,this,n,0);
      task->Granularity = granularity;
      wl->AddTask(task);
    }
  }
  else
  {
    GeoBuffer *gb = GetGeoBuffer();
    const sInt maxvertex = gb->VertexMax - MaxVertex;
    const sInt maxindex = gb->IndexMax - MaxIndex;
    Container *c;
    sFORALL(ActiveCells[ActiveCellsDB],c)
    {
      if(gb->Vertex>=maxvertex || gb->Index>=maxindex)
      {
        gb = GetGeoBuffer();
        if(gb==0)
          goto out;
      }
      RenderCell(gb,c);
    }
  }
out:;
}

void MarchingCubes::Render(sVector31 *parts,sInt pn)
{
  // distribute particles to buckets

  sClear(Grid);
  const sF32 e = Influence/CellSize;
  sVector30 gspos(1.0f/CellSize/Scale,1.0f/CellSize/Scale,1.0f/CellSize/Scale);
  sVector31 gtpos((GridSize)*0.5f,(GridSize)*0.5f,(GridSize)*0.5f);
  for(sInt i=0;i<pn;i++)
  {
    sVector31 pos = sVector30(parts[i])*gspos+gtpos;
    sInt x = sInt(pos.x);
    sInt y = sInt(pos.y);
    sInt z = sInt(pos.z);
    if(x>=1 && y>=1 && z>=1 && x<GridSize-1 && y<GridSize-1 && z<GridSize-1)
    {
      sF32 fx = pos.x-x;
      sF32 fy = pos.y-y;
      sF32 fz = pos.z-z;
      sInt bits = 0;
      if(fx<e) bits |= 0x01;
      if(fy<e) bits |= 0x02;
      if(fz<e) bits |= 0x04;
      if(fx>1-e) bits |= 0x10;
      if(fy>1-e) bits |= 0x20;
      if(fz>1-e) bits |= 0x40;

      for(sInt t=0;t<27;t++)
      {
        if((bits & table[t][0])==table[t][0])
        {
          Container **g = &Grid[z+table[t][1]][y+table[t][2]][x+table[t][3]];
          Container *c = *g;
          if(c==0 || c->Count==ContainerSize)
          {
            Container *oc = c;
            c = GetContainer();
            c->Next = oc;
            *g = c;
          }
          c->Parts[c->Count++] = parts[i];
        }
      }
    }
  }

  // collect buckets
    
  sVector30 spos(CellSize,CellSize,CellSize);
  sVector31 tpos(-CellSize*(GridSize)*0.5f,-CellSize*(GridSize)*0.5f,-CellSize*(GridSize)*0.5f);
  sVERIFY(ActiveCells[1-ActiveCellsDB].IsEmpty());
  for(sInt z=0;z<GridSize;z++)
  {
    for(sInt y=0;y<GridSize;y++)
    {
      for(sInt x=0;x<GridSize;x++)
      {
        Container *c = Grid[z][y][x];
        Grid[z][y][x] = 0;
        if(c)
        {
          sVector30 v(x,y,z);
          c->Pos = v*spos+tpos;
          ActiveCells[1-ActiveCellsDB].AddTail(c);
        }
      }
    }
  }
}

void MarchingCubes::MCTask(sInt container,sInt thread)
{
  GeoBuffer *gb = ThreadBuffers[thread];
  if(gb==0 || gb->Vertex >= gb->VertexMax-MaxVertex || gb->Index >= gb->IndexMax-MaxIndex)
    gb = ThreadBuffers[thread] = GetGeoBuffer();
  if(gb)
  {
    Container *c = ActiveCells[ActiveCellsDB][container];
    RenderCell(gb,c);
  }
}


void MarchingCubes::End()
{
  Recycle();
  GeoBuffer *gb;
  sFORALL(GeoBuffers,gb)
  {
    gb->Geo->EndLoadIB(gb->Index);
    gb->Geo->EndLoadVB(gb->Vertex);
  }
}

void MarchingCubes::Draw()
{
  GeoBuffer *gb;
  sFORALL(GeoBuffers,gb)
  {
    if(gb->Vertex>0)
      gb->Geo->Draw();
  }
}

/****************************************************************************/

#define SIMD 1
#define SUBDIVIDE 1

struct simdpart
{
  __m128 x,y,z;
};

struct funcinfo
{
  __m128 tresh4;
  __m128 treshf4;
  __m128 one;
  __m128 epsilon;
  simdpart *parts4;
  sInt pn4;

  sF32 iso;
  sF32 tresh;
  sF32 treshf;
  sVector31 *parts;
  sInt pn;
};

#if !SIMD
static void sINLINE func(const sVector31 &v,sVector4 &pot,const funcinfo &fi)
{
  sF32 p = 0;
  sVector30 n(0,0,0);
  for(sInt i=0;i<fi.pn;i++)
  {
    sVector30 d(v-fi.parts[i]);
    sF32 pp = d^d;
    if(pp<=fi.treshf)
    {
      pp = 1.0f/(pp)-fi.tresh;
      p += pp;
      n += d*(pp*pp);
    }
  }
  if(p==0)
    n.Init(0,0,0);
  else
    n.UnitFast();
  pot.Init(n.x,n.y,n.z,p-fi.iso);
}
#endif

#if SIMD
static void sINLINE func(const sVector31 &v,sVector4 &pot,const funcinfo &fi)
{
  __m128 vx = _mm_load_ps1(&v.x);
  __m128 vy = _mm_load_ps1(&v.y);
  __m128 vz = _mm_load_ps1(&v.z);
  __m128 po = _mm_setzero_ps();
  __m128 nx = _mm_setzero_ps();
  __m128 ny = _mm_setzero_ps();
  __m128 nz = _mm_setzero_ps();
  
  for(sInt i=0;i<fi.pn4;i++)
  {
    const simdpart *part = fi.parts4 + i;

    __m128 dx = _mm_sub_ps(vx,part->x);
    __m128 dy = _mm_sub_ps(vy,part->y);
    __m128 dz = _mm_sub_ps(vz,part->z);
    __m128 ddx = _mm_mul_ps(dx,dx);
    __m128 ddy = _mm_mul_ps(dy,dy);
    __m128 ddz = _mm_mul_ps(dz,dz);
    __m128 pp = _mm_add_ps(_mm_add_ps(ddx,ddy),ddz);

    if(_mm_movemask_ps(_mm_cmple_ps(pp,fi.treshf4))!=0)
    {
      __m128 pp2 = _mm_sub_ps(_mm_div_ps(fi.one,pp),fi.tresh4);
      __m128 pp3 = _mm_max_ps(pp2,_mm_setzero_ps());
      po = _mm_add_ps(po,pp3);
      __m128 pp4 = _mm_mul_ps(pp3,pp3);
      nx = _mm_add_ps(nx,_mm_mul_ps(pp4,dx));
      ny = _mm_add_ps(ny,_mm_mul_ps(pp4,dy));
      nz = _mm_add_ps(nz,_mm_mul_ps(pp4,dz));
    }
  }

  sF32 p = 0;
  sVector30 n;
  
  _MM_TRANSPOSE4_PS(nx,ny,nz,po);
  __m128 r = _mm_add_ps(_mm_add_ps(_mm_add_ps(nx,ny),nz),po);
  n.x = r.m128_f32[0];
  n.y = r.m128_f32[1];
  n.z = r.m128_f32[2];
  p = r.m128_f32[3];

  if(p==0)
    n.Init(0,0,0);
  else
    n.UnitFast();
  pot.Init(n.x,n.y,n.z,p-fi.iso);
}
#endif

void MarchingCubes::RenderCell(GeoBuffer *gb,Container *con)
{
  const sInt s = CellSize;
  sF32 S = Scale;
  sVector30 spos(S,S,S);
  sVector31 tpos(con->Pos*S);

  sALIGNED(sVector4,pot[s+2][s+1][s+1],16);
  sU16 I[3][s+1][s+1][s+1];
  funcinfo fi;

  // calculate potential and normal

  fi.tresh = 1/(Influence*Scale*Influence*Scale);
  fi.treshf = 1.0f/fi.tresh+0.01f;
  fi.parts = con->Parts;      // the non-simd code paints only the first container!
  fi.pn = con->Count;
  fi.iso = IsoValue;

  // reorganize array for SIMD

#if SIMD

  sInt pn4 = 0;
  Container *cp = con;
  while(cp)
  {
    pn4 += (cp->Count+3)/4;
    cp = cp->Next;
  }

  fi.tresh4 = _mm_load_ps1(&fi.tresh);
  fi.treshf4 = _mm_load_ps1(&fi.treshf);
  fi.one = _mm_set_ps1(1.0f);
  fi.epsilon = _mm_set_ps1(0.01f);
  fi.pn4 = pn4;
  fi.parts4 = sALLOCSTACK(simdpart,fi.pn4);
  sInt i4 = 0;

  sVector31 far(1024*1024,0,0);
  cp = con;
  while(cp)
  {
    sInt pn = cp->Count;
    sVector31 *p = cp->Parts;

    switch(pn&3)
    {
      case 1: p[pn+2] = far;
      case 2: p[pn+1] = far;
      case 3: p[pn+0] = far;
      case 0: break;
    }

    for(sInt i=0;i<(pn+3)/4;i++)
    {
      fi.parts4[i4].x.m128_f32[0] = p[0].x;
      fi.parts4[i4].x.m128_f32[1] = p[1].x;
      fi.parts4[i4].x.m128_f32[2] = p[2].x;
      fi.parts4[i4].x.m128_f32[3] = p[3].x;

      fi.parts4[i4].y.m128_f32[0] = p[0].y;
      fi.parts4[i4].y.m128_f32[1] = p[1].y;
      fi.parts4[i4].y.m128_f32[2] = p[2].y;
      fi.parts4[i4].y.m128_f32[3] = p[3].y;

      fi.parts4[i4].z.m128_f32[0] = p[0].z;
      fi.parts4[i4].z.m128_f32[1] = p[1].z;
      fi.parts4[i4].z.m128_f32[2] = p[2].z;
      fi.parts4[i4].z.m128_f32[3] = p[3].z;

      p+=4;
      i4++;
    }
    cp = cp->Next;
  }
  sVERIFY(i4==fi.pn4);
#endif

  // pass 1: skip every second vertex

  const sInt step = SUBDIVIDE ? 2 : 1;
  for(sInt z=0;z<s+1;z+=step)
  {
    for(sInt y=0;y<s+1;y+=step)
    {
      for(sInt x=0;x<s+1;x+=step)
      {
        sVector31 v = sVector30(x,y,z) * spos + tpos;

        func(v,pot[z][y][x],fi);
      }
    }
  }

  // pass 2: refine where required

#if SUBDIVIDE
  for(sInt z=0;z<=s;z+=2)
  {
    for(sInt y=0;y<=s;y+=2)
    {
      for(sInt x=0;x<s;x+=2)
      {
        sBool c0 = pot[z+0][y+0][x+0].w<0;
        sBool c1 = pot[z+0][y+0][x+2].w<0;
        if(c0!=c1)
        {
          sVector31 v = sVector30(x+1,y,z) * spos + tpos;
          func(v,pot[z][y][x+1],fi);
        }
        else
        {
          pot[z+0][y+0][x+1].Average(pot[z+0][y+0][x+0],pot[z+0][y+0][x+2]);
        }
      }
    }
  }
  for(sInt z=0;z<=s;z+=2)
  {
    for(sInt y=0;y<s;y+=2)
    {
      for(sInt x=0;x<=s;x+=1)
      {
        sBool c0 = pot[z+0][y+0][x+0].w<0;
        sBool c1 = pot[z+0][y+2][x+0].w<0;
        if(c0!=c1)
        {
          sVector31 v = sVector30(x,y+1,z) * spos + tpos;
          func(v,pot[z][y+1][x],fi);
        }
        else
        {
          pot[z+0][y+1][x+0].Average(pot[z+0][y+0][x+0],pot[z+0][y+2][x+0]);
        }
      }
    }
  }
  for(sInt z=0;z<s;z+=2)
  {
    for(sInt y=0;y<=s;y+=1)
    {
      for(sInt x=0;x<=s;x+=1)
      {
        sBool c0 = pot[z+0][y+0][x+0].w<0;
        sBool c1 = pot[z+2][y+0][x+0].w<0;
        if(c0!=c1)
        {
          sVector31 v = sVector30(x,y,z+1) * spos + tpos;
          func(v,pot[z+1][y][x],fi);
        }
        else
        {
          pot[z+1][y+0][x+0].Average(pot[z+0][y+0][x+0],pot[z+2][y+0][x+0]);
        }
      }
    }
  }
#endif

  // find and write potential vertices

  sVertexStandard *vp = gb->VP + gb->Vertex;
  sInt vi = gb->Vertex;

  for(sInt z=0;z<=s;z++)
  {
    for(sInt y=0;y<=s;y++)
    {
      for(sInt x=0;x<=s;x++)
      {
        sVector4 p0 = pot[z+0][y+0][x+0];
        sVector4 px = pot[z+0][y+0][x+1];
        sVector4 py = pot[z+0][y+1][x+0];
        sVector4 pz = pot[z+1][y+0][x+0];
        sBool c0 = p0.w<=0;
        sBool cx = px.w<=0;
        sBool cy = py.w<=0;
        sBool cz = pz.w<=0;
        sVector31 v = sVector30(x,y,z) * spos + tpos;

        if(c0!=cx)                // x
        {
          sF32 f = -p0.w / (px.w-p0.w);
          vp->px = v.x+f*S;
          vp->py = v.y;
          vp->pz = v.z;
          vp->nx = sFade(f,p0.x,px.x);
          vp->ny = sFade(f,p0.y,px.y);
          vp->nz = sFade(f,p0.z,px.z);
          vp->u0 = 0;
          vp->v0 = 0;
          I[0][z][y][x] = vi++;
          vp++;
        }

        if(c0!=cy)                // y
        {
          sF32 f = -p0.w / (py.w-p0.w);
          vp->px = v.x;
          vp->py = v.y+f*S;
          vp->pz = v.z;
          vp->nx = sFade(f,p0.x,py.x);
          vp->ny = sFade(f,p0.y,py.y);
          vp->nz = sFade(f,p0.z,py.z);
          vp->u0 = 0;
          vp->v0 = 0;
          I[1][z][y][x] = vi++;
          vp++;
        }

        if(c0!=cz)                // z
        {
          sF32 f = -p0.w / (pz.w-p0.w);
          vp->px = v.x;
          vp->py = v.y;
          vp->pz = v.z+f*S;
          vp->nx = sFade(f,p0.x,pz.x);
          vp->ny = sFade(f,p0.y,pz.y);
          vp->nz = sFade(f,p0.z,pz.z);
          vp->u0 = 0;
          vp->v0 = 0;
          I[2][z][y][x] = vi++;
          vp++;
        }
      }
    }
  }

  gb->Vertex = vi;

  // write out indices

  sU16 *ip = gb->IP + gb->Index;

  for(sInt z=0;z<s;z++)
  {
    for(sInt y=0;y<s;y++)
    {
      for(sInt x=0;x<s;x++)
      {
        sInt vm = 0;
        if(pot[z+0][y+0][x+0].w<=0) vm |= 0x01;
        if(pot[z+0][y+0][x+1].w<=0) vm |= 0x02;
        if(pot[z+0][y+1][x+1].w<=0) vm |= 0x04;
        if(pot[z+0][y+1][x+0].w<=0) vm |= 0x08;
        if(pot[z+1][y+0][x+0].w<=0) vm |= 0x10;
        if(pot[z+1][y+0][x+1].w<=0) vm |= 0x20;
        if(pot[z+1][y+1][x+1].w<=0) vm |= 0x40;
        if(pot[z+1][y+1][x+0].w<=0) vm |= 0x80;

        sInt n = MyTriTable[vm][0];
        for(sInt i=0;i<n;i++)
          ip[i] = I[0][z][y][x + MyTriTable[vm][1+i]];
        ip += n;
      }
    }
  }

  gb->Index = ip - gb->IP;
}


/****************************************************************************/
/***                                                                      ***/
/***   GeoBuffer. Usefull in many contexts                                ***/
/***                                                                      ***/
/****************************************************************************/

GeoBufferHelper::GeoBuffer::GeoBuffer(GeoBufferHelper *base)
{
  VertexAlloc = base->VertexMax;
  IndexAlloc = base->IndexMax;
  VertexUsed = 0;
  IndexUsed = 0;

  Base = base;
  Geo = new sGeometry(sGF_INDEX16|sGF_TRILIST,base->Format);
  if(Base->Pipelined)
    Geo->InitDyn(IndexAlloc,VertexAlloc);

  VP = 0;
  IP = 0;
}

GeoBufferHelper::GeoBuffer::~GeoBuffer()
{
  delete Geo;
}

void GeoBufferHelper::GeoBuffer::Begin(sBool lock)
{
  if(lock) Base->Lock.Lock();
  if(Base->Pipelined)
  {
    VP = (sF32 *)Geo->BeginDynVB(1,0);
    IP = (sU16 *)Geo->BeginDynIB(1);
  }
  else
  {
    Geo->BeginLoadVB(VertexAlloc,Base->Duration,&VP);
    Geo->BeginLoadIB(IndexAlloc,Base->Duration,&IP);
  }
  if(lock) Base->Lock.Unlock();
  VertexUsed = 0;
  IndexUsed = 0;
}

void GeoBufferHelper::GeoBuffer::End()
{
  Base->Lock.Lock();
  if(Base->Pipelined)
  {
    Geo->EndDynVB(0);
    Geo->EndDynIB();
  }
  else
  {
    Geo->EndLoadVB(VertexUsed);
    Geo->EndLoadIB(IndexUsed);
  }
  Base->Lock.Unlock();
}

void GeoBufferHelper::GeoBuffer::Draw()
{
  if(IndexUsed>0)
  {
    sDrawRange dr;
    dr.Start = 0;
    dr.End = IndexUsed;
    Geo->Draw(&dr,1,0,0);
  }
}

/****************************************************************************/

GeoBufferHelper::GeoBufferHelper()
{
  MaxThread = sSched->GetThreadCount();
  Format = sVertexFormatStandard;
  VertexMax = 0x8000;
  IndexMax = 0x30000;
  GeoCount = 0;
  ThreadGeo = new GeoBuffer *[MaxThread];
  for(sInt i=0;i<MaxThread;i++)
    ThreadGeo[i] = 0;
}

GeoBufferHelper::~GeoBufferHelper()
{
  sDeleteAll(FreeGeos);
  sDeleteAll(ReadyGeos);
  sDeleteAll(DrawGeos);
  delete[] ThreadGeo;
}

void GeoBufferHelper::Init(sVertexFormatHandle *fmt,sInt vc,sInt ic,sBool p,sBool s)
{
  Format = fmt;
  FloatSize = Format->GetSize(0)/4;
  VertexMax = vc;
  IndexMax = ic;
  Pipelined = p;
  Duration = s ? sGD_STATIC : sGD_FRAME;
}

void GeoBufferHelper::Begin()
{
  for(sInt i=0;i<MaxThread;i++)
    ThreadGeo[i] = 0;

//  if(Pipelined)
  {
    GeoBuffer *gb;
    sFORALL(FreeGeos,gb)
      gb->Begin();
  }
}

void GeoBufferHelper::End()
{
  GeoBuffer *gb;
//  if(Pipelined)
  {
    sFORALL(FreeGeos,gb)
      gb->End(); 
  }
  sFORALL(ReadyGeos,gb)
    gb->End(); 
  for(sInt i=0;i<MaxThread;i++)
  {
    if(ThreadGeo[i])
    {
//      ThreadGeo[i]->End();
      ThreadGeo[i] = 0;
    }
  }

  FreeGeos.Add(DrawGeos);
  DrawGeos.Clear();
  ReadyGeos.Swap(DrawGeos);
}

void GeoBufferHelper::ChargeGeo(sInt thread)
{
  GeoBuffer *c = 0;
  if(thread==0 && FreeGeos.GetCount()<2*MaxThread)
  {
    Lock.Lock();
    while(FreeGeos.GetCount()<2*MaxThread)
    {
      c = new GeoBuffer(this);
//      if(Pipelined)
        c->Begin(0);
      FreeGeos.AddTail(c);
      GeoCount++;
    }
    ReadyGeos.GrowTo(GeoCount);
    DrawGeos.GrowTo(GeoCount);
    Lock.Unlock();
  }
}


GeoBufferHelper::GeoBuffer *GeoBufferHelper::GetBuffer(sInt thread)
{
  GeoBuffer *c = 0;

  if(thread==0)
    ChargeGeo(thread);

  for(;;)
  {
    Lock.Lock();
    if(!FreeGeos.IsEmpty())
    {
      c = FreeGeos.RemTail();
      ReadyGeos.AddTail(c);
      Lock.Unlock();
      break;
    }
    Lock.Unlock();
    sSpin();
  }

  return c;
}

void GeoBufferHelper::BeginLoad(sInt thread,sInt vcmax,sInt icmax,sInt &vi,void **vc,sU16 **ic)
{
  GeoBuffer *gb = ThreadGeo[thread];
  if(gb==0 || gb->VertexUsed+vcmax>gb->VertexAlloc || gb->IndexUsed+icmax>gb->IndexAlloc)
  {
//    if(gb)
//      gb->End();
    gb = ThreadGeo[thread] = GetBuffer(thread);
//    if(!Pipelined)
//      gb->Begin();
  }
  *vc = gb->VP + FloatSize*gb->VertexUsed;
  *ic = gb->IP + gb->IndexUsed;
  vi = gb->VertexUsed;
}

void GeoBufferHelper::EndLoad(sInt thread,sInt vc,sInt ic)
{
  ThreadGeo[thread]->VertexUsed += vc;
  ThreadGeo[thread]->IndexUsed += ic;
}

void GeoBufferHelper::Draw()
{
  GeoBuffer *gb;
  sFORALL(DrawGeos,gb)
  {
    gb->Draw();
  }
}

/****************************************************************************/
/****************************************************************************/

MarchingCubesHelper::MarchingCubesHelper(sBool color)
{
  MaxThread = sSched->GetThreadCount();
  for(sInt p=0;p<6;p++)
  {
    sInt s = 1<<p;
    sInt trans_[12] =
    {
      0*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 0*(s+1) + 0,
      1*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 0*(s+1) + 1,
      0*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 1*(s+1) + 0,
      1*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 0*(s+1) + 0,

      0*(s+1)*(s+1)*(s+1) + 1*(s+1)*(s+1) + 0*(s+1) + 0,
      1*(s+1)*(s+1)*(s+1) + 1*(s+1)*(s+1) + 0*(s+1) + 1,
      0*(s+1)*(s+1)*(s+1) + 1*(s+1)*(s+1) + 1*(s+1) + 0,
      1*(s+1)*(s+1)*(s+1) + 1*(s+1)*(s+1) + 0*(s+1) + 0,

      2*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 0*(s+1) + 0,
      2*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 0*(s+1) + 1,
      2*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 1*(s+1) + 1,
      2*(s+1)*(s+1)*(s+1) + 0*(s+1)*(s+1) + 1*(s+1) + 0,
    };

    for(sInt i=0;i<256;i++)
    {
      sInt n = 0;
      while(triTableBase[i][n]!=-1) n++;
      MyTriTable[p][i][0] = n;
      for(sInt j=0;j<n;j++)
        MyTriTable[p][i][j+1] = trans_[triTableBase[i][j]];
    }
  }

  sU32 desc0[] = 
  {
    sVF_POSITION|sVF_F3,
    sVF_NORMAL|sVF_F3,
    sVF_END,
  };
  sU32 desc1[] = 
  {
    sVF_POSITION|sVF_F3,
    sVF_NORMAL|sVF_F3,
    sVF_COLOR0|sVF_C4,
    sVF_END,
  };

  Format = sCreateVertexFormat(color ? desc1:desc0);
}

MarchingCubesHelper::~MarchingCubesHelper()
{
}

void MarchingCubesHelper::Init(sBool p,sBool s)
{
  Geos.Init(Format,0x8000,0x18000,p,s);
}

void MarchingCubesHelper::Begin()
{
  Geos.Begin();
}

void MarchingCubesHelper::End()
{
  Geos.End();
}

void MarchingCubesHelper::Draw()
{
  Geos.Draw();
}

void MarchingCubesHelper::ChargeGeo()
{
  Geos.ChargeGeo(0);
}

/****************************************************************************/

sU32 MCPotField::c = 0;

sU32 myfade(sU32 a, sU32 b, sF32 f)
{
  if((a&0xff000000)==0) return b;
  if((b&0xff000000)==0) return a;
  sU32 f1 = sU32(f*256);
  sU32 f0 = 256-sU32(f*256);

  return 0xff000000 |
         (( ( ((a>>16)&0xff)*f0+((b>>16)&0xff)*f1 ) /256) << 16) |
         (( ( ((a>> 8)&0xff)*f0+((b>> 8)&0xff)*f1 ) /256) <<  8) |
         (( ( ((a>> 0)&0xff)*f0+((b>> 0)&0xff)*f1 ) /256) <<  0);
}


template<class fieldtype,int CellSizeP,int Iso0>
void MarchingCubesHelper::March(const fieldtype *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread)
{
  const sInt CellSize = 1<<CellSizeP;
  const sInt s = CellSize;
  const sInt m = CellSize+1;
  const sInt mm = m*m;
  sVector30 spos(scale,scale,scale);
  sF32 S = scale;
  sU16 I[3][s+1][s+1][s+1];

  sInt worstvc = (s+1)*(s+1)*(s+1)*3;
  sInt worstic = s*s*s*15;

  if(Iso0)
    iso = 0;

  // find and write potential vertices

  const sInt vsize = 6 + fieldtype::ColorFlag;

  sInt vi;
  sF32 *vp;
  sU16 *ip;
  Geos.BeginLoad(thread,worstvc,worstic,vi,&vp,&ip);

  sInt vc = 0;
  for(sInt z=0;z<=s;z++)
  {
    for(sInt y=0;y<=s;y++)
    {
      for(sInt x=0;x<=s;x++)
      {
        fieldtype p0 = pot[(z+0)*mm + (y+0)*m + x+0];
        fieldtype px = pot[(z+0)*mm + (y+0)*m + x+1];
        fieldtype py = pot[(z+0)*mm + (y+1)*m + x+0];
        fieldtype pz = pot[(z+1)*mm + (y+0)*m + x+0];

/*
        sBool c0 = p0.w<=iso;
        sBool cx = px.w<=iso;
        sBool cy = py.w<=iso;
        sBool cz = pz.w<=iso;
        */
        __m128 cmp = _mm_cmple_ps(_mm_set_ps(pz.w,py.w,px.w,p0.w),_mm_set1_ps(iso));
        sU32 c0 = cmp.m128_u32[0];
        sU32 cx = cmp.m128_u32[1];
        sU32 cy = cmp.m128_u32[2];
        sU32 cz = cmp.m128_u32[3];

        sVector31 v = sVector30(x,y,z) * spos + tpos;

        if(c0!=cx)                // x
        {
          sF32 f = -(p0.w-iso) / (px.w-p0.w);
          vp[0] = v.x+f*S;
          vp[1] = v.y;
          vp[2] = v.z;
          vp[3] = sFade(f,p0.x,px.x);
          vp[4] = sFade(f,p0.y,px.y);
          vp[5] = sFade(f,p0.z,px.z);
          if(fieldtype::ColorFlag)
            *(sU32*)&vp[6] = myfade(p0.c,px.c,f);

          I[0][z][y][x] = vi++;
          vp+=vsize;
          vc++;
        }

        if(c0!=cy)                // y
        {
          sF32 f = -(p0.w-iso) / (py.w-p0.w);
          vp[0] = v.x;
          vp[1] = v.y+f*S;
          vp[2] = v.z;
          vp[3] = sFade(f,p0.x,py.x);
          vp[4] = sFade(f,p0.y,py.y);
          vp[5] = sFade(f,p0.z,py.z);
          if(fieldtype::ColorFlag)
            *(sU32*)&vp[6] = myfade(p0.c,py.c,f);

          I[1][z][y][x] = vi++;
          vp+=vsize;
          vc++;
        }

        if(c0!=cz)                // z
        {
          sF32 f = -(p0.w-iso) / (pz.w-p0.w);
          vp[0] = v.x;
          vp[1] = v.y;
          vp[2] = v.z+f*S;
          vp[3] = sFade(f,p0.x,pz.x);
          vp[4] = sFade(f,p0.y,pz.y);
          vp[5] = sFade(f,p0.z,pz.z);
          if(fieldtype::ColorFlag)
            *(sU32*)&vp[6] = myfade(p0.c,pz.c,f);

          I[2][z][y][x] = vi++;
          vp+=vsize;
          vc++;
        }
      }
    }
  }

  // write out indices

  sInt ic=0;
  for(sInt z=0;z<s;z++)
  {
    for(sInt y=0;y<s;y++)
    {
      for(sInt x=0;x<s;x++)
      {
        sInt vm = 0;
/*
        if(pot[(z+0)*mm + (y+0)*m + (x+0)].w<=iso) vm |= 0x01;
        if(pot[(z+0)*mm + (y+0)*m + (x+1)].w<=iso) vm |= 0x02;
        if(pot[(z+0)*mm + (y+1)*m + (x+1)].w<=iso) vm |= 0x04;
        if(pot[(z+0)*mm + (y+1)*m + (x+0)].w<=iso) vm |= 0x08;
        if(pot[(z+1)*mm + (y+0)*m + (x+0)].w<=iso) vm |= 0x10;
        if(pot[(z+1)*mm + (y+0)*m + (x+1)].w<=iso) vm |= 0x20;
        if(pot[(z+1)*mm + (y+1)*m + (x+1)].w<=iso) vm |= 0x40;
        if(pot[(z+1)*mm + (y+1)*m + (x+0)].w<=iso) vm |= 0x80;
*/
        __m128 pot0 = _mm_set_ps(pot[(z+0)*mm + (y+1)*m + (x+0)].w,
                                 pot[(z+0)*mm + (y+1)*m + (x+1)].w,
                                 pot[(z+0)*mm + (y+0)*m + (x+1)].w,
                                 pot[(z+0)*mm + (y+0)*m + (x+0)].w);
        __m128 pot1 = _mm_set_ps(pot[(z+1)*mm + (y+1)*m + (x+0)].w,
                                 pot[(z+1)*mm + (y+1)*m + (x+1)].w,
                                 pot[(z+1)*mm + (y+0)*m + (x+1)].w,
                                 pot[(z+1)*mm + (y+0)*m + (x+0)].w);
        __m128 mmiso = _mm_set1_ps(iso);
        __m128 cmp0 = _mm_cmple_ps(pot0,mmiso);
        __m128 cmp1 = _mm_cmple_ps(pot1,mmiso);
        vm = _mm_movemask_ps(cmp0) | (_mm_movemask_ps(cmp1)<<4);

        sInt n = MyTriTable[CellSizeP][vm][0];
        for(sInt i=0;i<n;i++)
        {
          sInt index = I[0][z][y][x + MyTriTable[CellSizeP][vm][1+i]];
          ip[i] = index;
        }
        ip += n;
        ic += n;
      }
    }
  }

  // done

  Geos.EndLoad(thread,vc,ic);
}


/****************************************************************************/

void MarchingCubesHelper::March(sInt grid,const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread)
{
  switch(grid)
  {
    case 0: March_0_1(pot,scale,tpos,thread); break;
    case 1: March_1_1(pot,scale,tpos,thread); break;
    case 2: March_2_1(pot,scale,tpos,thread); break;
    case 3: March_3_1(pot,scale,tpos,thread); break;
    case 4: March_4_1(pot,scale,tpos,thread); break;
    case 5: March_5_1(pot,scale,tpos,thread); break;
    default: sVERIFYFALSE;
  }
}

void MarchingCubesHelper::MarchIso(sInt grid,const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread)
{
  switch(grid)
  {
    case 0: March_0_0(pot,scale,tpos,iso,thread); break;
    case 1: March_1_0(pot,scale,tpos,iso,thread); break;
    case 2: March_2_0(pot,scale,tpos,iso,thread); break;
    case 3: March_3_0(pot,scale,tpos,iso,thread); break;
    case 4: March_4_0(pot,scale,tpos,iso,thread); break;
    case 5: March_5_0(pot,scale,tpos,iso,thread); break;
    default: sVERIFYFALSE;
  }
}

void MarchingCubesHelper::MarchColor(sInt grid,const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread)
{
  switch(grid)
  {
    case 0: March_0_1(pot,scale,tpos,thread); break;
    case 1: March_1_1(pot,scale,tpos,thread); break;
    case 2: March_2_1(pot,scale,tpos,thread); break;
    case 3: March_3_1(pot,scale,tpos,thread); break;
    case 4: March_4_1(pot,scale,tpos,thread); break;
    case 5: March_5_1(pot,scale,tpos,thread); break;
    default: sVERIFYFALSE;
  }
}


void MarchingCubesHelper::March_0_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotField,1,1>(pot,scale,tpos,0,thread); }
void MarchingCubesHelper::March_1_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotField,1,1>(pot,scale,tpos,0,thread); }
void MarchingCubesHelper::March_2_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotField,2,1>(pot,scale,tpos,0,thread); }
void MarchingCubesHelper::March_3_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotField,3,1>(pot,scale,tpos,0,thread); }
void MarchingCubesHelper::March_4_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotField,4,1>(pot,scale,tpos,0,thread); }
void MarchingCubesHelper::March_5_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotField,5,1>(pot,scale,tpos,0,thread); }

void MarchingCubesHelper::March_0_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread) { March<MCPotField,1,0>(pot,scale,tpos,iso,thread); }
void MarchingCubesHelper::March_1_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread) { March<MCPotField,1,0>(pot,scale,tpos,iso,thread); }
void MarchingCubesHelper::March_2_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread) { March<MCPotField,2,0>(pot,scale,tpos,iso,thread); }
void MarchingCubesHelper::March_3_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread) { March<MCPotField,3,0>(pot,scale,tpos,iso,thread); }
void MarchingCubesHelper::March_4_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread) { March<MCPotField,4,0>(pot,scale,tpos,iso,thread); }
void MarchingCubesHelper::March_5_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread) { March<MCPotField,5,0>(pot,scale,tpos,iso,thread); }

void MarchingCubesHelper::March_0_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotFieldColor,1,1>(pot,scale,tpos,0,thread); }
void MarchingCubesHelper::March_1_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotFieldColor,1,1>(pot,scale,tpos,0,thread); }
void MarchingCubesHelper::March_2_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotFieldColor,2,1>(pot,scale,tpos,0,thread); }
void MarchingCubesHelper::March_3_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotFieldColor,3,1>(pot,scale,tpos,0,thread); }
void MarchingCubesHelper::March_4_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotFieldColor,4,1>(pot,scale,tpos,0,thread); }
void MarchingCubesHelper::March_5_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread) { March<MCPotFieldColor,5,1>(pot,scale,tpos,0,thread); }

/****************************************************************************/
