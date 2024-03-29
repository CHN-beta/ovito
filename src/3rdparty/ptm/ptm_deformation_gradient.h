/*Copyright (c) 2022 PM Larsen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PTM_DEFORMATION_GRADIENT_H
#define PTM_DEFORMATION_GRADIENT_H

#include <stdint.h>
#include "ptm_constants.h"

namespace ptm {

void calculate_deformation_gradient(int num_points, int8_t* mapping, double (*normalized)[3], const double (*penrose)[3], double* F);

const double penrose_sc[1][PTM_MAX_POINTS][3] = {{
        {    0,    0,    0 },
        {    0,    0, -0.5 },
        {    0,    0,  0.5 },
        {    0, -0.5,    0 },
        {    0,  0.5,    0 },
        { -0.5,    0,    0 },
        {  0.5,    0,    0 },
}};

const double penrose_fcc[1][PTM_MAX_POINTS][3] = {{
        {          0,          0,          0 },
        {  sqrt(2)/8,  sqrt(2)/8,          0 },
        {          0,  sqrt(2)/8,  sqrt(2)/8 },
        {  sqrt(2)/8,          0,  sqrt(2)/8 },
        { -sqrt(2)/8, -sqrt(2)/8,          0 },
        {          0, -sqrt(2)/8, -sqrt(2)/8 },
        { -sqrt(2)/8,          0, -sqrt(2)/8 },
        { -sqrt(2)/8,  sqrt(2)/8,          0 },
        {          0, -sqrt(2)/8,  sqrt(2)/8 },
        { -sqrt(2)/8,          0,  sqrt(2)/8 },
        {  sqrt(2)/8, -sqrt(2)/8,          0 },
        {          0,  sqrt(2)/8, -sqrt(2)/8 },
        {  sqrt(2)/8,          0, -sqrt(2)/8 },
}};

const double penrose_hcp[2][PTM_MAX_POINTS][3] = {{
        {           0,           0,           0 },
        {        1./8,  -sqrt(3)/8,           0 },
        {       -1./4,           0,           0 },
        {       -1./8,  sqrt(3)/24, -sqrt(6)/12 },
        {        1./8,  sqrt(3)/24, -sqrt(6)/12 },
        {           0, -sqrt(3)/12, -sqrt(6)/12 },
        {       -1./8,   sqrt(3)/8,           0 },
        {        1./8,   sqrt(3)/8,           0 },
        {        1./4,           0,           0 },
        {       -1./8,  -sqrt(3)/8,           0 },
        {           0, -sqrt(3)/12,  sqrt(6)/12 },
        {        1./8,  sqrt(3)/24,  sqrt(6)/12 },
        {       -1./8,  sqrt(3)/24,  sqrt(6)/12 }, },

{
        {           0,           0,           0 },
        {        1./4,           0,           0 },
        {       -1./8,  -sqrt(3)/8,           0 },
        {       -1./8, -sqrt(3)/24, -sqrt(6)/12 },
        {           0,  sqrt(3)/12, -sqrt(6)/12 },
        {        1./8, -sqrt(3)/24, -sqrt(6)/12 },
        {       -1./4,           0,           0 },
        {       -1./8,   sqrt(3)/8,           0 },
        {        1./8,   sqrt(3)/8,           0 },
        {        1./8,  -sqrt(3)/8,           0 },
        {        1./8, -sqrt(3)/24,  sqrt(6)/12 },
        {           0,  sqrt(3)/12,  sqrt(6)/12 },
        {       -1./8, -sqrt(3)/24,  sqrt(6)/12 },
}};

const double penrose_ico[1][PTM_MAX_POINTS][3] = {{
        {                          0,                          0,                          0 },
        {                          0,                          0,                       0.25 },
        {                          0,                          0,                      -0.25 },
        { -sqrt(-10*sqrt(5) + 50)/40,          sqrt(5)/40 + 1./8,                -sqrt(5)/20 },
        {  sqrt(-10*sqrt(5) + 50)/40,         -1./8 - sqrt(5)/40,                 sqrt(5)/20 },
        {                          0,                -sqrt(5)/10,                -sqrt(5)/20 },
        {                          0,                 sqrt(5)/10,                 sqrt(5)/20 },
        {   sqrt(10*sqrt(5) + 50)/40,         -1./8 + sqrt(5)/40,                -sqrt(5)/20 },
        {  -sqrt(10*sqrt(5) + 50)/40,         -sqrt(5)/40 + 1./8,                 sqrt(5)/20 },
        {  -sqrt(10*sqrt(5) + 50)/40,         -1./8 + sqrt(5)/40,                -sqrt(5)/20 },
        {   sqrt(10*sqrt(5) + 50)/40,         -sqrt(5)/40 + 1./8,                 sqrt(5)/20 },
        {  sqrt(-10*sqrt(5) + 50)/40,          sqrt(5)/40 + 1./8,                -sqrt(5)/20 },
        { -sqrt(-10*sqrt(5) + 50)/40,         -1./8 - sqrt(5)/40,                 sqrt(5)/20 },
}};

const double penrose_bcc[1][PTM_MAX_POINTS][3] = {{
        {                  0,                  0,                   0 },
        {  3./56 + sqrt(3)/28,  3./56 + sqrt(3)/28,  3./56 + sqrt(3)/28 },
        { -sqrt(3)/28 - 3./56,  3./56 + sqrt(3)/28,  3./56 + sqrt(3)/28 },
        {  3./56 + sqrt(3)/28,  3./56 + sqrt(3)/28, -sqrt(3)/28 - 3./56 },
        { -sqrt(3)/28 - 3./56, -sqrt(3)/28 - 3./56,  3./56 + sqrt(3)/28 },
        {  3./56 + sqrt(3)/28, -sqrt(3)/28 - 3./56,  3./56 + sqrt(3)/28 },
        { -sqrt(3)/28 - 3./56,  3./56 + sqrt(3)/28, -sqrt(3)/28 - 3./56 },
        { -sqrt(3)/28 - 3./56, -sqrt(3)/28 - 3./56, -sqrt(3)/28 - 3./56 },
        {  3./56 + sqrt(3)/28, -sqrt(3)/28 - 3./56, -sqrt(3)/28 - 3./56 },
        {  3./28 + sqrt(3)/14,                   0,                   0 },
        { -sqrt(3)/14 - 3./28,                   0,                   0 },
        {                   0,  3./28 + sqrt(3)/14,                   0 },
        {                   0, -sqrt(3)/14 - 3./28,                   0 },
        {                   0,                   0,  3./28 + sqrt(3)/14 },
        {                   0,                   0, -sqrt(3)/14 - 3./28 },
}};

const double penrose_dcub[2][PTM_MAX_POINTS][3] = {{
        {                               0,                                0,                                0 },
        {  23./(48*(-sqrt(3) + 6*sqrt(2))),  23./(48*(-sqrt(3) + 6*sqrt(2))),  23./(48*(-sqrt(3) + 6*sqrt(2))) },
        {  23./(48*(-sqrt(3) + 6*sqrt(2))), -23./(-48*sqrt(3) + 288*sqrt(2)), -23./(-48*sqrt(3) + 288*sqrt(2)) },
        { -23./(-48*sqrt(3) + 288*sqrt(2)), -23./(-48*sqrt(3) + 288*sqrt(2)),  23./(48*(-sqrt(3) + 6*sqrt(2))) },
        { -23./(-48*sqrt(3) + 288*sqrt(2)),  23./(48*(-sqrt(3) + 6*sqrt(2))), -23./(-48*sqrt(3) + 288*sqrt(2)) },
        {  23./(24*(-sqrt(3) + 6*sqrt(2))),  23./(24*(-sqrt(3) + 6*sqrt(2))),                                0 },
        {                                0,  23./(24*(-sqrt(3) + 6*sqrt(2))),  23./(24*(-sqrt(3) + 6*sqrt(2))) },
        {  23./(24*(-sqrt(3) + 6*sqrt(2))),                                0,  23./(24*(-sqrt(3) + 6*sqrt(2))) },
        {                                0, -23./(-24*sqrt(3) + 144*sqrt(2)), -23./(-24*sqrt(3) + 144*sqrt(2)) },
        {  23./(24*(-sqrt(3) + 6*sqrt(2))), -23./(-24*sqrt(3) + 144*sqrt(2)),                                0 },
        {  23./(24*(-sqrt(3) + 6*sqrt(2))),                               0, -23./(-24*sqrt(3) + 144*sqrt(2)) },
        { -23./(-24*sqrt(3) + 144*sqrt(2)), -23./(-24*sqrt(3) + 144*sqrt(2)),                                0 },
        {                                0, -23./(-24*sqrt(3) + 144*sqrt(2)),  23./(24*(-sqrt(3) + 6*sqrt(2))) },
        { -23./(-24*sqrt(3) + 144*sqrt(2)),                                0,  23./(24*(-sqrt(3) + 6*sqrt(2))) },
        { -23./(-24*sqrt(3) + 144*sqrt(2)),                                0, -23./(-24*sqrt(3) + 144*sqrt(2)) },
        { -23./(-24*sqrt(3) + 144*sqrt(2)),  23./(24*(-sqrt(3) + 6*sqrt(2))),                                0 },
        {                                0,  23./(24*(-sqrt(3) + 6*sqrt(2))), -23./(-24*sqrt(3) + 144*sqrt(2)) }, },

{
        {                                0,                                0,                                0 },
        {  23./(48*(-sqrt(3) + 6*sqrt(2))), -23./(-48*sqrt(3) + 288*sqrt(2)),  23./(48*(-sqrt(3) + 6*sqrt(2))) },
        {  23./(48*(-sqrt(3) + 6*sqrt(2))),  23./(48*(-sqrt(3) + 6*sqrt(2))), -23./(-48*sqrt(3) + 288*sqrt(2)) },
        { -23./(-48*sqrt(3) + 288*sqrt(2)), -23./(-48*sqrt(3) + 288*sqrt(2)), -23./(-48*sqrt(3) + 288*sqrt(2)) },
        { -23./(-48*sqrt(3) + 288*sqrt(2)),  23./(48*(-sqrt(3) + 6*sqrt(2))),  23./(48*(-sqrt(3) + 6*sqrt(2))) },
        {  23./(24*(-sqrt(3) + 6*sqrt(2))),                                0,  23./(24*(-sqrt(3) + 6*sqrt(2))) },
        {                                0, -23./(-24*sqrt(3) + 144*sqrt(2)),  23./(24*(-sqrt(3) + 6*sqrt(2))) },
        {  23./(24*(-sqrt(3) + 6*sqrt(2))), -23./(-24*sqrt(3) + 144*sqrt(2)),                                0 },
        {                                0,  23./(24*(-sqrt(3) + 6*sqrt(2))), -23./(-24*sqrt(3) + 144*sqrt(2)) },
        {  23./(24*(-sqrt(3) + 6*sqrt(2))),                                0, -23./(-24*sqrt(3) + 144*sqrt(2)) },
        {  23./(24*(-sqrt(3) + 6*sqrt(2))),  23./(24*(-sqrt(3) + 6*sqrt(2))),                                0 },
        { -23./(-24*sqrt(3) + 144*sqrt(2)),                                0, -23./(-24*sqrt(3) + 144*sqrt(2)) },
        {                                0, -23./(-24*sqrt(3) + 144*sqrt(2)), -23./(-24*sqrt(3) + 144*sqrt(2)) },
        { -23./(-24*sqrt(3) + 144*sqrt(2)), -23./(-24*sqrt(3) + 144*sqrt(2)),                                0 },
        { -23./(-24*sqrt(3) + 144*sqrt(2)),  23./(24*(-sqrt(3) + 6*sqrt(2))),                                0 },
        { -23./(-24*sqrt(3) + 144*sqrt(2)),                                0,  23./(24*(-sqrt(3) + 6*sqrt(2))) },
        {                                0,  23./(24*(-sqrt(3) + 6*sqrt(2))),  23./(24*(-sqrt(3) + 6*sqrt(2))) },
}};


const double penrose_dhex[4][PTM_MAX_POINTS][3] = {{
        {                                        0,                                        0,                                        0 },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)),  23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(3)/(-144*sqrt(3) + 864*sqrt(2)) },
        {                                        0,  -23*sqrt(6)/(-72*sqrt(3) + 432*sqrt(2)), -23*sqrt(3)/(-144*sqrt(3) + 864*sqrt(2)) },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))),  23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(3)/(-144*sqrt(3) + 864*sqrt(2)) },
        {                                        0,                                        0,   23*sqrt(3)/(48*(-sqrt(3) + 6*sqrt(2))) },
        {  -23*sqrt(2)/(-24*sqrt(3) + 144*sqrt(2)),                                        0,                                        0 },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)),  23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))),  -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)),   23*sqrt(6)/(48*(-sqrt(3) + 6*sqrt(2))),                                        0 },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))),  -23*sqrt(6)/(-48*sqrt(3) + 288*sqrt(2)),                                        0 },
        {                                        0,  -23*sqrt(6)/(-72*sqrt(3) + 432*sqrt(2)),  -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)),  -23*sqrt(6)/(-48*sqrt(3) + 288*sqrt(2)),                                        0 },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))),  23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))),  -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))),   23*sqrt(6)/(48*(-sqrt(3) + 6*sqrt(2))),                                        0 },
        {   23*sqrt(2)/(24*(-sqrt(3) + 6*sqrt(2))),                                        0,                                        0 },
        {                                        0,  -23*sqrt(6)/(-72*sqrt(3) + 432*sqrt(2)),   23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))),  23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))),   23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)),  23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))),   23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) }, },

{
        {                                        0,                                        0,                                        0 },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)), -23*sqrt(3)/(-144*sqrt(3) + 864*sqrt(2)) },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)), -23*sqrt(3)/(-144*sqrt(3) + 864*sqrt(2)) },
        {                                        0,   23*sqrt(6)/(72*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(3)/(-144*sqrt(3) + 864*sqrt(2)) },
        {                                        0,                                        0,   23*sqrt(3)/(48*(-sqrt(3) + 6*sqrt(2))) },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)),  -23*sqrt(6)/(-48*sqrt(3) + 288*sqrt(2)),                                        0 },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)),  -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
        {  -23*sqrt(2)/(-24*sqrt(3) + 144*sqrt(2)),                                        0,                                        0 },
        {   23*sqrt(2)/(24*(-sqrt(3) + 6*sqrt(2))),                                        0,                                        0 },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)),  -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))),  -23*sqrt(6)/(-48*sqrt(3) + 288*sqrt(2)),                                        0 },
        {                                        0,   23*sqrt(6)/(72*(-sqrt(3) + 6*sqrt(2))),  -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)),   23*sqrt(6)/(48*(-sqrt(3) + 6*sqrt(2))),                                        0 },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))),   23*sqrt(6)/(48*(-sqrt(3) + 6*sqrt(2))),                                        0 },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)),   23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) },
        {                                        0,   23*sqrt(6)/(72*(-sqrt(3) + 6*sqrt(2))),   23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)),   23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) }, },

{
        {                                       0,                                       0,                                       0 },
        {                                       0, -23*sqrt(6)/(-72*sqrt(3) + 432*sqrt(2)), 23*sqrt(3)/(144*(-sqrt(3) + 6*sqrt(2))) },
        { -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)), 23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))), 23*sqrt(3)/(144*(-sqrt(3) + 6*sqrt(2))) },
        {  23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))), 23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))), 23*sqrt(3)/(144*(-sqrt(3) + 6*sqrt(2))) },
        {                                       0,                                       0, -23*sqrt(3)/(-48*sqrt(3) + 288*sqrt(2)) },
        { -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)), -23*sqrt(6)/(-48*sqrt(3) + 288*sqrt(2)),                                       0 },
        {                                       0, -23*sqrt(6)/(-72*sqrt(3) + 432*sqrt(2)),  23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) },
        {  23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(6)/(-48*sqrt(3) + 288*sqrt(2)),                                       0 },
        { -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)),  23*sqrt(6)/(48*(-sqrt(3) + 6*sqrt(2))),                                       0 },
        { -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)), 23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))),  23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) },
        { -23*sqrt(2)/(-24*sqrt(3) + 144*sqrt(2)),                                       0,                                       0 },
        {  23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))), 23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))),  23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) },
        {  23*sqrt(2)/(24*(-sqrt(3) + 6*sqrt(2))),                                       0,                                       0 },
        {  23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))),  23*sqrt(6)/(48*(-sqrt(3) + 6*sqrt(2))),                                       0 },
        { -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)), 23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
        {  23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))), 23*sqrt(6)/(144*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
        {                                       0, -23*sqrt(6)/(-72*sqrt(3) + 432*sqrt(2)), -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) }, },

{
        {                                        0,                                        0,                                        0 },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)),  23*sqrt(3)/(144*(-sqrt(3) + 6*sqrt(2))) },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)),  23*sqrt(3)/(144*(-sqrt(3) + 6*sqrt(2))) },
        {                                        0,   23*sqrt(6)/(72*(-sqrt(3) + 6*sqrt(2))),  23*sqrt(3)/(144*(-sqrt(3) + 6*sqrt(2))) },
        {                                        0,                                        0,  -23*sqrt(3)/(-48*sqrt(3) + 288*sqrt(2)) },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))),  -23*sqrt(6)/(-48*sqrt(3) + 288*sqrt(2)),                                        0 },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)),   23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) },
        {   23*sqrt(2)/(24*(-sqrt(3) + 6*sqrt(2))),                                        0,                                        0 },
        {  -23*sqrt(2)/(-24*sqrt(3) + 144*sqrt(2)),                                        0,                                        0 },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)),   23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)),  -23*sqrt(6)/(-48*sqrt(3) + 288*sqrt(2)),                                        0 },
        {                                        0,   23*sqrt(6)/(72*(-sqrt(3) + 6*sqrt(2))),   23*sqrt(3)/(36*(-sqrt(3) + 6*sqrt(2))) },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))),   23*sqrt(6)/(48*(-sqrt(3) + 6*sqrt(2))),                                        0 },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)),   23*sqrt(6)/(48*(-sqrt(3) + 6*sqrt(2))),                                        0 },
        {  -23*sqrt(2)/(-48*sqrt(3) + 288*sqrt(2)), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)),  -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
        {                                        0,   23*sqrt(6)/(72*(-sqrt(3) + 6*sqrt(2))),  -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
        {   23*sqrt(2)/(48*(-sqrt(3) + 6*sqrt(2))), -23*sqrt(6)/(-144*sqrt(3) + 864*sqrt(2)),  -23*sqrt(3)/(-36*sqrt(3) + 216*sqrt(2)) },
}};

const double penrose_graphene[2][PTM_MAX_POINTS][3] = {{
        {                     0,                     0,                   0 },
        {                     0,  2./63 + 4*sqrt(3)/63,                   0 },
        {    sqrt(3)/63 + 2./21, -2*sqrt(3)/63 - 1./63,                   0 },
        {   -2./21 - sqrt(3)/63, -2*sqrt(3)/63 - 1./63,                   0 },
        {   -2./21 - sqrt(3)/63,  1./21 + 2*sqrt(3)/21,                   0 },
        {    sqrt(3)/63 + 2./21,  1./21 + 2*sqrt(3)/21,                   0 },
        {  2*sqrt(3)/63 + 4./21,                     0,                   0 },
        {    sqrt(3)/63 + 2./21, -2*sqrt(3)/21 - 1./21,                   0 },
        {   -2./21 - sqrt(3)/63, -2*sqrt(3)/21 - 1./21,                   0 },
        { -4./21 - 2*sqrt(3)/63,                     0,                   0 }, },

{
        {                     0,                     0,                    0 },
        {   -2./21 - sqrt(3)/63,  1./63 + 2*sqrt(3)/63,                    0 },
        {    sqrt(3)/63 + 2./21,  1./63 + 2*sqrt(3)/63,                    0 },
        {                     0, -4*sqrt(3)/63 - 2./63,                    0 },
        { -4./21 - 2*sqrt(3)/63,                     0,                    0 },
        {   -2./21 - sqrt(3)/63,  1./21 + 2*sqrt(3)/21,                    0 },
        {    sqrt(3)/63 + 2./21,  1./21 + 2*sqrt(3)/21,                    0 },
        {  2*sqrt(3)/63 + 4./21,                     0,                    0 },
        {    sqrt(3)/63 + 2./21, -2*sqrt(3)/21 - 1./21,                    0 },
        {   -2./21 - sqrt(3)/63, -2*sqrt(3)/21 - 1./21,                    0 },
}};

}

#endif

