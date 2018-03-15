#ifndef FUNDAMENTAL_MAPPINGS_HPP
#define FUNDAMENTAL_MAPPINGS_HPP

#include <cstdint>

#define NUM_CUBIC_MAPPINGS 24
#define NUM_ICO_MAPPINGS 60
#define NUM_HEX_MAPPINGS 6
#define NUM_DCUB_MAPPINGS 12
#define NUM_DHEX_MAPPINGS 3

const int8_t mapping_sc[NUM_CUBIC_MAPPINGS][PTM_MAX_POINTS] = {
					{0, 1, 2, 3, 4, 5, 6},
					{0, 2, 1, 4, 3, 5, 6},
					{0, 2, 1, 3, 4, 6, 5},
					{0, 1, 2, 4, 3, 6, 5},
					{0, 3, 4, 5, 6, 1, 2},
					{0, 5, 6, 2, 1, 4, 3},
					{0, 6, 5, 1, 2, 4, 3},
					{0, 4, 3, 5, 6, 2, 1},
					{0, 5, 6, 1, 2, 3, 4},
					{0, 4, 3, 6, 5, 1, 2},
					{0, 3, 4, 6, 5, 2, 1},
					{0, 6, 5, 2, 1, 3, 4},
					{0, 3, 4, 2, 1, 5, 6},
					{0, 6, 5, 3, 4, 1, 2},
					{0, 1, 2, 5, 6, 4, 3},
					{0, 4, 3, 1, 2, 5, 6},
					{0, 5, 6, 3, 4, 2, 1},
					{0, 1, 2, 6, 5, 3, 4},
					{0, 2, 1, 5, 6, 3, 4},
					{0, 5, 6, 4, 3, 1, 2},
					{0, 3, 4, 1, 2, 6, 5},
					{0, 2, 1, 6, 5, 4, 3},
					{0, 6, 5, 4, 3, 2, 1},
					{0, 4, 3, 2, 1, 6, 5}	};

