/*
	staging.c

	Vulkan staging buffer manager

	Copyright (C) 2021      Bill Currie <bill@taniwha.org>

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

#ifdef HAVE_MATH_H
# include <math.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include "QF/alloc.h"
#include "QF/cvar.h"
#include "QF/dstring.h"
#include "QF/hash.h"
#include "QF/qfplist.h"
#include "QF/quakefs.h"
#include "QF/sys.h"
#include "QF/Vulkan/qf_vid.h"
#include "QF/Vulkan/buffer.h"
#include "QF/Vulkan/command.h"
#include "QF/Vulkan/device.h"
#include "QF/Vulkan/instance.h"
#include "QF/Vulkan/staging.h"

#include "vid_vulkan.h"

qfv_stagebuf_t *
QFV_CreateStagingBuffer (qfv_device_t *device, size_t size,
						 VkCommandPool cmdPool)
{
	size_t atom = device->physDev->properties.limits.nonCoherentAtomSize;
	qfv_devfuncs_t *dfunc = device->funcs;

	qfv_stagebuf_t *stage = calloc (1, sizeof (qfv_stagebuf_t));
	stage->atom_mask = atom - 1;
	size = (size + stage->atom_mask) & ~stage->atom_mask;
	stage->device = device;
	stage->buffer = QFV_CreateBuffer (device, size,
									  VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	stage->memory = QFV_AllocBufferMemory (device, stage->buffer,
										   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
										   size, 0);
	stage->size = size;
	stage->end = size;

	dfunc->vkMapMemory (device->dev, stage->memory, 0, size, 0, &stage->data);
	QFV_BindBufferMemory (device, stage->buffer, stage->memory, 0);

	int         count = RB_buffer_size (&stage->packets);

	__auto_type bufferset = QFV_AllocCommandBufferSet (count, alloca);
	QFV_AllocateCommandBuffers (device, cmdPool, 0, bufferset);
	for (int i = 0; i < count; i++) {
		qfv_packet_t *packet = &stage->packets.buffer[i];
		packet->stage = stage;
		packet->cmd = bufferset->a[i];
		packet->fence = QFV_CreateFence (device, 1);
		packet->offset = 0;
		packet->length = 0;
	}
	return stage;
}

void
QFV_DestroyStagingBuffer (qfv_stagebuf_t *stage)
{
	qfv_device_t *device = stage->device;
	qfv_devfuncs_t *dfunc = device->funcs;

	int         count = RB_buffer_size (&stage->packets);
	__auto_type fences = QFV_AllocFenceSet (count, alloca);
	for (int i = 0; i < count; i++) {
		fences->a[i] = stage->packets.buffer[i].fence;
	}
	dfunc->vkWaitForFences (device->dev, fences->size, fences->a, VK_TRUE,
							500000000ull);
	for (int i = 0; i < count; i++) {
		dfunc->vkDestroyFence (device->dev, fences->a[i], 0);
	}

	dfunc->vkUnmapMemory (device->dev, stage->memory);
	dfunc->vkFreeMemory (device->dev, stage->memory, 0);
	dfunc->vkDestroyBuffer (device->dev, stage->buffer, 0);
	free (stage);
}

void
QFV_FlushStagingBuffer (qfv_stagebuf_t *stage, size_t offset, size_t size)
{
	qfv_device_t *device = stage->device;
	qfv_devfuncs_t *dfunc = device->funcs;

	offset &= ~stage->atom_mask;
	size = (size + stage->atom_mask) & ~stage->atom_mask;
	VkMappedMemoryRange range = {
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0,
		stage->memory, offset, size
	};
	dfunc->vkFlushMappedMemoryRanges (device->dev, 1, &range);
}

static void
release_space (qfv_stagebuf_t *stage, size_t offset, size_t length)
{
	if (stage->space_end != offset
		&& offset != 0
		&& stage->space_end != stage->end) {
		Sys_Error ("staging: out of sequence packet release");
	}
	if (stage->space_end == stage->end) {
		stage->space_end = 0;
		stage->end = stage->size;
	}
	stage->space_end += length;
}

static void *
acquire_space (qfv_packet_t *packet, size_t size)
{
	qfv_stagebuf_t *stage = packet->stage;
	qfv_device_t *device = stage->device;
	qfv_devfuncs_t *dfunc = device->funcs;

	// clean up after any completed packets
	while (RB_DATA_AVAILABLE (stage->packets) > 1) {
		qfv_packet_t *p = RB_PEEK_DATA (stage->packets, 0);
		if (dfunc->vkGetFenceStatus (device->dev, p->fence) != VK_SUCCESS) {
			break;
		}
		release_space (stage, p->offset, p->length);
		RB_RELEASE (stage->packets, 1);
	}

	if (size > stage->size) {
		// utterly impossible allocation
		return 0;
	}

	// if the staging buffer has been freed up and no data is assigned to the
	// single existing packet, then ensure the packet starts at the beginning
	// of the staging buffer in order to maximize the space available to it
	// (some of the tests are redundant since if any space is assigned to a
	// packet, the buffer cannot be fully freed up)
	if (stage->space_end == stage->space_start
		&& RB_DATA_AVAILABLE (stage->packets) == 1
		&& packet->length == 0) {
		stage->space_end = 0;
		stage->space_start = 0;
		packet->offset = 0;
	}
	if (stage->space_start >= stage->space_end) {
		// all the space to the actual end of the buffer is free
		if (stage->space_start + size <= stage->size) {
			void       *data = (byte *) stage->data + stage->space_start;
			stage->space_start += size;
			return data;
		}
		// doesn't fit at the end of the buffer, try the beginning but only
		// if the packet can be moved (no spaced has been allocated to it yet)
		if (packet->length > 0) {
			// can't move it
			return 0;
		}
		// mark the unused end of the buffer such that it gets reclaimed
		// properly when the preceeding packet is freed
		stage->end = stage->space_start;
		stage->space_start = 0;
		packet->offset = 0;
	}
	while (stage->space_start + size > stage->space_end
		   && RB_DATA_AVAILABLE (stage->packets) > 1) {
		packet = RB_PEEK_DATA (stage->packets, 0);
		dfunc->vkWaitForFences (device->dev, 1, &packet->fence, VK_TRUE,
								~0ull);
		release_space (stage, packet->offset, packet->length);
		RB_RELEASE (stage->packets, 1);
	}
	if (stage->space_start + size > stage->space_end) {
		return 0;
	}
	void       *data = (byte *) stage->data + stage->space_start;
	stage->space_start += size;
	return data;
}

qfv_packet_t *
QFV_PacketAcquire (qfv_stagebuf_t *stage)
{
	qfv_device_t *device = stage->device;
	qfv_devfuncs_t *dfunc = device->funcs;

	qfv_packet_t *packet = 0;
	if (!RB_SPACE_AVAILABLE (stage->packets)) {
		// need to wait for a packet to become available
		packet = RB_PEEK_DATA (stage->packets, 0);
		dfunc->vkWaitForFences (device->dev, 1, &packet->fence, VK_TRUE,
								~0ull);
		release_space (stage, packet->offset, packet->length);
		RB_RELEASE (stage->packets, 1);
	}
	packet = RB_ACQUIRE (stage->packets, 1);

	packet->offset = stage->space_start;
	packet->length = 0;

	dfunc->vkResetFences (device->dev, 1, &packet->fence);
	dfunc->vkResetCommandBuffer (packet->cmd, 0);
	VkCommandBufferBeginInfo beginInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 0,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0,
	};
	dfunc->vkBeginCommandBuffer (packet->cmd, &beginInfo);

	return packet;
}

void *
QFV_PacketExtend (qfv_packet_t *packet, size_t size)
{
	void       *data = acquire_space (packet, size);
	if (data) {
		packet->length += size;
	}
	return data;
}

void
QFV_PacketSubmit (qfv_packet_t *packet)
{
	qfv_stagebuf_t *stage = packet->stage;
	qfv_device_t *device = stage->device;
	qfv_devfuncs_t *dfunc = device->funcs;

	if (!packet->length) {
		// XXX at this stage, this looks the same as below, I think a queue
		// completing is the only way to set a fence (other than creation),
		// so submit the (hopefully) empty command buffer so the fence becomes
		// set, but without waiting on or triggering any semaphores.
		dfunc->vkEndCommandBuffer (packet->cmd);
		VkSubmitInfo submitInfo = {
			VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
			0, 0, 0,
			1, &packet->cmd,
			0, 0,
		};
		dfunc->vkQueueSubmit (device->queue.queue, 1, &submitInfo,
							  packet->fence);
		return;
	}

	QFV_FlushStagingBuffer (stage, packet->offset, packet->length);

	dfunc->vkEndCommandBuffer (packet->cmd);
	//XXX it may become necessary to pass in semaphores etc (maybe add to
	//packet?)
	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
		0, 0, 0,
		1, &packet->cmd,
		0, 0,
	};
	// The fence was reset when the packet was acquired
	dfunc->vkQueueSubmit (device->queue.queue, 1, &submitInfo, packet->fence);
}