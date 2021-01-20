/*
	qf_bsp.h

	Vulkan specific brush model stuff

	Copyright (C) 2012 Bill Currie <bill@taniwha.org>
	Copyright (C) 2021 Bill Currie <bill@taniwha.org>

	Author: Bill Currie <bill@taniwha.org>
	Date: 2012/1/7
	Date: 2021/1/18

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

*/
#ifndef __QF_Vulkan_qf_bsp_h
#define __QF_Vulkan_qf_bsp_h

#include "QF/darray.h"
#include "QF/model.h"
#include "QF/Vulkan/qf_vid.h"

typedef struct bspvert_s {
	quat_t      vertex;
	quat_t      tlst;
} bspvert_t;

typedef struct elements_s {
	struct elements_s *_next;
	struct elements_s *next;
	byte       *base;
} elements_t;

typedef struct elechain_s {
	struct elechain_s *_next;
	struct elechain_s *next;
	int         index;
	elements_t *elements;
	vec_t      *transform;
	float      *color;
} elechain_t;

typedef struct bspframe_s {
	uint32_t    *indeces;
	VkCommandBuffer bsp_cmd;
	VkCommandBuffer turb_cmd;
	VkCommandBuffer sky_cmd;
	VkDescriptorSet descriptors;
} bspframe_t;

typedef struct bspframeset_s
    DARRAY_TYPE (bspframe_t) bspframeset_t;

typedef struct texchainset_s
    DARRAY_TYPE (struct vulktex_s *) texchainset_t;

typedef struct bspctx_s {
	instsurf_t  *waterchain;
	instsurf_t **waterchain_tail;
	instsurf_t  *sky_chain;
	instsurf_t **sky_chain_tail;

	texchainset_t texture_chains;

	// for world and non-instance models
	instsurf_t  *static_instsurfs;
	instsurf_t **static_instsurfs_tail;
	instsurf_t  *free_static_instsurfs;

	// for instance models
	elechain_t  *elechains;
	elechain_t **elechains_tail;
	elechain_t  *free_elechains;
	elements_t  *elementss;
	elements_t **elementss_tail;
	elements_t  *free_elementss;
	instsurf_t  *instsurfs;
	instsurf_t **instsurfs_tail;
	instsurf_t  *free_instsurfs;

	struct qfv_tex_s *skybox_tex;
	quat_t       sky_rotation[2];
	quat_t       sky_velocity;
	quat_t       sky_fix;
	double       sky_time;

	quat_t       default_color;
	quat_t       last_color;

	struct scrap_s *light_scrap;
	struct qfv_stagebuf_s *light_stage;

	struct bsppoly_s *polys;

	VkDeviceMemory texture_memory;
	VkPipeline   main;
	VkPipelineLayout layout;
	size_t       vertex_buffer_size;
	size_t       index_buffer_size;
	VkBuffer     vertex_buffer;
	VkDeviceMemory vertex_memory;
	VkBuffer     index_buffer;
	VkDeviceMemory index_memory;
	bspframeset_t frames;
} bspctx_t;

struct vulkan_ctx_s;
void Vulkan_ClearElements (struct vulkan_ctx_s *ctx);
void Vulkan_DrawWorld (struct vulkan_ctx_s *ctx);
void Vulkan_DrawSky (struct vulkan_ctx_s *ctx);
void Vulkan_RegisterTextures (model_t **models, int num_models,
							  struct vulkan_ctx_s *ctx);
void Vulkan_BuildDisplayLists (model_t **models, int num_models,
							   struct vulkan_ctx_s *ctx);
void Vulkan_Bsp_Init (struct vulkan_ctx_s *ctx);
void Vulkan_Bsp_Shutdown (struct vulkan_ctx_s *ctx);
void Vulkan_DrawWaterSurfaces (struct vulkan_ctx_s *ctx);

#endif//__QF_Vulkan_qf_bsp_h
