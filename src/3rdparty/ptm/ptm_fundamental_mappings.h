/*Copyright (c) 2022 PM Larsen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PTM_FUNDAMENTAL_MAPPINGS_H
#define PTM_FUNDAMENTAL_MAPPINGS_H

#include <stdint.h>

namespace ptm {

#define NUM_CUBIC_MAPPINGS 24
#define NUM_ICO_MAPPINGS 60
#define NUM_HEX_MAPPINGS 6
#define NUM_DCUB_MAPPINGS 12
#define NUM_DHEX_MAPPINGS 3

#define NUM_GRAPHENE_MAPPINGS 6
#define NUM_CONVENTIONAL_GRAPHENE_MAPPINGS 12

#define NUM_CONVENTIONAL_HEX_MAPPINGS 12
#define NUM_CONVENTIONAL_DCUB_MAPPINGS 24
#define NUM_CONVENTIONAL_DHEX_MAPPINGS 12


const int8_t template_indices_zero[NUM_ICO_MAPPINGS] = {0};

//-----------------------------------------------------------------------------
// sc
//-----------------------------------------------------------------------------

const int8_t mapping_sc[NUM_CUBIC_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6},
                                        { 0,  3,  4,  2,  1,  5,  6},
                                        { 0,  6,  5,  3,  4,  1,  2},
                                        { 0,  1,  2,  5,  6,  4,  3},
                                        { 0,  1,  2,  6,  5,  3,  4},
                                        { 0,  5,  6,  3,  4,  2,  1},
                                        { 0,  4,  3,  1,  2,  5,  6},
                                        { 0,  3,  4,  5,  6,  1,  2},
                                        { 0,  6,  5,  2,  1,  3,  4},
                                        { 0,  5,  6,  2,  1,  4,  3},
                                        { 0,  3,  4,  6,  5,  2,  1},
                                        { 0,  6,  5,  1,  2,  4,  3},
                                        { 0,  4,  3,  6,  5,  1,  2},
                                        { 0,  4,  3,  5,  6,  2,  1},
                                        { 0,  5,  6,  1,  2,  3,  4},
                                        { 0,  2,  1,  4,  3,  5,  6},
                                        { 0,  2,  1,  5,  6,  3,  4},
                                        { 0,  5,  6,  4,  3,  1,  2},
                                        { 0,  6,  5,  4,  3,  2,  1},
                                        { 0,  2,  1,  6,  5,  4,  3},
                                        { 0,  2,  1,  3,  4,  6,  5},
                                        { 0,  3,  4,  1,  2,  6,  5},
                                        { 0,  4,  3,  2,  1,  6,  5},
                                        { 0,  1,  2,  4,  3,  6,  5},
};

//-----------------------------------------------------------------------------
// fcc
//-----------------------------------------------------------------------------

const int8_t mapping_fcc[NUM_CUBIC_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
                                        { 0, 12, 11,  1,  9,  8,  4,  6,  2,  7,  3,  5, 10},
                                        { 0,  2,  7,  9,  5, 10, 12, 11,  4,  6,  8,  1,  3},
                                        { 0, 10,  3,  8,  7,  6, 11,  1,  9,  2,  4, 12,  5},
                                        { 0,  7,  9,  2, 10, 12,  5,  4,  3,  8,  1,  6, 11},
                                        { 0, 11,  1, 12,  8,  4,  9,  2, 10,  3,  5,  7,  6},
                                        { 0,  3,  8, 10,  6, 11,  7,  9,  5,  4, 12,  2,  1},
                                        { 0,  3,  1,  2,  6,  4,  5, 12,  7, 11,  9, 10,  8},
                                        { 0, 11,  6,  7,  8,  3, 10,  5,  9,  4,  2, 12,  1},
                                        { 0,  5, 12, 10,  2,  9,  7, 11,  3,  1,  8,  6,  4},
                                        { 0,  6,  7, 11,  3, 10,  8,  9,  1,  2, 12,  4,  5},
                                        { 0,  8,  9,  4, 11, 12,  1,  2,  6,  7,  5,  3, 10},
                                        { 0,  9,  4,  8, 12,  1, 11,  6, 10,  5,  3,  7,  2},
                                        { 0, 12, 10,  5,  9,  7,  2,  3,  4,  8,  6,  1, 11},
                                        { 0,  2,  3,  1,  5,  6,  4,  8, 12, 10, 11,  9,  7},
                                        { 0, 10,  5, 12,  7,  2,  9,  4, 11,  6,  1,  8,  3},
                                        { 0,  1, 12, 11,  4,  9,  8, 10,  6,  5,  7,  3,  2},
                                        { 0,  8, 10,  3, 11,  7,  6,  5,  1, 12,  2,  4,  9},
                                        { 0,  5,  4,  6,  2,  1,  3,  8,  7,  9, 11, 10, 12},
                                        { 0,  4,  6,  5,  1,  3,  2,  7, 12, 11, 10,  9,  8},
                                        { 0,  7, 11,  6, 10,  8,  3,  1,  5, 12,  4,  2,  9},
                                        { 0,  9,  2,  7, 12,  5, 10,  3, 11,  1,  6,  8,  4},
                                        { 0,  6,  5,  4,  3,  2,  1, 12,  8, 10,  9, 11,  7},
                                        { 0,  4,  8,  9,  1, 11, 12, 10,  2,  3,  7,  5,  6},
        };

//-----------------------------------------------------------------------------
// bcc
//-----------------------------------------------------------------------------

const int8_t mapping_bcc[NUM_CUBIC_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14},
                                        { 0,  3,  6,  8,  2,  1,  7,  4,  5,  9, 10, 14, 13, 11, 12},
                                        { 0,  2,  6,  1,  7,  4,  3,  8,  5, 13, 14, 11, 12, 10,  9},
                                        { 0,  5,  1,  8,  2,  4,  3,  6,  7, 12, 11,  9, 10, 13, 14},
                                        { 0,  2,  4,  6,  5,  1,  7,  8,  3, 11, 12, 10,  9, 13, 14},
                                        { 0,  3,  1,  6,  5,  8,  2,  4,  7, 14, 13, 11, 12,  9, 10},
                                        { 0,  5,  4,  1,  7,  8,  2,  6,  3,  9, 10, 13, 14, 12, 11},
                                        { 0,  1,  3,  5,  6,  2,  8,  7,  4, 13, 14,  9, 10, 11, 12},
                                        { 0,  6,  7,  3,  4,  2,  8,  5,  1, 11, 12, 14, 13, 10,  9},
                                        { 0,  8,  3,  7,  1,  5,  6,  2,  4, 12, 11, 14, 13,  9, 10},
                                        { 0,  6,  2,  7,  1,  3,  4,  5,  8, 14, 13, 10,  9, 11, 12},
                                        { 0,  4,  2,  5,  6,  7,  1,  3,  8, 12, 11, 13, 14, 10,  9},
                                        { 0,  4,  7,  2,  8,  5,  6,  3,  1, 13, 14, 10,  9, 12, 11},
                                        { 0,  8,  5,  3,  4,  7,  1,  2,  6, 14, 13,  9, 10, 12, 11},
                                        { 0,  1,  5,  2,  8,  3,  4,  7,  6, 11, 12, 13, 14,  9, 10},
                                        { 0,  8,  7,  5,  6,  3,  4,  2,  1,  9, 10, 12, 11, 14, 13},
                                        { 0,  3,  8,  1,  7,  6,  5,  4,  2, 11, 12,  9, 10, 14, 13},
                                        { 0,  5,  8,  4,  3,  1,  7,  6,  2, 13, 14, 12, 11,  9, 10},
                                        { 0,  7,  4,  8,  2,  6,  5,  1,  3, 14, 13, 12, 11, 10,  9},
                                        { 0,  7,  6,  4,  3,  8,  2,  1,  5, 12, 11, 10,  9, 14, 13},
                                        { 0,  6,  3,  2,  8,  7,  1,  5,  4, 10,  9, 11, 12, 14, 13},
                                        { 0,  2,  1,  4,  3,  6,  5,  8,  7, 10,  9, 13, 14, 11, 12},
                                        { 0,  7,  8,  6,  5,  4,  3,  1,  2, 10,  9, 14, 13, 12, 11},
                                        { 0,  4,  5,  7,  1,  2,  8,  3,  6, 10,  9, 12, 11, 13, 14},
        };

//-----------------------------------------------------------------------------
// ico
//-----------------------------------------------------------------------------

const int8_t mapping_ico[NUM_ICO_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
                                        { 0,  6,  5,  2,  1, 12, 11,  4,  3,  9, 10,  7,  8},
                                        { 0,  6,  5,  9, 10,  4,  3,  7,  8, 12, 11,  2,  1},
                                        { 0,  8,  7,  2,  1,  4,  3, 10,  9,  5,  6, 11, 12},
                                        { 0, 10,  9,  3,  4, 12, 11,  5,  6,  8,  7,  2,  1},
                                        { 0,  8,  7, 11, 12,  5,  6,  4,  3,  2,  1, 10,  9},
                                        { 0,  1,  2, 11, 12,  9, 10,  5,  6,  3,  4,  7,  8},
                                        { 0,  1,  2,  9, 10,  7,  8, 11, 12,  5,  6,  3,  4},
                                        { 0, 10,  9,  8,  7,  5,  6,  2,  1, 12, 11,  3,  4},
                                        { 0, 12, 11,  3,  4,  7,  8, 10,  9,  2,  1,  6,  5},
                                        { 0,  4,  3,  6,  5,  9, 10,  2,  1,  8,  7, 11, 12},
                                        { 0, 12, 11,  6,  5,  2,  1,  7,  8,  3,  4, 10,  9},
                                        { 0,  4,  3,  8,  7,  2,  1, 11, 12,  9, 10,  6,  5},
                                        { 0,  3,  4,  5,  6,  1,  2, 10,  9, 12, 11,  7,  8},
                                        { 0, 11, 12,  9, 10,  1,  2,  4,  3,  8,  7,  5,  6},
                                        { 0,  3,  4,  7,  8, 12, 11,  1,  2,  5,  6, 10,  9},
                                        { 0,  8,  7,  5,  6, 10,  9, 11, 12,  4,  3,  2,  1},
                                        { 0, 10,  9,  2,  1,  8,  7, 12, 11,  3,  4,  5,  6},
                                        { 0, 11, 12,  8,  7,  4,  3,  5,  6,  1,  2,  9, 10},
                                        { 0,  6,  5,  7,  8,  9, 10, 12, 11,  2,  1,  4,  3},
                                        { 0,  6,  5, 12, 11,  7,  8,  2,  1,  4,  3,  9, 10},
                                        { 0,  9, 10, 11, 12,  4,  3,  1,  2,  7,  8,  6,  5},
                                        { 0, 12, 11,  2,  1, 10,  9,  6,  5,  7,  8,  3,  4},
                                        { 0,  4,  3, 11, 12,  8,  7,  9, 10,  6,  5,  2,  1},
                                        { 0,  7,  8,  6,  5, 12, 11,  9, 10,  1,  2,  3,  4},
                                        { 0,  8,  7, 10,  9,  2,  1,  5,  6, 11, 12,  4,  3},
                                        { 0, 10,  9, 12, 11,  2,  1,  3,  4,  5,  6,  8,  7},
                                        { 0,  9, 10,  6,  5,  7,  8,  4,  3, 11, 12,  1,  2},
                                        { 0,  4,  3,  9, 10, 11, 12,  6,  5,  2,  1,  8,  7},
                                        { 0, 12, 11, 10,  9,  3,  4,  2,  1,  6,  5,  7,  8},
                                        { 0,  7,  8,  1,  2,  9, 10,  3,  4, 12, 11,  6,  5},
                                        { 0,  5,  6,  8,  7, 11, 12, 10,  9,  3,  4,  1,  2},
                                        { 0,  5,  6,  1,  2,  3,  4, 11, 12,  8,  7, 10,  9},
                                        { 0, 11, 12,  5,  6,  8,  7,  1,  2,  9, 10,  4,  3},
                                        { 0,  3,  4, 12, 11, 10,  9,  7,  8,  1,  2,  5,  6},
                                        { 0,  9, 10,  7,  8,  1,  2,  6,  5,  4,  3, 11, 12},
                                        { 0,  7,  8,  3,  4,  1,  2, 12, 11,  6,  5,  9, 10},
                                        { 0,  3,  4, 10,  9,  5,  6, 12, 11,  7,  8,  1,  2},
                                        { 0,  1,  2,  7,  8,  3,  4,  9, 10, 11, 12,  5,  6},
                                        { 0,  1,  2,  5,  6, 11, 12,  3,  4,  7,  8,  9, 10},
                                        { 0, 11, 12,  1,  2,  5,  6,  9, 10,  4,  3,  8,  7},
                                        { 0,  5,  6,  3,  4, 10,  9,  1,  2, 11, 12,  8,  7},
                                        { 0,  5,  6, 10,  9,  8,  7,  3,  4,  1,  2, 11, 12},
                                        { 0,  9, 10,  1,  2, 11, 12,  7,  8,  6,  5,  4,  3},
                                        { 0,  7,  8, 12, 11,  3,  4,  6,  5,  9, 10,  1,  2},
                                        { 0,  2,  1, 12, 11,  6,  5, 10,  9,  8,  7,  4,  3},
                                        { 0,  2,  1,  4,  3,  8,  7,  6,  5, 12, 11, 10,  9},
                                        { 0,  9, 10,  4,  3,  6,  5, 11, 12,  1,  2,  7,  8},
                                        { 0,  7,  8,  9, 10,  6,  5,  1,  2,  3,  4, 12, 11},
                                        { 0,  2,  1,  8,  7, 10,  9,  4,  3,  6,  5, 12, 11},
                                        { 0, 11, 12,  4,  3,  9, 10,  8,  7,  5,  6,  1,  2},
                                        { 0, 10,  9,  5,  6,  3,  4,  8,  7,  2,  1, 12, 11},
                                        { 0,  8,  7,  4,  3, 11, 12,  2,  1, 10,  9,  5,  6},
                                        { 0,  3,  4,  1,  2,  7,  8,  5,  6, 10,  9, 12, 11},
                                        { 0,  2,  1, 10,  9, 12, 11,  8,  7,  4,  3,  6,  5},
                                        { 0, 12, 11,  7,  8,  6,  5,  3,  4, 10,  9,  2,  1},
                                        { 0,  4,  3,  2,  1,  6,  5,  8,  7, 11, 12,  9, 10},
                                        { 0,  2,  1,  6,  5,  4,  3, 12, 11, 10,  9,  8,  7},
                                        { 0,  5,  6, 11, 12,  1,  2,  8,  7, 10,  9,  3,  4},
                                        { 0,  6,  5,  4,  3,  2,  1,  9, 10,  7,  8, 12, 11},
};

//-----------------------------------------------------------------------------
// hcp
//-----------------------------------------------------------------------------

const int8_t mapping_hcp[NUM_HEX_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
                                        { 0,  2,  7,  4,  5,  3,  8,  1,  9,  6, 12, 10, 11},
                                        { 0,  7,  1,  5,  3,  4,  9,  2,  6,  8, 11, 12, 10},
                                        { 0,  6,  9, 10, 11, 12,  1,  8,  7,  2,  3,  4,  5},
                                        { 0,  8,  6, 12, 10, 11,  2,  9,  1,  7,  4,  5,  3},
                                        { 0,  9,  8, 11, 12, 10,  7,  6,  2,  1,  5,  3,  4},
};

const int8_t template_indices_hcp[NUM_CONVENTIONAL_HEX_MAPPINGS] = {0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1};

const int8_t mapping_hcp_conventional[NUM_CONVENTIONAL_HEX_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
                                        { 0,  2,  7,  4,  5,  3,  8,  1,  9,  6, 12, 10, 11},
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
                                        { 0,  2,  7,  4,  5,  3,  8,  1,  9,  6, 12, 10, 11},
                                        { 0,  7,  1,  5,  3,  4,  9,  2,  6,  8, 11, 12, 10},
                                        { 0,  8,  6, 12, 10, 11,  2,  9,  1,  7,  4,  5,  3},
                                        { 0,  6,  9, 10, 11, 12,  1,  8,  7,  2,  3,  4,  5},
                                        { 0,  8,  6, 12, 10, 11,  2,  9,  1,  7,  4,  5,  3},
                                        { 0,  6,  9, 10, 11, 12,  1,  8,  7,  2,  3,  4,  5},
                                        { 0,  9,  8, 11, 12, 10,  7,  6,  2,  1,  5,  3,  4},
                                        { 0,  9,  8, 11, 12, 10,  7,  6,  2,  1,  5,  3,  4},
                                        { 0,  7,  1,  5,  3,  4,  9,  2,  6,  8, 11, 12, 10},
};

const int8_t mapping_hcp_conventional_inverse[NUM_CONVENTIONAL_HEX_MAPPINGS][PTM_MAX_POINTS] = {
					{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
					{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
					{ 0,  7,  1,  5,  3,  4,  9,  2,  6,  8, 11, 12, 10},
					{ 0,  2,  7,  4,  5,  3,  8,  1,  9,  6, 12, 10, 11},
					{ 0,  7,  1,  5,  3,  4,  9,  2,  6,  8, 11, 12, 10},
					{ 0,  8,  6, 12, 10, 11,  2,  9,  1,  7,  4,  5,  3},
					{ 0,  8,  6, 12, 10, 11,  2,  9,  1,  7,  4,  5,  3},
					{ 0,  9,  8, 11, 12, 10,  7,  6,  2,  1,  5,  3,  4},
					{ 0,  6,  9, 10, 11, 12,  1,  8,  7,  2,  3,  4,  5},
					{ 0,  9,  8, 11, 12, 10,  7,  6,  2,  1,  5,  3,  4},
					{ 0,  6,  9, 10, 11, 12,  1,  8,  7,  2,  3,  4,  5},
					{ 0,  2,  7,  4,  5,  3,  8,  1,  9,  6, 12, 10, 11},
};


//-----------------------------------------------------------------------------
// dcub
//-----------------------------------------------------------------------------

const int8_t mapping_dcub[NUM_DCUB_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
                                        { 0,  1,  3,  4,  2,  7,  5,  6, 11, 13, 12, 14, 15, 16,  8, 10,  9},
                                        { 0,  4,  1,  3,  2, 16, 14, 15,  7,  6,  5, 12, 13, 11,  9,  8, 10},
                                        { 0,  2,  3,  1,  4,  8, 10,  9, 13, 12, 11,  6,  7,  5, 15, 16, 14},
                                        { 0,  4,  2,  1,  3, 14, 15, 16,  9, 10,  8,  7,  5,  6, 12, 13, 11},
                                        { 0,  3,  2,  4,  1, 12, 13, 11, 10,  8,  9, 16, 14, 15,  5,  6,  7},
                                        { 0,  3,  1,  2,  4, 13, 11, 12,  5,  7,  6, 10,  9,  8, 16, 14, 15},
                                        { 0,  2,  4,  3,  1, 10,  9,  8, 15, 14, 16, 13, 11, 12,  6,  7,  5},
                                        { 0,  1,  4,  2,  3,  6,  7,  5, 14, 16, 15,  8, 10,  9, 11, 12, 13},
                                        { 0,  2,  1,  4,  3,  9,  8, 10,  6,  5,  7, 15, 16, 14, 13, 11, 12},
                                        { 0,  4,  3,  2,  1, 15, 16, 14, 12, 11, 13,  9,  8, 10,  7,  5,  6},
                                        { 0,  3,  4,  1,  2, 11, 12, 13, 16, 15, 14,  5,  6,  7, 10,  9,  8},
};

const int8_t template_indices_dcub[NUM_CONVENTIONAL_DCUB_MAPPINGS] = {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0};

const int8_t mapping_dcub_conventional[NUM_CONVENTIONAL_DCUB_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
                                        { 0,  2,  1,  4,  3,  9,  8, 10,  6,  5,  7, 15, 16, 14, 13, 11, 12},
                                        { 0,  4,  1,  3,  2, 16, 14, 15,  7,  6,  5, 12, 13, 11,  9,  8, 10},
                                        { 0,  1,  3,  4,  2,  7,  5,  6, 11, 13, 12, 14, 15, 16,  8, 10,  9},
                                        { 0,  4,  2,  1,  3, 14, 15, 16,  9, 10,  8,  7,  5,  6, 12, 13, 11},
                                        { 0,  2,  3,  1,  4,  8, 10,  9, 13, 12, 11,  6,  7,  5, 15, 16, 14},
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
                                        { 0,  1,  3,  4,  2,  7,  5,  6, 11, 13, 12, 14, 15, 16,  8, 10,  9},
                                        { 0,  4,  1,  3,  2, 16, 14, 15,  7,  6,  5, 12, 13, 11,  9,  8, 10},
                                        { 0,  2,  3,  1,  4,  8, 10,  9, 13, 12, 11,  6,  7,  5, 15, 16, 14},
                                        { 0,  4,  2,  1,  3, 14, 15, 16,  9, 10,  8,  7,  5,  6, 12, 13, 11},
                                        { 0,  3,  2,  4,  1, 12, 13, 11, 10,  8,  9, 16, 14, 15,  5,  6,  7},
                                        { 0,  3,  1,  2,  4, 13, 11, 12,  5,  7,  6, 10,  9,  8, 16, 14, 15},
                                        { 0,  2,  4,  3,  1, 10,  9,  8, 15, 14, 16, 13, 11, 12,  6,  7,  5},
                                        { 0,  1,  4,  2,  3,  6,  7,  5, 14, 16, 15,  8, 10,  9, 11, 12, 13},
                                        { 0,  2,  1,  4,  3,  9,  8, 10,  6,  5,  7, 15, 16, 14, 13, 11, 12},
                                        { 0,  2,  4,  3,  1, 10,  9,  8, 15, 14, 16, 13, 11, 12,  6,  7,  5},
                                        { 0,  1,  4,  2,  3,  6,  7,  5, 14, 16, 15,  8, 10,  9, 11, 12, 13},
                                        { 0,  3,  2,  4,  1, 12, 13, 11, 10,  8,  9, 16, 14, 15,  5,  6,  7},
                                        { 0,  3,  1,  2,  4, 13, 11, 12,  5,  7,  6, 10,  9,  8, 16, 14, 15},
                                        { 0,  4,  3,  2,  1, 15, 16, 14, 12, 11, 13,  9,  8, 10,  7,  5,  6},
                                        { 0,  4,  3,  2,  1, 15, 16, 14, 12, 11, 13,  9,  8, 10,  7,  5,  6},
                                        { 0,  3,  4,  1,  2, 11, 12, 13, 16, 15, 14,  5,  6,  7, 10,  9,  8},
                                        { 0,  3,  4,  1,  2, 11, 12, 13, 16, 15, 14,  5,  6,  7, 10,  9,  8},
};

const int8_t mapping_dcub_conventional_inverse[NUM_CONVENTIONAL_DCUB_MAPPINGS][PTM_MAX_POINTS] = {
					{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
					{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
					{ 0,  3,  1,  2,  4, 13, 11, 12,  5,  7,  6, 10,  9,  8, 16, 14, 15},
					{ 0,  3,  2,  4,  1, 12, 13, 11, 10,  8,  9, 16, 14, 15,  5,  6,  7},
					{ 0,  1,  4,  2,  3,  6,  7,  5, 14, 16, 15,  8, 10,  9, 11, 12, 13},
					{ 0,  2,  4,  3,  1, 10,  9,  8, 15, 14, 16, 13, 11, 12,  6,  7,  5},
					{ 0,  2,  1,  4,  3,  9,  8, 10,  6,  5,  7, 15, 16, 14, 13, 11, 12},
					{ 0,  4,  1,  3,  2, 16, 14, 15,  7,  6,  5, 12, 13, 11,  9,  8, 10},
					{ 0,  4,  2,  1,  3, 14, 15, 16,  9, 10,  8,  7,  5,  6, 12, 13, 11},
					{ 0,  1,  3,  4,  2,  7,  5,  6, 11, 13, 12, 14, 15, 16,  8, 10,  9},
					{ 0,  2,  3,  1,  4,  8, 10,  9, 13, 12, 11,  6,  7,  5, 15, 16, 14},
					{ 0,  3,  1,  2,  4, 13, 11, 12,  5,  7,  6, 10,  9,  8, 16, 14, 15},
					{ 0,  1,  4,  2,  3,  6,  7,  5, 14, 16, 15,  8, 10,  9, 11, 12, 13},
					{ 0,  3,  2,  4,  1, 12, 13, 11, 10,  8,  9, 16, 14, 15,  5,  6,  7},
					{ 0,  2,  4,  3,  1, 10,  9,  8, 15, 14, 16, 13, 11, 12,  6,  7,  5},
					{ 0,  2,  1,  4,  3,  9,  8, 10,  6,  5,  7, 15, 16, 14, 13, 11, 12},
					{ 0,  4,  1,  3,  2, 16, 14, 15,  7,  6,  5, 12, 13, 11,  9,  8, 10},
					{ 0,  1,  3,  4,  2,  7,  5,  6, 11, 13, 12, 14, 15, 16,  8, 10,  9},
					{ 0,  4,  2,  1,  3, 14, 15, 16,  9, 10,  8,  7,  5,  6, 12, 13, 11},
					{ 0,  2,  3,  1,  4,  8, 10,  9, 13, 12, 11,  6,  7,  5, 15, 16, 14},
					{ 0,  3,  4,  1,  2, 11, 12, 13, 16, 15, 14,  5,  6,  7, 10,  9,  8},
					{ 0,  4,  3,  2,  1, 15, 16, 14, 12, 11, 13,  9,  8, 10,  7,  5,  6},
					{ 0,  3,  4,  1,  2, 11, 12, 13, 16, 15, 14,  5,  6,  7, 10,  9,  8},
					{ 0,  4,  3,  2,  1, 15, 16, 14, 12, 11, 13,  9,  8, 10,  7,  5,  6},
};


//-----------------------------------------------------------------------------
// dhex
//-----------------------------------------------------------------------------

const int8_t mapping_dhex[NUM_DHEX_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
                                        { 0,  3,  1,  2,  4, 12, 11, 13,  5,  6,  7,  9,  8, 10, 16, 14, 15},
                                        { 0,  2,  3,  1,  4,  8,  9, 10, 12, 11, 13,  6,  5,  7, 15, 16, 14},
};

const int8_t template_indices_dhex[NUM_CONVENTIONAL_DHEX_MAPPINGS] = {0, 1, 1, 0, 0, 3, 2, 2, 3, 3, 2, 1};

const int8_t mapping_dhex_conventional[NUM_CONVENTIONAL_DHEX_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
                                        { 0,  3,  1,  2,  4, 12, 11, 13,  5,  6,  7,  9,  8, 10, 16, 14, 15},
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
                                        { 0,  3,  1,  2,  4, 12, 11, 13,  5,  6,  7,  9,  8, 10, 16, 14, 15},
                                        { 0,  2,  3,  1,  4,  8,  9, 10, 12, 11, 13,  6,  5,  7, 15, 16, 14},
                                        { 0,  2,  3,  1,  4,  8,  9, 10, 12, 11, 13,  6,  5,  7, 15, 16, 14},
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
                                        { 0,  2,  3,  1,  4,  8,  9, 10, 12, 11, 13,  6,  5,  7, 15, 16, 14},
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
                                        { 0,  3,  1,  2,  4, 12, 11, 13,  5,  6,  7,  9,  8, 10, 16, 14, 15},
                                        { 0,  3,  1,  2,  4, 12, 11, 13,  5,  6,  7,  9,  8, 10, 16, 14, 15},
                                        { 0,  2,  3,  1,  4,  8,  9, 10, 12, 11, 13,  6,  5,  7, 15, 16, 14},
};

const int8_t mapping_dhex_conventional_inverse[NUM_CONVENTIONAL_DHEX_MAPPINGS][PTM_MAX_POINTS] = {
					{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
					{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
					{ 0,  2,  3,  1,  4,  8,  9, 10, 12, 11, 13,  6,  5,  7, 15, 16, 14},
					{ 0,  3,  1,  2,  4, 12, 11, 13,  5,  6,  7,  9,  8, 10, 16, 14, 15},
					{ 0,  2,  3,  1,  4,  8,  9, 10, 12, 11, 13,  6,  5,  7, 15, 16, 14},
					{ 0,  3,  1,  2,  4, 12, 11, 13,  5,  6,  7,  9,  8, 10, 16, 14, 15},
					{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
					{ 0,  3,  1,  2,  4, 12, 11, 13,  5,  6,  7,  9,  8, 10, 16, 14, 15},
					{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
					{ 0,  2,  3,  1,  4,  8,  9, 10, 12, 11, 13,  6,  5,  7, 15, 16, 14},
					{ 0,  2,  3,  1,  4,  8,  9, 10, 12, 11, 13,  6,  5,  7, 15, 16, 14},
					{ 0,  3,  1,  2,  4, 12, 11, 13,  5,  6,  7,  9,  8, 10, 16, 14, 15},
};

//-----------------------------------------------------------------------------
// graphene
//-----------------------------------------------------------------------------

const int8_t mapping_graphene[NUM_GRAPHENE_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
                                        { 0,  2,  3,  1,  6,  7,  8,  9,  4,  5},
                                        { 0,  3,  1,  2,  8,  9,  4,  5,  6,  7},
                                        { 0,  2,  1,  3,  7,  6,  5,  4,  9,  8},
                                        { 0,  3,  2,  1,  9,  8,  7,  6,  5,  4},
                                        { 0,  1,  3,  2,  5,  4,  9,  8,  7,  6},
};

const int8_t template_indices_graphene[NUM_CONVENTIONAL_GRAPHENE_MAPPINGS] = {0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1};

const int8_t mapping_graphene_conventional[NUM_CONVENTIONAL_GRAPHENE_MAPPINGS][PTM_MAX_POINTS] = {
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
                                        { 0,  2,  3,  1,  6,  7,  8,  9,  4,  5},
                                        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
                                        { 0,  2,  3,  1,  6,  7,  8,  9,  4,  5},
                                        { 0,  3,  1,  2,  8,  9,  4,  5,  6,  7},
                                        { 0,  3,  2,  1,  9,  8,  7,  6,  5,  4},
                                        { 0,  2,  1,  3,  7,  6,  5,  4,  9,  8},
                                        { 0,  3,  2,  1,  9,  8,  7,  6,  5,  4},
                                        { 0,  2,  1,  3,  7,  6,  5,  4,  9,  8},
                                        { 0,  1,  3,  2,  5,  4,  9,  8,  7,  6},
                                        { 0,  1,  3,  2,  5,  4,  9,  8,  7,  6},
                                        { 0,  3,  1,  2,  8,  9,  4,  5,  6,  7},
};

const int8_t mapping_graphene_conventional_inverse[NUM_CONVENTIONAL_GRAPHENE_MAPPINGS][PTM_MAX_POINTS] = {
					{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
					{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
					{ 0,  3,  1,  2,  8,  9,  4,  5,  6,  7},
					{ 0,  2,  3,  1,  6,  7,  8,  9,  4,  5},
					{ 0,  3,  1,  2,  8,  9,  4,  5,  6,  7},
					{ 0,  3,  2,  1,  9,  8,  7,  6,  5,  4},
					{ 0,  3,  2,  1,  9,  8,  7,  6,  5,  4},
					{ 0,  1,  3,  2,  5,  4,  9,  8,  7,  6},
					{ 0,  2,  1,  3,  7,  6,  5,  4,  9,  8},
					{ 0,  1,  3,  2,  5,  4,  9,  8,  7,  6},
					{ 0,  2,  1,  3,  7,  6,  5,  4,  9,  8},
					{ 0,  2,  3,  1,  6,  7,  8,  9,  4,  5},
};

}

#endif

