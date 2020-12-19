/*
	vid_render_vulkan.c

	Vulkan version of the renderer

	Copyright (C) 2019 Bill Currie <bill@taniwha.org>

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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>

//#define NH_DEFINE
//#include "vulkan/namehack.h"

#include "QF/darray.h"
#include "QF/sys.h"

#include "QF/plugin/general.h"
#include "QF/plugin/vid_render.h"

#include "QF/Vulkan/qf_vid.h"
#include "QF/Vulkan/command.h"
#include "QF/Vulkan/device.h"
#include "QF/Vulkan/image.h"
#include "QF/Vulkan/instance.h"
#include "QF/Vulkan/swapchain.h"

#include "mod_internal.h"
#include "r_internal.h"
#include "vid_internal.h"
#include "vid_vulkan.h"

#include "vulkan/namehack.h"

static vulkan_ctx_t *vulkan_ctx;

static void
vulkan_R_Init (void)
{
	qfv_device_t *device = vulkan_ctx->device;
	qfv_devfuncs_t *dfunc = device->funcs;

	Vulkan_CreateSwapchain (vulkan_ctx);
	Vulkan_CreateRenderPass (vulkan_ctx);
	Vulkan_CreateFramebuffers (vulkan_ctx);

	qfv_swapchain_t *sc = vulkan_ctx->swapchain;

	VkCommandBufferBeginInfo beginInfo
		= { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkClearValue clearValues[3] = {
		{ { {0.7294, 0.8549, 0.3333, 1.0} } },
		{ { {1.0, 0.0} } },
		{ { {0, 0, 0, 0} } },
	};
	VkRenderPassBeginInfo renderPassInfo = {
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, 0,
		vulkan_ctx->renderpass.renderpass, 0,
		{ {0, 0}, sc->extent },
		2, clearValues
	};
	for (size_t i = 0; i < vulkan_ctx->framebuffers.size; i++) {
		__auto_type framebuffer = &vulkan_ctx->framebuffers.a[i];
		dfunc->vkBeginCommandBuffer (framebuffer->cmdBuffer, &beginInfo);
		renderPassInfo.framebuffer = framebuffer->framebuffer;
		dfunc->vkCmdBeginRenderPass (framebuffer->cmdBuffer, &renderPassInfo,
				VK_SUBPASS_CONTENTS_INLINE);

		dfunc->vkCmdEndRenderPass (framebuffer->cmdBuffer);
		dfunc->vkEndCommandBuffer (framebuffer->cmdBuffer);
	}
	Sys_Printf ("R_Init %p %d", vulkan_ctx->swapchain->swapchain,
				vulkan_ctx->swapchain->numImages);
	for (int32_t i = 0; i < vulkan_ctx->swapchain->numImages; i++) {
		Sys_Printf (" %p", vulkan_ctx->swapchain->images->a[i]);
	}
	Sys_Printf ("\n");
}

static void
vulkan_SCR_UpdateScreen (double time,  void (*f)(void), void (**g)(void))
{
	static int count = 0;
	static double startTime;
	uint32_t imageIndex = 0;
	qfv_device_t *device = vulkan_ctx->device;
	qfv_devfuncs_t *dfunc = device->funcs;
	VkDevice    dev = device->dev;
	qfv_queue_t *queue = &vulkan_ctx->device->queue;

	__auto_type framebuffer
		= &vulkan_ctx->framebuffers.a[vulkan_ctx->curFrame];

	dfunc->vkWaitForFences (dev, 1, &framebuffer->fence, VK_TRUE, 2000000000);
	QFV_AcquireNextImage (vulkan_ctx->swapchain,
						  framebuffer->imageAvailableSemaphore,
						  0, &imageIndex);

	VkPipelineStageFlags waitStage
		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
		1,
		&framebuffer->imageAvailableSemaphore,
		&waitStage,
		1, &framebuffer->cmdBuffer,
		1, &framebuffer->renderDoneSemaphore,
	};
	dfunc->vkResetFences (dev, 1, &framebuffer->fence);
	dfunc->vkQueueSubmit (queue->queue, 1, &submitInfo, framebuffer->fence);

	VkPresentInfoKHR presentInfo = {
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, 0,
		1, &framebuffer->renderDoneSemaphore,
		1, &vulkan_ctx->swapchain->swapchain, &imageIndex,
		0
	};
	dfunc->vkQueuePresentKHR (queue->queue, &presentInfo);

	vulkan_ctx->curFrame++;
	vulkan_ctx->curFrame %= vulkan_ctx->framebuffers.size;

	if (++count >= 100) {
		double currenTime = Sys_DoubleTime ();
		double time = currenTime - startTime;
		startTime = currenTime;
		printf ("%d frames in %g s: %g fps         \r",
				count, time, count / time);
		fflush (stdout);
		count = 0;
	}
}

static qpic_t *
vulkan_Draw_CachePic (const char *path, qboolean alpha)
{
	return 0;
}

static qpic_t qpic = { 1, 1, {0} };

static qpic_t *
vulkan_Draw_MakePic (int width, int height, const byte *data)
{
	return &qpic;
}

static vid_model_funcs_t model_funcs = {
	0,//vulkan_Mod_LoadExternalTextures,
	0,//vulkan_Mod_LoadLighting,
	0,//vulkan_Mod_SubdivideSurface,
	0,//vulkan_Mod_ProcessTexture,

	Mod_LoadIQM,
	Mod_LoadAliasModel,
	Mod_LoadSpriteModel,

	0,//vulkan_Mod_MakeAliasModelDisplayLists,
	0,//vulkan_Mod_LoadSkin,
	0,//vulkan_Mod_FinalizeAliasModel,
	0,//vulkan_Mod_LoadExternalSkins,
	0,//vulkan_Mod_IQMFinish,
	0,
	0,//vulkan_Mod_SpriteLoadTexture,

	Skin_SetColormap,
	Skin_SetSkin,
	0,//vulkan_Skin_SetupSkin,
	Skin_SetTranslation,
	0,//vulkan_Skin_ProcessTranslation,
	0,//vulkan_Skin_InitTranslations,
};

vid_render_funcs_t vulkan_vid_render_funcs = {
	0,//vulkan_Draw_Init,
	0,//vulkan_Draw_Character,
	0,//vulkan_Draw_String,
	0,//vulkan_Draw_nString,
	0,//vulkan_Draw_AltString,
	0,//vulkan_Draw_ConsoleBackground,
	0,//vulkan_Draw_Crosshair,
	0,//vulkan_Draw_CrosshairAt,
	0,//vulkan_Draw_TileClear,
	0,//vulkan_Draw_Fill,
	0,//vulkan_Draw_TextBox,
	0,//vulkan_Draw_FadeScreen,
	0,//vulkan_Draw_BlendScreen,
	vulkan_Draw_CachePic,
	0,//vulkan_Draw_UncachePic,
	vulkan_Draw_MakePic,
	0,//vulkan_Draw_DestroyPic,
	0,//vulkan_Draw_PicFromWad,
	0,//vulkan_Draw_Pic,
	0,//vulkan_Draw_Picf,
	0,//vulkan_Draw_SubPic,

	vulkan_SCR_UpdateScreen,
	SCR_DrawRam,
	SCR_DrawTurtle,
	SCR_DrawPause,
	0,//vulkan_SCR_CaptureBGR,
	0,//vulkan_SCR_ScreenShot,
	SCR_DrawStringToSnap,

	0,//vulkan_Fog_Update,
	0,//vulkan_Fog_ParseWorldspawn,

	vulkan_R_Init,
	0,//vulkan_R_ClearState,
	0,//vulkan_R_LoadSkys,
	0,//vulkan_R_NewMap,
	R_AddEfrags,
	R_RemoveEfrags,
	R_EnqueueEntity,
	0,//vulkan_R_LineGraph,
	R_AllocDlight,
	R_AllocEntity,
	0,//vulkan_R_RenderView,
	R_DecayLights,
	0,//vulkan_R_ViewChanged,
	0,//vulkan_R_ClearParticles,
	0,//vulkan_R_InitParticles,
	0,//vulkan_SCR_ScreenShot_f,
	0,//vulkan_r_easter_eggs_f,
	0,//vulkan_r_particles_style_f,
	0,
	&model_funcs
};

static void
set_palette (const byte *palette)
{
	//FIXME really don't want this here: need an application domain
	//so Quake can be separated from QuakeForge (ie, Quake itself becomes
	//an app using the QuakeForge engine)
}

static void
vulkan_vid_render_choose_visual (void)
{
	Vulkan_CreateDevice (vulkan_ctx);
	vulkan_ctx->choose_visual (vulkan_ctx);
	vulkan_ctx->cmdpool = QFV_CreateCommandPool (vulkan_ctx->device,
									 vulkan_ctx->device->queue.queueFamily,
									 0, 1);
	__auto_type cmdset = QFV_AllocateCommandBuffers (vulkan_ctx->device,
													 vulkan_ctx->cmdpool,
													 0, 1);
	vulkan_ctx->cmdbuffer = cmdset->a[0];
	free (cmdset);
	vulkan_ctx->fence = QFV_CreateFence (vulkan_ctx->device, 1);
	Sys_Printf ("vk choose visual %p %p %d %p\n", vulkan_ctx->device->dev,
				vulkan_ctx->device->queue.queue,
				vulkan_ctx->device->queue.queueFamily,
				vulkan_ctx->cmdpool);
}

static void
vulkan_vid_render_create_context (void)
{
	vulkan_ctx->create_window (vulkan_ctx);
	vulkan_ctx->surface = vulkan_ctx->create_surface (vulkan_ctx);
	Sys_Printf ("vk create context %p\n", vulkan_ctx->surface);
}

static void
vulkan_vid_render_init (void)
{
	vulkan_ctx = vr_data.vid->vid_internal->vulkan_context ();
	vulkan_ctx->load_vulkan (vulkan_ctx);

	Vulkan_Init_Common (vulkan_ctx);

	vr_data.vid->vid_internal->set_palette = set_palette;
	vr_data.vid->vid_internal->choose_visual = vulkan_vid_render_choose_visual;
	vr_data.vid->vid_internal->create_context = vulkan_vid_render_create_context;

	vr_funcs = &vulkan_vid_render_funcs;
	m_funcs = &model_funcs;
}

static void
vulkan_vid_render_shutdown (void)
{
	qfv_device_t *device = vulkan_ctx->device;
	qfv_devfuncs_t *df = device->funcs;
	VkDevice    dev = device->dev;
	QFV_DeviceWaitIdle (device);
	df->vkDestroyFence (dev, vulkan_ctx->fence, 0);
	df->vkDestroyCommandPool (dev, vulkan_ctx->cmdpool, 0);
	Vulkan_Shutdown_Common (vulkan_ctx);
}

static general_funcs_t plugin_info_general_funcs = {
	vulkan_vid_render_init,
	vulkan_vid_render_shutdown,
};

static general_data_t plugin_info_general_data;

static plugin_funcs_t plugin_info_funcs = {
	&plugin_info_general_funcs,
	0,
	0,
	0,
	0,
	0,
	&vulkan_vid_render_funcs,
};

static plugin_data_t plugin_info_data = {
	&plugin_info_general_data,
	0,
	0,
	0,
	0,
	0,
	&vid_render_data,
};

static plugin_t plugin_info = {
	qfp_vid_render,
	0,
	QFPLUGIN_VERSION,
	"0.1",
	"Vulkan Renderer",
	"Copyright (C) 1996-1997  Id Software, Inc.\n"
	"Copyright (C) 1999-2019  contributors of the QuakeForge project\n"
	"Please see the file \"AUTHORS\" for a list of contributors",
	&plugin_info_funcs,
	&plugin_info_data,
};

PLUGIN_INFO(vid_render, vulkan)
{
	return &plugin_info;
}