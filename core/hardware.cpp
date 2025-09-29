#include "hardware.h"


#ifdef VKBUILD

/**
 *	select detected gpu
 */
void GPUDevice::select()
{
	COMM_ERR_COND(!supported,"selected gpu %s is not supported",properties.deviceName);
	COMM_LOG_FALLBACK("selecting gpu %s",properties.deviceName);
	g_GPU.device_info = this;

	// queue creation
	f32 __QueuePriority = 1.f;
	vector<VkDeviceQueueCreateInfo> __QueueInfos;
	__QueueInfos.reserve(queues.size());
	for (u32 __QueueID : queues)
	{
		__QueueInfos.push_back({  });
		__QueueInfos.back().sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		__QueueInfos.back().queueFamilyIndex = __QueueID;
		__QueueInfos.back().queueCount = 1;
		__QueueInfos.back().pQueuePriorities = &__QueuePriority;
	}

	// device features
	VkPhysicalDeviceFeatures __DeviceFeatures = {  };  // TODO

	// device creation specifics
	VkDeviceCreateInfo __DeviceInfo = {  };
	__DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	__DeviceInfo.queueCreateInfoCount = (u32)__QueueInfos.size();
	__DeviceInfo.pQueueCreateInfos = &__QueueInfos[0];
	__DeviceInfo.enabledExtensionCount = (u32)g_GPUExtensions.size();
	__DeviceInfo.ppEnabledExtensionNames = &g_GPUExtensions[0];
	__DeviceInfo.pEnabledFeatures = &__DeviceFeatures;

	// enable validation layers here as well for safety, even though it's deprecated
#ifdef DEBUG
	__DeviceInfo.enabledLayerCount = (u32)g_ValidationLayers.size();
	__DeviceInfo.ppEnabledLayerNames = &g_ValidationLayers[0];
#else
	__DeviceInfo.enabledLayerCount = 0;
#endif

	// create device
	VkResult __Result = vkCreateDevice(gpu,&__DeviceInfo,nullptr,&g_GPU.gpu);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not create logical interface for gpu %s",properties.deviceName);

	// initialize queues
	vkGetDeviceQueue(g_GPU.gpu,graphical_queue,0,&g_GPU.graphical_queue);
	vkGetDeviceQueue(g_GPU.gpu,presentation_queue,0,&g_GPU.presentation_queue);
}

/**
 *	hardware detection routine
 */
Hardware::Hardware()
{
	COMM_LOG("detecting available GPUs");
	u32 __GPUCount;
	vkEnumeratePhysicalDevices(g_Vk.instance,&__GPUCount,nullptr);
	COMM_ERR_COND(!__GPUCount,"no vulkan capable gpus found. use opengl version!")
	COMM_SCC_FALLBACK("found %u vulkan capable graphics card%s",__GPUCount,(__GPUCount>1)?"s":"");
	vector<VkPhysicalDevice> __PhysicalGPUs = vector<VkPhysicalDevice>(__GPUCount);
	gpus.resize(__GPUCount);
	vkEnumeratePhysicalDevices(g_Vk.instance,&__GPUCount,&__PhysicalGPUs[0]);
	// TODO the fallback macro should be using the pluralization macro once it's done

	// scanning available gpus for specifics
	gpus.resize(__GPUCount);
	for (u8 i=0;i<__GPUCount;i++)
	{
		gpus[i].gpu = __PhysicalGPUs[i];

		// get available queue families
		u32 __QueueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(gpus[i].gpu,&__QueueCount,nullptr);
		vector<VkQueueFamilyProperties> __Queues(__QueueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(gpus[i].gpu,&__QueueCount,&__Queues[0]);

		// iterate queue families & extract ids
		for (u32 j=0;j<__QueueCount;j++)
		{
			// check for graphical support
			if (__Queues[j].queueFlags&VK_QUEUE_GRAPHICS_BIT) gpus[i].graphical_queue = j;

			// check for presentation support
			VkBool32 __PresentingQueue = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i].gpu,j,g_Vk.surface,&__PresentingQueue);
			if (__PresentingQueue) gpus[i].presentation_queue = j;

			// check for sufficient queue support & abort to align graphical queue with presenting queue
			if (gpus[i].graphical_queue!=-1&&gpus[i].presentation_queue!=-1)
			{
				gpus[i].queues = { (u32)gpus[i].graphical_queue,(u32)gpus[i].presentation_queue };
				gpus[i].supported = GPU_FEATURE_SUPPORT_BASIC;
				break;
			}
			// TODO iterate fully & check for aligning ids, to avoid queue split in edge-cases
		}

		// interrupt gpu read should queue support not be sufficient
		if (!gpus[i].supported)
		{
			COMM_ERR("interrupting GPU read at index %i due to insufficient queue support",i);
			continue;
		}

		// get available extensions
		u32 __ExtensionCount,__FormatCount,__ModeCount;
		vkEnumerateDeviceExtensionProperties(gpus[i].gpu,nullptr,&__ExtensionCount,nullptr);
		gpus[i].extensions.resize(__ExtensionCount);
		vkEnumerateDeviceExtensionProperties(gpus[i].gpu,nullptr,&__ExtensionCount,&gpus[i].extensions[0]);

		// checking extension support
		set<string> __RequiredExtensions = set<string>(g_GPUExtensions.begin(),g_GPUExtensions.end());
		for (VkExtensionProperties& __Extension : gpus[i].extensions)
			__RequiredExtensions.erase(__Extension.extensionName);
		gpus[i].supported = __RequiredExtensions.empty()*GPU_FEATURE_SUPPORT_BASIC;
		if (!gpus[i].supported)
		{
			COMM_ERR("interrupting GPU read at index %i, the device is missing crucial extensions",i);
			continue;
		}

		// get device specifics
		vkGetPhysicalDeviceProperties(gpus[i].gpu,&gpus[i].properties);
		vkGetPhysicalDeviceFeatures(gpus[i].gpu,&gpus[i].features);
		COMM_SCC("found supported GPU %s",gpus[i].properties.deviceName);
		// TODO later, read the capabilities of the selected device, allow to change it and change features

		// get swap chain format capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpus[i].gpu,g_Vk.surface,&gpus[i].swapchain_info.capabilities);
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[i].gpu,g_Vk.surface,&__FormatCount,nullptr);
		if (!!__FormatCount)
		{
			gpus[i].swapchain_info.formats.resize(__FormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[i].gpu,g_Vk.surface,
												 &__FormatCount,&gpus[i].swapchain_info.formats[0]);
		}
		COMM_ERR_FALLBACK("no surface formats found for GPU %s",gpus[i].properties.deviceName);

		// get swap chain mode capabilities
		vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[i].gpu,g_Vk.surface,&__ModeCount,nullptr);
		if (!!__ModeCount)
		{
			gpus[i].swapchain_info.modes.resize(__ModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[i].gpu,g_Vk.surface,
													  &__ModeCount,&gpus[i].swapchain_info.modes[0]);
		}
		COMM_ERR_FALLBACK("no presentation modes found for GPU %s",gpus[i].properties.deviceName);

		// get memory types
		vkGetPhysicalDeviceMemoryProperties(gpus[i].gpu,&gpus[i].memory_properties);
	}
}

#endif
