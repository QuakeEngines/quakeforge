#ifndef __QF_Vulkan_command_h
#define __QF_Vulkan_command_h

#include "QF/darray.h"

typedef struct qfv_cmdbufferset_s
	DARRAY_TYPE (VkCommandBuffer) qfv_cmdbufferset_t;

typedef struct qfv_semaphoreset_s
	DARRAY_TYPE (VkSemaphore) qfv_semaphoreset_t;

typedef struct qfv_fenceset_s
	DARRAY_TYPE (VkFence) qfv_fenceset_t;

struct qfv_queue_s;
struct qfv_device_s;
VkCommandPool QFV_CreateCommandPool (struct qfv_device_s *device,
									  uint32_t queueFamily,
									  int transient, int reset);
qfv_cmdbufferset_t *QFV_AllocateCommandBuffers (struct qfv_device_s *device,
												VkCommandPool pool,
											    int secondary, int count);

VkSemaphore QFV_CreateSemaphore (struct qfv_device_s *device);
VkFence QFV_CreateFence (struct qfv_device_s *device, int signaled);
int QFV_QueueSubmit (struct qfv_queue_s *queue,
					 qfv_semaphoreset_t *waitSemaphores,
					 VkPipelineStageFlags *stages,
					 qfv_cmdbufferset_t *buffers,
					 qfv_semaphoreset_t *signalSemaphores, VkFence fence);
int QFV_QueueWaitIdle (struct qfv_queue_s *queue);

#endif//__QF_Vulkan_command_h