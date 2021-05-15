/*Copyright (c) 2021 PM Larsen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PTM_INITIALIZE_DATA_H
#define PTM_INITIALIZE_DATA_H


#include "ptm_graph_data.h"
#include "ptm_graph_tools.h"
#include "ptm_deformation_gradient.h"
#include "ptm_fundamental_mappings.h"
#include "ptm_neighbour_ordering.h"
#include "ptm_canonical_coloured.h"
#include "ptm_convex_hull_incremental.h"
#include "ptm_templates.h"
#include "ptm_scaled_templates.h"
#include "ptm_quat.h"


namespace ptm {

typedef struct
{
	int type;
	int num_nbrs;
	int num_facets;
	int max_degree;
	int num_graphs;
	graph_t* graphs;
	const double (*points)[3];
	const double (*points_alt1)[3];
	const double (*points_alt2)[3];
	const double (*points_alt3)[3];
	const double (*penrose)[3];
	const double (*penrose_alt1)[3];
	const double (*penrose_alt2)[3];
	const double (*penrose_alt3)[3];
	const int8_t (*scaled)[3];
	const int8_t (*scaled_alt1)[3];
	const int8_t (*scaled_alt2)[3];
	const int8_t (*scaled_alt3)[3];
	int num_mappings;
	const int8_t (*mapping)[PTM_MAX_POINTS];
	int num_conventional_mappings;
	const int8_t (*mapping_conventional)[PTM_MAX_POINTS];
	const int8_t (*mapping_conventional_inverse)[PTM_MAX_POINTS];
	const int8_t *template_indices;
	const double (*qconventional)[4];
} refdata_t;


const refdata_t structure_sc = {	PTM_MATCH_SC,			//.type
					6,				//.num_nbrs
					8,				//.num_facets
					4,				//.max_degree
					NUM_SC_GRAPHS,			//.num_graphs
					graphs_sc,			//.graphs
					ptm_template_sc,		//.points
					NULL,				//.points_alt1
					NULL,				//.points_alt2
					NULL,				//.points_alt3
					penrose_sc,			//.penrose
					NULL,				//.penrose_alt1
					NULL,				//.penrose_alt2
					NULL,				//.penrose_alt3
					ptm_scaled_template_sc,		//.scaled
					NULL,				//.scaled_alt1
					NULL,				//.scaled_alt2
					NULL,				//.scaled_alt3
					NUM_CUBIC_MAPPINGS,		//.num_mappings
					mapping_sc,			//.mapping
					0,				//.num_conventional_mappings
					NULL,				//.mapping_conventional
					NULL,				//.mapping_conventional_inverse
					NULL,				//.template_indices
					NULL,				//.qconventional
				};

const refdata_t structure_fcc = {	PTM_MATCH_FCC,			//.type
					12,				//.num_nbrs
					20,				//.num_facets
					6,				//.max_degree
					NUM_FCC_GRAPHS,			//.num_graphs
					graphs_fcc,			//.graphs
					ptm_template_fcc,		//.points
					NULL,				//.points_alt1
					NULL,				//.points_alt2
					NULL,				//.points_alt3
					penrose_fcc,			//.penrose
					NULL,				//.penrose_alt1
					NULL,				//.penrose_alt2
					NULL,				//.penrose_alt3
					ptm_scaled_template_fcc,		//.scaled
					NULL,				//.scaled_alt1
					NULL,				//.scaled_alt2
					NULL,				//.scaled_alt3
					NUM_CUBIC_MAPPINGS,		//.num_mappings
					mapping_fcc,			//.mapping
					0,				//.num_conventional_mappings
					NULL,				//.mapping_conventional
					NULL,				//.mapping_conventional_inverse
					NULL,				//.template_indices
					NULL,				//.qconventional
				};

const refdata_t structure_hcp = {	PTM_MATCH_HCP,				//.type
					12,					//.num_nbrs
					20,					//.num_facets
					6,					//.max_degree
					NUM_HCP_GRAPHS,				//.num_graphs
					graphs_hcp,				//.graphs
					ptm_template_hcp,			//.points
					ptm_template_hcp_alt1,			//.points_alt1
					NULL,					//.points_alt2
					NULL,					//.points_alt3
					penrose_hcp,				//.penrose
					penrose_hcp_alt1,			//.penrose_alt1
					NULL,					//.penrose_alt2
					NULL,					//.penrose_alt3
					ptm_scaled_template_hcp,		//.scaled
					ptm_scaled_template_hcp_alt1,				//.scaled_alt1
					NULL,				//.scaled_alt2
					NULL,				//.scaled_alt3
					NUM_HEX_MAPPINGS,			//.num_mappings
					mapping_hcp,				//.mapping
					NUM_CONVENTIONAL_HEX_MAPPINGS,		//.num_conventional_mappings
					mapping_hcp_conventional,		//.mapping_conventional
					mapping_hcp_conventional_inverse,	//.mapping_conventional_inverse
					template_indices_hcp,			//.template_indices
					ptm::generator_hcp_conventional,	//.qconventional
				};

const refdata_t structure_ico = {	PTM_MATCH_ICO,			//.type
					12,				//.num_nbrs
					20,				//.num_facets
					6,				//.max_degree
					NUM_ICO_GRAPHS,			//.num_graphs
					graphs_ico,			//.graphs
					ptm_template_ico,		//.points
					NULL,				//.points_alt1
					NULL,				//.points_alt2
					NULL,				//.points_alt3
					penrose_ico,			//.penrose
					NULL,				//.penrose_alt1
					NULL,				//.penrose_alt2
					NULL,				//.penrose_alt3
					NULL,       		//.scaled
					NULL,				//.scaled_alt1
					NULL,				//.scaled_alt2
					NULL,				//.scaled_alt3
					NUM_ICO_MAPPINGS,		//.num_mappings
					mapping_ico,			//.mapping
					0,				//.num_conventional_mappings
					NULL,				//.mapping_conventional
					NULL,				//.mapping_conventional_inverse
					NULL,				//.template_indices
					NULL,				//.qconventional
				};

const refdata_t structure_bcc = {	PTM_MATCH_BCC,			//.type
					14,				//.num_nbrs
					24,				//.num_facets
					8,				//.max_degree
					NUM_BCC_GRAPHS,			//.num_graphs
					graphs_bcc,			//.graphs
					ptm_template_bcc,		//.points
					NULL,				//.points_alt1
					NULL,				//.points_alt2
					NULL,				//.points_alt3
					penrose_bcc,			//.penrose
					NULL,				//.penrose_alt1
					NULL,				//.penrose_alt2
					NULL,				//.penrose_alt3
					ptm_scaled_template_bcc,		//.scaled
					NULL,				//.scaled_alt1
					NULL,				//.scaled_alt2
					NULL,				//.scaled_alt3
					NUM_CUBIC_MAPPINGS,		//.num_mappings
					mapping_bcc,			//.mapping
					0,				//.num_conventional_mappings
					NULL,				//.mapping_conventional
					NULL,				//.mapping_conventional_inverse
					NULL,				//.template_indices
					NULL,				//.qconventional
				};

const refdata_t structure_dcub = {	PTM_MATCH_DCUB,				//.type
					16,					//.num_nbrs
					28,					//.num_facets
					8,					//.max_degree
					NUM_DCUB_GRAPHS,			//.num_graphs
					graphs_dcub,				//.graphs
					ptm_template_dcub,			//.points
					ptm_template_dcub_alt1,			//.points_alt1
					NULL,					//.points_alt2
					NULL,					//.points_alt3
					penrose_dcub,				//.penrose
					penrose_dcub_alt1,			//.penrose_alt1
					NULL,					//.penrose_alt2
					NULL,					//.penrose_alt3
					ptm_scaled_template_dcub,		//.scaled
					ptm_scaled_template_dcub_alt1,				//.scaled_alt1
					NULL,				//.scaled_alt2
					NULL,				//.scaled_alt3
					NUM_DCUB_MAPPINGS,			//.num_mappings
					mapping_dcub,				//.mapping
					NUM_CONVENTIONAL_DCUB_MAPPINGS,		//.num_conventional_mappings
					mapping_dcub_conventional,		//.mapping_conventional
					mapping_dcub_conventional_inverse,	//.mapping_conventional_inverse
					template_indices_dcub,			//.template_indices
					generator_cubic,			//.qconventional
				};

const refdata_t structure_dhex = {	PTM_MATCH_DHEX,				//.type
					16,					//.num_nbrs
					28,					//.num_facets
					8,					//.max_degree
					NUM_DHEX_GRAPHS,			//.num_graphs
					graphs_dhex,				//.graphs
					ptm_template_dhex,			//.points
					ptm_template_dhex_alt1,			//.points_alt1
					ptm_template_dhex_alt2,			//.points_alt2
					ptm_template_dhex_alt3,			//.points_alt3
					penrose_dhex,				//.penrose
					penrose_dhex_alt1,			//.penrose_alt1
					penrose_dhex_alt2,			//.penrose_alt2
					penrose_dhex_alt3,			//.penrose_alt3
					ptm_scaled_template_dhex,		//.scaled
					ptm_scaled_template_dhex_alt1,				//.scaled_alt1
					ptm_scaled_template_dhex_alt2,				//.scaled_alt2
					ptm_scaled_template_dhex_alt3,				//.scaled_alt3
					NUM_DHEX_MAPPINGS,			//.num_mappings
					mapping_dhex,				//.mapping
					NUM_CONVENTIONAL_DHEX_MAPPINGS,		//.num_conventional_mappings
					mapping_dhex_conventional,		//.mapping_conventional
					mapping_dhex_conventional_inverse,	//.mapping_conventional_inverse
					template_indices_dhex,			//.template_indices
					generator_hcp_conventional,		//.qconventional
				};

const refdata_t structure_graphene = {	PTM_MATCH_GRAPHENE,			//.type
					9,					//.num_nbrs
					-1,					//.num_facets
					-1,					//.max_degree
					-1,					//.num_graphs
					NULL,					//.graphs
					ptm_template_graphene,			//.points
					ptm_template_graphene_alt1,		//.points_alt1
					NULL,					//.points_alt2
					NULL,					//.points_alt3
					penrose_graphene,			//.penrose
					penrose_graphene_alt1,			//.penrose_alt1
					NULL,					//.penrose_alt2
					NULL,					//.penrose_alt3
					ptm_scaled_template_graphene,       		//.scaled
					ptm_scaled_template_graphene_alt1,				//.scaled_alt1
					NULL,				//.scaled_alt2
					NULL,				//.scaled_alt3
					-1,					//.num_mappings
					mapping_graphene,			//.mapping
					NUM_CONVENTIONAL_GRAPHENE_MAPPINGS,	//.num_conventional_mappings
					mapping_graphene_conventional,		//.mapping_conventional
					mapping_graphene_conventional_inverse,	//.mapping_conventional_inverse
					template_indices_graphene,		//.template_indices
					generator_hcp_conventional,		//.qconventional
				};

const refdata_t* const refdata[] = {	NULL,
					&structure_fcc,
					&structure_hcp,
					&structure_bcc,
					&structure_ico,
					&structure_sc,
					&structure_dcub,
					&structure_dhex,
					&structure_graphene	};
}

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ptm_local_handle* ptm_local_handle_t;
ptm_local_handle_t ptm_initialize_local();
void ptm_uninitialize_local(ptm_local_handle_t ptr);
int ptm_initialize_global();

//------------------------------------
//    global initialization switch
//------------------------------------
extern bool ptm_initialized;


#ifdef __cplusplus
}
#endif


#endif