const int8_t mapping_fcc[NUM_CUBIC_MAPPINGS][PTM_MAX_POINTS] = {
					{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
					{0, 2, 1, 4, 3, 7, 8, 5, 6, 11, 12, 9, 10},
					{0, 3, 4, 1, 2, 6, 5, 8, 7, 12, 11, 10, 9},
					{0, 4, 3, 2, 1, 8, 7, 6, 5, 10, 9, 12, 11},
					{0, 9, 10, 11, 12, 1, 2, 4, 3, 5, 6, 8, 7},
					{0, 7, 8, 6, 5, 11, 12, 10, 9, 2, 1, 4, 3},
					{0, 8, 7, 5, 6, 10, 9, 11, 12, 4, 3, 2, 1},
					{0, 11, 12, 9, 10, 2, 1, 3, 4, 7, 8, 6, 5},
					{0, 5, 6, 8, 7, 9, 10, 12, 11, 1, 2, 3, 4},
					{0, 10, 9, 12, 11, 4, 3, 1, 2, 8, 7, 5, 6},
					{0, 12, 11, 10, 9, 3, 4, 2, 1, 6, 5, 7, 8},
					{0, 6, 5, 7, 8, 12, 11, 9, 10, 3, 4, 1, 2},
					{0, 3, 4, 2, 1, 9, 10, 11, 12, 7, 8, 5, 6},
					{0, 12, 11, 9, 10, 8, 7, 5, 6, 1, 2, 4, 3},
					{0, 5, 6, 7, 8, 4, 3, 2, 1, 11, 12, 10, 9},
					{0, 4, 3, 1, 2, 11, 12, 9, 10, 5, 6, 7, 8},
					{0, 9, 10, 12, 11, 7, 8, 6, 5, 3, 4, 2, 1},
					{0, 8, 7, 6, 5, 1, 2, 3, 4, 12, 11, 9, 10},
					{0, 7, 8, 5, 6, 3, 4, 1, 2, 9, 10, 12, 11},
					{0, 11, 12, 10, 9, 5, 6, 8, 7, 4, 3, 1, 2},
					{0, 1, 2, 4, 3, 12, 11, 10, 9, 8, 7, 6, 5},
					{0, 6, 5, 8, 7, 2, 1, 4, 3, 10, 9, 11, 12},
					{0, 10, 9, 11, 12, 6, 5, 7, 8, 2, 1, 3, 4},
					{0, 2, 1, 3, 4, 10, 9, 12, 11, 6, 5, 8, 7}	};

const int8_t mapping_bcc[NUM_CUBIC_MAPPINGS][PTM_MAX_POINTS] = {
					{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
					{0, 4, 3, 2, 1, 7, 8, 5, 6, 10, 9, 12, 11, 13, 14},
					{0, 6, 5, 7, 8, 2, 1, 3, 4, 10, 9, 11, 12, 14, 13},
					{0, 8, 7, 5, 6, 3, 4, 2, 1, 9, 10, 12, 11, 14, 13},
					{0, 1, 2, 7, 8, 3, 4, 5, 6, 11, 12, 13, 14, 9, 10},
					{0, 4, 3, 7, 8, 5, 6, 2, 1, 13, 14, 10, 9, 12, 11},
					{0, 8, 7, 3, 4, 2, 1, 5, 6, 14, 13, 9, 10, 12, 11},
					{0, 4, 3, 5, 6, 2, 1, 7, 8, 12, 11, 13, 14, 10, 9},
					{0, 1, 2, 5, 6, 7, 8, 3, 4, 13, 14, 9, 10, 11, 12},
					{0, 8, 7, 2, 1, 5, 6, 3, 4, 12, 11, 14, 13, 9, 10},
					{0, 6, 5, 3, 4, 7, 8, 2, 1, 11, 12, 14, 13, 10, 9},
					{0, 6, 5, 2, 1, 3, 4, 7, 8, 14, 13, 10, 9, 11, 12},
					{0, 7, 8, 6, 5, 1, 2, 4, 3, 11, 12, 10, 9, 13, 14},
					{0, 3, 4, 6, 5, 8, 7, 1, 2, 14, 13, 11, 12, 9, 10},
					{0, 5, 6, 1, 2, 8, 7, 4, 3, 9, 10, 13, 14, 12, 11},
					{0, 5, 6, 8, 7, 4, 3, 1, 2, 12, 11, 9, 10, 13, 14},
					{0, 7, 8, 1, 2, 4, 3, 6, 5, 13, 14, 11, 12, 10, 9},
					{0, 3, 4, 8, 7, 1, 2, 6, 5, 9, 10, 14, 13, 11, 12},
					{0, 7, 8, 4, 3, 6, 5, 1, 2, 10, 9, 13, 14, 11, 12},
					{0, 5, 6, 4, 3, 1, 2, 8, 7, 13, 14, 12, 11, 9, 10},
					{0, 3, 4, 1, 2, 6, 5, 8, 7, 11, 12, 9, 10, 14, 13},
					{0, 2, 1, 6, 5, 4, 3, 8, 7, 10, 9, 14, 13, 12, 11},
					{0, 2, 1, 8, 7, 6, 5, 4, 3, 14, 13, 12, 11, 10, 9},
					{0, 2, 1, 4, 3, 8, 7, 6, 5, 12, 11, 10, 9, 14, 13}	};

const int8_t mapping_ico[NUM_ICO_MAPPINGS][PTM_MAX_POINTS] = {
					{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
					{0, 10, 9, 8, 7, 5, 6, 2, 1, 12, 11, 3, 4},
					{0, 1, 2, 9, 10, 7, 8, 11, 12, 5, 6, 3, 4},
					{0, 4, 3, 8, 7, 2, 1, 11, 12, 9, 10, 6, 5},
					{0, 6, 5, 9, 10, 4, 3, 7, 8, 12, 11, 2, 1},
					{0, 12, 11, 3, 4, 7, 8, 10, 9, 2, 1, 6, 5},
					{0, 4, 3, 6, 5, 9, 10, 2, 1, 8, 7, 11, 12},
					{0, 8, 7, 2, 1, 4, 3, 10, 9, 5, 6, 11, 12},
					{0, 10, 9, 3, 4, 12, 11, 5, 6, 8, 7, 2, 1},
					{0, 12, 11, 6, 5, 2, 1, 7, 8, 3, 4, 10, 9},
					{0, 1, 2, 11, 12, 9, 10, 5, 6, 3, 4, 7, 8},
					{0, 8, 7, 11, 12, 5, 6, 4, 3, 2, 1, 10, 9},
					{0, 6, 5, 2, 1, 12, 11, 4, 3, 9, 10, 7, 8},
					{0, 3, 4, 5, 6, 1, 2, 10, 9, 12, 11, 7, 8},
					{0, 3, 4, 7, 8, 12, 11, 1, 2, 5, 6, 10, 9},
					{0, 6, 5, 7, 8, 9, 10, 12, 11, 2, 1, 4, 3},
					{0, 9, 10, 11, 12, 4, 3, 1, 2, 7, 8, 6, 5},
					{0, 11, 12, 9, 10, 1, 2, 4, 3, 8, 7, 5, 6},
					{0, 8, 7, 5, 6, 10, 9, 11, 12, 4, 3, 2, 1},
					{0, 10, 9, 2, 1, 8, 7, 12, 11, 3, 4, 5, 6},
					{0, 12, 11, 2, 1, 10, 9, 6, 5, 7, 8, 3, 4},
					{0, 9, 10, 6, 5, 7, 8, 4, 3, 11, 12, 1, 2},
					{0, 8, 7, 10, 9, 2, 1, 5, 6, 11, 12, 4, 3},
					{0, 6, 5, 12, 11, 7, 8, 2, 1, 4, 3, 9, 10},
					{0, 11, 12, 8, 7, 4, 3, 5, 6, 1, 2, 9, 10},
					{0, 4, 3, 11, 12, 8, 7, 9, 10, 6, 5, 2, 1},
					{0, 4, 3, 9, 10, 11, 12, 6, 5, 2, 1, 8, 7},
					{0, 12, 11, 10, 9, 3, 4, 2, 1, 6, 5, 7, 8},
					{0, 5, 6, 8, 7, 11, 12, 10, 9, 3, 4, 1, 2},
					{0, 7, 8, 6, 5, 12, 11, 9, 10, 1, 2, 3, 4},
					{0, 10, 9, 12, 11, 2, 1, 3, 4, 5, 6, 8, 7},
					{0, 7, 8, 1, 2, 9, 10, 3, 4, 12, 11, 6, 5},
					{0, 5, 6, 1, 2, 3, 4, 11, 12, 8, 7, 10, 9},
					{0, 7, 8, 12, 11, 3, 4, 6, 5, 9, 10, 1, 2},
					{0, 1, 2, 5, 6, 11, 12, 3, 4, 7, 8, 9, 10},
					{0, 11, 12, 1, 2, 5, 6, 9, 10, 4, 3, 8, 7},
					{0, 5, 6, 3, 4, 10, 9, 1, 2, 11, 12, 8, 7},
					{0, 5, 6, 10, 9, 8, 7, 3, 4, 1, 2, 11, 12},
					{0, 3, 4, 12, 11, 10, 9, 7, 8, 1, 2, 5, 6},
					{0, 9, 10, 7, 8, 1, 2, 6, 5, 4, 3, 11, 12},
					{0, 9, 10, 1, 2, 11, 12, 7, 8, 6, 5, 4, 3},
					{0, 7, 8, 3, 4, 1, 2, 12, 11, 6, 5, 9, 10},
					{0, 11, 12, 5, 6, 8, 7, 1, 2, 9, 10, 4, 3},
					{0, 1, 2, 7, 8, 3, 4, 9, 10, 11, 12, 5, 6},
					{0, 3, 4, 10, 9, 5, 6, 12, 11, 7, 8, 1, 2},
					{0, 2, 1, 4, 3, 8, 7, 6, 5, 12, 11, 10, 9},
					{0, 2, 1, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3},
					{0, 9, 10, 4, 3, 6, 5, 11, 12, 1, 2, 7, 8},
					{0, 11, 12, 4, 3, 9, 10, 8, 7, 5, 6, 1, 2},
					{0, 2, 1, 10, 9, 12, 11, 8, 7, 4, 3, 6, 5},
					{0, 5, 6, 11, 12, 1, 2, 8, 7, 10, 9, 3, 4},
					{0, 10, 9, 5, 6, 3, 4, 8, 7, 2, 1, 12, 11},
					{0, 12, 11, 7, 8, 6, 5, 3, 4, 10, 9, 2, 1},
					{0, 7, 8, 9, 10, 6, 5, 1, 2, 3, 4, 12, 11},
					{0, 2, 1, 8, 7, 10, 9, 4, 3, 6, 5, 12, 11},
					{0, 8, 7, 4, 3, 11, 12, 2, 1, 10, 9, 5, 6},
					{0, 6, 5, 4, 3, 2, 1, 9, 10, 7, 8, 12, 11},
					{0, 2, 1, 6, 5, 4, 3, 12, 11, 10, 9, 8, 7},
					{0, 3, 4, 1, 2, 7, 8, 5, 6, 10, 9, 12, 11},
					{0, 4, 3, 2, 1, 6, 5, 8, 7, 11, 12, 9, 10}	};

const int8_t mapping_hcp[NUM_HEX_MAPPINGS][PTM_MAX_POINTS] = {
					{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
					{0, 5, 6, 1, 2, 3, 4, 9, 10, 12, 11, 8, 7},
					{0, 3, 4, 5, 6, 1, 2, 12, 11, 7, 8, 10, 9},
					{0, 4, 3, 2, 1, 6, 5, 11, 12, 10, 9, 7, 8},
					{0, 2, 1, 6, 5, 4, 3, 8, 7, 11, 12, 9, 10},
					{0, 6, 5, 4, 3, 2, 1, 10, 9, 8, 7, 12, 11}	};

const int8_t mapping_dcub[NUM_DCUB_MAPPINGS][PTM_MAX_POINTS] = {
					{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
					{0, 2, 1, 4, 3, 9, 8, 10, 6, 5, 7, 14, 16, 15, 11, 13, 12},
					{0, 4, 3, 2, 1, 15, 16, 14, 12, 13, 11, 10, 8, 9, 7, 5, 6},
					{0, 3, 4, 1, 2, 13, 12, 11, 16, 15, 14, 7, 6, 5, 10, 9, 8},
					{0, 4, 2, 1, 3, 14, 15, 16, 9, 10, 8, 6, 5, 7, 12, 11, 13},
					{0, 4, 1, 3, 2, 16, 14, 15, 7, 6, 5, 13, 11, 12, 9, 8, 10},
					{0, 1, 4, 2, 3, 6, 7, 5, 14, 16, 15, 9, 10, 8, 13, 12, 11},
					{0, 3, 1, 2, 4, 11, 13, 12, 5, 7, 6, 8, 9, 10, 16, 14, 15},
					{0, 3, 2, 4, 1, 12, 11, 13, 10, 8, 9, 15, 14, 16, 5, 6, 7},
					{0, 2, 4, 3, 1, 10, 9, 8, 15, 14, 16, 12, 13, 11, 6, 7, 5},
					{0, 1, 3, 4, 2, 7, 5, 6, 13, 11, 12, 16, 15, 14, 8, 10, 9},
					{0, 2, 3, 1, 4, 8, 10, 9, 11, 12, 13, 5, 7, 6, 15, 16, 14}	};

const int8_t mapping_dhex[NUM_DHEX_MAPPINGS][PTM_MAX_POINTS] = {
					{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
					{0, 1, 3, 4, 2, 6, 7, 5, 11, 13, 12, 14, 16, 15, 8, 9, 10},
					{0, 1, 4, 2, 3, 7, 5, 6, 14, 15, 16, 8, 10, 9, 11, 13, 12}	};

#endif

