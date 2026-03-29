#include "Context.h"
#include <algorithm>

namespace caaVk {

	using namespace std;

	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };
	PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };
	PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{ nullptr };

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
	VkDebugUtilsMessengerEXT debugUtilsMessenger;

	VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessageCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		std::stringstream debugMessage;
		if (pCallbackData->pMessageIdName) {
			debugMessage << "[" << pCallbackData->messageIdNumber << "]["
				<< pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;
		}
		else {
			debugMessage << "[" << pCallbackData->messageIdNumber << "] : " << pCallbackData->pMessage;
		}

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
			printLog("[VERBOSE] {}", debugMessage.str());
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			printLog("[INFO] {}", debugMessage.str());
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			printLog("[WARNING] {}", debugMessage.str());
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			exitWithMessage("[ERROR] {}", debugMessage.str());
		}

		return VK_FALSE;
	}

	caaVk::Context::Context(const vector<const char*>& requiredInstanceExtensions, bool useSwapchain)
		: descriptorPool_(_device)
	{
	
		createInstance(requiredInstanceExtensions);
		selectPhysicalDevice();
		createLogicalDevice(useSwapchain);
		createQueues();
		createPipelineCache();
		determineDepthStencilFormat();
		descriptorPool_.createFromScript();
	}

	caaVk::Context::~Context()
	{
	}

	void caaVk::Context::cleanup()
	{
	}

	void caaVk::Context::createQueues()
	{
		// Validate queue family indices before getting queues
		if (queueFamilyIndices_.graphics == uint32_t(-1)) {
			exitWithMessage("Graphics queue family index is invalid");
		}

		vkGetDeviceQueue(_device, queueFamilyIndices_.graphics, 0, &graphicsQueue_);

		// For compute: either get from dedicated family or use graphics queue
		if (queueFamilyIndices_.compute != queueFamilyIndices_.graphics &&
			queueFamilyIndices_.compute != uint32_t(-1)) {
			vkGetDeviceQueue(_device, queueFamilyIndices_.compute, 0, &computeQueue_);
		}
		else {
			computeQueue_ = graphicsQueue_;
		}

		// For transfer: either get from dedicated family or use appropriate fallback
		if (queueFamilyIndices_.transfer != queueFamilyIndices_.graphics &&
			queueFamilyIndices_.transfer != queueFamilyIndices_.compute &&
			queueFamilyIndices_.transfer != uint32_t(-1)) {
			vkGetDeviceQueue(_device, queueFamilyIndices_.transfer, 0, &transferQueue_);
		}
		else if (queueFamilyIndices_.transfer == queueFamilyIndices_.compute) {
			transferQueue_ = computeQueue_;
		}
		else {
			transferQueue_ = graphicsQueue_;
		}

		// Validate all queues were obtained
		if (graphicsQueue_ == VK_NULL_HANDLE) {
			exitWithMessage("Failed to get graphics queue");
		}
		if (computeQueue_ == VK_NULL_HANDLE) {
			exitWithMessage("Failed to get compute queue");
		}
		if (transferQueue_ == VK_NULL_HANDLE) {
			exitWithMessage("Failed to get transfer queue");
		}

	}


	/*
	Instance ?앹꽦 ?쒖꽌
	1. Extension ?먯깋 (vkEnumerateInstanceExtensionProperties)
	2. Extension?먯꽌 ?붽뎄???ㅽ럺?덈뒗吏 ?뺤씤
	3. DebugUtil 以鍮?(VkDebugUtilsMessengerCreateInfoEXT)
	4. InstanceLayerProperties 媛?몄삤湲?(?꾩옱 ?쒖뒪?쒖뿉 ?ㅼ튂?섏뼱 ?ъ슜 媛?ν븳 '?몄뒪?댁뒪 ?덉씠??Instance Layers)'??紐⑸줉???뺤씤)
		- Layer媛 ?덉쑝硫?App???몄텧?섎㈃ Layer??嫄곗퀜???쒕씪?대쾭濡?媛?
	5. Instance ?앹꽦
	6. Debug ?앹꽦
	*/

	void caaVk::Context::createInstance(vector<const char*> instanceExtensions)
	{
	#ifdef NDEBUG
		bool useValidation = false; // Release build
	#else
		bool useValidation = true; // Debug build
	#endif

		const uint32_t apiVersion = VK_API_VERSION_1_3;
		const string name = "Vulkan Examples";

		// Extension 寃??
		vector<string> supportedInstanceExtensions;
		uint32_t extCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
		if (extCount > 0) {
			vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) ==
				VK_SUCCESS) {
				for (VkExtensionProperties& extension : extensions) {
					supportedInstanceExtensions.push_back(extension.extensionName);
				}
			}
		}

		printLog("Supported Instance Extensions:");
		for (const string& extension : supportedInstanceExtensions) {
			printLog("  {}", extension);
		}

		// MoltenVK on macOS/iOS supported
		const char* portabilityExtension = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
		bool portabilityAlreadyAdded = false;
		for (const char* ext : instanceExtensions) {
			if (strcmp(ext, portabilityExtension) == 0) {
				portabilityAlreadyAdded = true;
				break;
			}
		}

		// check PORTABILITY
		bool portabilitySupported =
			find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
				portabilityExtension) != supportedInstanceExtensions.end();

		if (!portabilityAlreadyAdded && portabilitySupported) {
			instanceExtensions.push_back(portabilityExtension);
			portabilityAlreadyAdded = true;
		}

		// Validate all required extensions are supported
		for (const char* requiredExtension : instanceExtensions) {
			if (find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
				requiredExtension) == supportedInstanceExtensions.end()) {
				exitWithMessage("Required instance extension \"{}\" is not supported",
					requiredExtension);
			}
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = name.c_str();
		appInfo.pEngineName = name.c_str();
		appInfo.apiVersion = apiVersion;

		VkInstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;

		if (portabilityAlreadyAdded) {
			instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
			// Also need to enable the extension in ppEnabledExtensionNames
		}


		// DebugUtil ?깅줉
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
		if (useValidation) {
			debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugUtilsMessengerCI
				.messageSeverity = // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				//  VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
				//  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			// VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugUtilsMessengerCI.pfnUserCallback = debugUtilsMessageCallback;
			debugUtilsMessengerCI.pNext = instanceCreateInfo.pNext;
			instanceCreateInfo.pNext = &debugUtilsMessengerCI;

			const char* debugExtension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
			if (find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
				debugExtension) != supportedInstanceExtensions.end()) {
				instanceExtensions.push_back(debugExtension);
			}
			else {
				printLog("Debug utils extension not supported, debug features will be limited");
			}
		}

		// print instanceExtensions
		printLog("Required Instance Extensions:");
		for (const char* extension : instanceExtensions) {
			printLog("  {}", extension);
		}

		if (!instanceExtensions.empty()) {
			instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
			instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
		}

		uint32_t instanceLayerCount;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());

		// Sort layerProperties by layerName
		sort(instanceLayerProperties.begin(), instanceLayerProperties.end(),
			[](const VkLayerProperties& a, const VkLayerProperties& b) {
				return strcmp(a.layerName, b.layerName) < 0;
			});

		printLog("Available instance layers:");
		for (const VkLayerProperties& props : instanceLayerProperties) {
			printLog("  {}", props.layerName);
		}

		if (useValidation) {
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

			// Validation layer check
			bool validationLayerPresent = false;
			for (VkLayerProperties& layer : instanceLayerProperties) {
				if (strcmp(layer.layerName, validationLayerName) == 0) {
					validationLayerPresent = true;
					break;
				}
			}

			if (validationLayerPresent) {
				instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
				instanceCreateInfo.enabledLayerCount = 1;
			}
			else {
				exitWithMessage("Validation layer VK_LAYER_KHRONOS_validation not present");
			}
		}

		check(vkCreateInstance(&instanceCreateInfo, nullptr, &_vkInstance));

		if (find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
			vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
				vkGetInstanceProcAddr(_vkInstance, "vkCmdBeginDebugUtilsLabelEXT"));
			vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
				vkGetInstanceProcAddr(_vkInstance, "vkCmdEndDebugUtilsLabelEXT"));
			vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
				vkGetInstanceProcAddr(_vkInstance, "vkCmdInsertDebugUtilsLabelEXT"));
		}

		if (useValidation) {
			vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(_vkInstance, "vkCreateDebugUtilsMessengerEXT"));
			vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(_vkInstance, "vkDestroyDebugUtilsMessengerEXT"));

			VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
			debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			debugUtilsMessengerCI.pfnUserCallback = debugUtilsMessageCallback;
			check(vkCreateDebugUtilsMessengerEXT(_vkInstance, &debugUtilsMessengerCI, nullptr,
				&debugUtilsMessenger));
		}





	}

	/*
	Logical Device : Physical ??븷?섎뒗 Device (Vulkan ?ㅼ젣 ?뚰넻?섎뒗 Device)
	- ?꾩슂??Queue Type : Compute, Graphics ?뱀? Transit
	- QueueFamily媛 Index 紐뉖쾲?몄? ?뺤씤 (queueFamily媛 紐뉖쾲 Index?몄???PhysicalDevice?먯꽌 ?뺤씤?덉뿀??)
	- Device瑜?留뚮뱾???댁젣 extension???뺤씤?댁쨾?쇳븿.
		- VK_KHR_SWAPCHAIN_EXTENSION_NAME
		- Physical Device?먯꽌 媛?몄삩 features ?ｌ뼱以섏빞??. (Shader 吏?먯뿬遺?대윴寃껊뱾..)
	- Device ?앹꽦 ??泥댁씠?앹쓣?듯븳 ?뺤옣
		- pNext???ㅼ쓬 寃껋쓣 ?ｌ뼱以?
	- Device ?앹꽦
	- Command ?앹꽦
	*/

	void caaVk::Context::createLogicalDevice(bool useSwapChain)
	{
		const VkQueueFlags requestedQueueTypes = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;

		VkPhysicalDeviceVulkan13Features enabledFeatures13{
	   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		enabledFeatures13.dynamicRendering = VK_TRUE;
		enabledFeatures13.synchronization2 = VK_TRUE;

		vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

		const float defaultQueuePriority(0.0f);

		if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
			queueFamilyIndices_.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = queueFamilyIndices_.graphics;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
		else {
			queueFamilyIndices_.graphics = 0;
		}

		if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
			queueFamilyIndices_.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
			if (queueFamilyIndices_.compute != queueFamilyIndices_.graphics) {

				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = queueFamilyIndices_.compute;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
		}
		else {

			queueFamilyIndices_.compute = queueFamilyIndices_.graphics;
		}

		if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
			queueFamilyIndices_.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
			if ((queueFamilyIndices_.transfer != queueFamilyIndices_.graphics) &&
				(queueFamilyIndices_.transfer != queueFamilyIndices_.compute)) {

				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = queueFamilyIndices_.transfer;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
		}
		else {
			queueFamilyIndices_.transfer = queueFamilyIndices_.graphics;
		}
	
		vector<const char*> deviceExtensions(enabledDeviceExtensions_);
		if (useSwapChain) {
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		enabledFeatures_.samplerAnisotropy = deviceFeatures_.samplerAnisotropy;
		enabledFeatures_.depthClamp = deviceFeatures_.depthClamp;
		enabledFeatures_.depthBiasClamp = deviceFeatures_.depthBiasClamp;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures_;

		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures2.features = enabledFeatures_;
		physicalDeviceFeatures2.pNext = &enabledFeatures13;
		deviceCreateInfo.pEnabledFeatures = nullptr; // ?섎떎 Feature媛 ?덉쑝硫??덈맖.
		deviceCreateInfo.pNext = &physicalDeviceFeatures2;

		if (deviceExtensions.size() > 0) {
			for (const char* enabledExtension : deviceExtensions) {
				if (!extensionSupported(enabledExtension)) {
					printLog("Enabled device extension \"{}\" is not present at device level",
						enabledExtension);
				}
			}

			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		check(vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device));

		graphicsCommandPool_ = createCommandPool(queueFamilyIndices_.graphics);
		if (queueFamilyIndices_.compute != queueFamilyIndices_.graphics) {
			computeCommandPool_ = createCommandPool(queueFamilyIndices_.compute);
		}
		else {
			computeCommandPool_ = graphicsCommandPool_;
		}
		if (queueFamilyIndices_.transfer != queueFamilyIndices_.graphics &&
			queueFamilyIndices_.transfer != queueFamilyIndices_.compute) {
			transferCommandPool_ = createCommandPool(queueFamilyIndices_.transfer);
		}
		else if (queueFamilyIndices_.transfer == queueFamilyIndices_.compute) {
			transferCommandPool_ = computeCommandPool_;
		}
		else {
			transferCommandPool_ = graphicsCommandPool_;
		}

	}

	[[nodiscard]]
	string getPhysicalDeviceTypeString(VkPhysicalDeviceType type)
	{
		switch (type) {
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			return "Other";
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			return "Integrated GPU";
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			return "Discrete GPU";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			return "Virtual GPU";
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			return "CPU";
		default:
			return "Unknown";
		}
	}

	/*
	?뚯씠?꾨씪???앹꽦 ?띾룄瑜?鍮꾩빟?곸쑝濡??믪씠湲??꾪빐 而댄뙆?쇰맂 ?곗씠?곕? ??ν븯??'罹먯떆(Cache) 媛앹껜
	*/

	void caaVk::Context::createPipelineCache()
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		check(vkCreatePipelineCache(_device, &pipelineCacheCreateInfo, nullptr, &pipelineCache_));
	}

	/*
	The support of a format depends on the tiling mode and usage, so we must also include these as parameters.
	*/
	void caaVk::Context::determineDepthStencilFormat()
	{

		/*
		??VK_FORMAT_D32_SFLOAT: 32-bit float for depth
		??VK_FORMAT_D32_SFLOAT_S8_UINT: 32-bit signed float for depth and 8 bit
		stencil component
		??VK_FORMAT_D24_UNORM_S8_UINT: 24-bit float for depth and 8 bit stencil
		component
		*/
		std::vector<VkFormat> formatList = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT
		};

		for (auto& format : formatList) {
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &formatProps);
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				depthFormat_ = format;
				break;
			}
		}

		assert(depthFormat_ != VK_FORMAT_UNDEFINED);
	}

	/*
	GPU 媛쒖닔 ?뺤씤
	1. PhysicalDevice ?뺤씤 諛??좏깮
	2. PhysicalDevice Feature ?좏깮
		-  Shader ?щ? ?뺤씤 : vkGetPhysicalDeviceFeatures
		- ?대뼡 硫붾え由?援ъ“瑜?媛吏怨??덈뒗吏 ?뺤씤 : vkGetPhysicalDeviceMemoryProperties
		- Family Queue ?뺤씤 : vkGetPhysicalDeviceQueueFamilyProperties
	3. 

	*/

	void caaVk::Context::selectPhysicalDevice()
	{
		uint32_t gpuCount = 0;
		check(vkEnumeratePhysicalDevices(_vkInstance, &gpuCount, nullptr));
		if (gpuCount == 0) {
			exitWithMessage("gpuCount is 0");
		}

		vector<VkPhysicalDevice> physicalDevices(gpuCount);
		check(vkEnumeratePhysicalDevices(_vkInstance, &gpuCount, physicalDevices.data()));

		printLog("\nAvailable physical devices: {}", gpuCount);
		for (size_t i = 0; i < gpuCount; ++i) {
			VkPhysicalDeviceProperties deviceProperties_;
			vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties_);
			printLog("  {} {} ({})", i, deviceProperties_.deviceName,
				getPhysicalDeviceTypeString(deviceProperties_.deviceType));
		}

		uint32_t selectedDevice = 0;
		// Prioritize Discrete GPU if available
		for (uint32_t i = 0; i < gpuCount; ++i) {
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(physicalDevices[i], &props);
			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				selectedDevice = i;
				break;
			}
		}

		_physicalDevice = physicalDevices[selectedDevice];

		vkGetPhysicalDeviceProperties(_physicalDevice, &deviceProperties_);

		printLog("Selected Device {}: {} ({})", selectedDevice, deviceProperties_.deviceName,
			getPhysicalDeviceTypeString(deviceProperties_.deviceType));
		printLog("  nonCoherentAtomSize: {}", deviceProperties_.limits.nonCoherentAtomSize);
		printLog("  Max UBO size: {} KBytes", deviceProperties_.limits.maxUniformBufferRange / 1024);
		printLog("  Max SSBO size: {} KBytes", deviceProperties_.limits.maxStorageBufferRange / 1024);
		printLog("  UBO offset alignment: {}",
			deviceProperties_.limits.minUniformBufferOffsetAlignment);
		printLog("  SSBO offset alignment: {}",
			deviceProperties_.limits.minStorageBufferOffsetAlignment);

		// To get the list of features supported by a given device
		vkGetPhysicalDeviceFeatures(_physicalDevice, &deviceFeatures_);

		printLog("\nDevice Features:");
		printLog("  geometryShader: {}", deviceFeatures_.geometryShader ? "YES" : "NO");
		printLog("  tessellationShader: {}", deviceFeatures_.tessellationShader ? "YES" : "NO");

		vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &deviceMemoryProperties_);

		// Print device memory properties
		printLog("\nDevice Memory Properties:");
		printLog("  Memory Type Count: {}", deviceMemoryProperties_.memoryTypeCount);
		for (uint32_t i = 0; i < deviceMemoryProperties_.memoryTypeCount; ++i) {
			const auto& memType = deviceMemoryProperties_.memoryTypes[i];
			string propFlags;
			if (memType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				propFlags += "DEVICE_LOCAL ";
			if (memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
				propFlags += "HOST_VISIBLE ";
			if (memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
				propFlags += "HOST_COHERENT ";
			if (memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
				propFlags += "HOST_CACHED ";
			if (memType.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
				propFlags += "LAZILY_ALLOCATED ";
			if (memType.propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
				propFlags += "PROTECTED ";
			if (propFlags.empty())
				propFlags = "NONE ";

			printLog("    Memory Type {}: heap {}, flags: {}", i, memType.heapIndex, propFlags);
		}

		printLog("  Memory Heap Count: {}", deviceMemoryProperties_.memoryHeapCount);
		for (uint32_t i = 0; i < deviceMemoryProperties_.memoryHeapCount; ++i) {
			const auto& heap = deviceMemoryProperties_.memoryHeaps[i];
			string heapFlags;
			if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				heapFlags += "DEVICE_LOCAL ";
			if (heap.flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
				heapFlags += "MULTI_INSTANCE ";
			if (heapFlags.empty())
				heapFlags = "NONE ";

			printLog("    Memory Heap {}: {} MB, flags: {}", i, heap.size / (1024 * 1024), heapFlags);
		}

		// Find queue family properties
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);
		assert(queueFamilyCount > 0);
		queueFamilyProperties_.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount,
			queueFamilyProperties_.data());

		printLog("\nQueue Family Properties: {}", queueFamilyCount);
		for (uint32_t i = 0; i < queueFamilyCount; ++ i) {
			const auto& props = queueFamilyProperties_[i];

			string queueFlagsStr;
			if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				queueFlagsStr += "GRAPHICS ";
			if (props.queueFlags & VK_QUEUE_COMPUTE_BIT)
				queueFlagsStr += "COMPUTE ";
			if (props.queueFlags & VK_QUEUE_TRANSFER_BIT)
				queueFlagsStr += "TRANSFER ";
			if (props.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
				queueFlagsStr += "SPARSE_BINDING ";
			if (props.queueFlags & VK_QUEUE_PROTECTED_BIT)
				queueFlagsStr += "PROTECTED ";
			if (props.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
				queueFlagsStr += "VIDEO_DECODE ";
			if (props.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
				queueFlagsStr += "VIDEO_ENCODE ";
			if (props.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
				queueFlagsStr += "OPTICAL_FLOW ";

			printLog("  Queue Family {}: {} queues, flags: {}", i, props.queueCount, queueFlagsStr);
		}

		uint32_t extCount = 0;
		printLog("\nEnumerating device extension properties...");
		vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extCount, nullptr);
		printLog("  Found {} device extensions", extCount);
		if (extCount > 0) {
			vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extCount,
				&extensions.front()) == VK_SUCCESS) {
				for (auto& ext : extensions) {
					supportedExtensions_.push_back(ext.extensionName);
				}
			}
		}
		printLog("Finished physical device selection.");

	}

	auto Context::device() -> VkDevice
	{
		return _device;
	}

	auto Context::instance() -> VkInstance
	{
		return _vkInstance;
	}

	auto Context::physicalDevice() -> VkPhysicalDevice
	{
		return _physicalDevice;
	}

	auto Context::graphicsCommandPool() const -> VkCommandPool
	{
		return graphicsCommandPool_;
	}

	auto Context::computeCommandPool() const -> VkCommandPool
	{
		return computeCommandPool_;
	}

	auto Context::transferCommandPool() const -> VkCommandPool
	{
		return transferCommandPool_;
	}

	auto Context::computeQueue() const -> VkQueue
	{
		return computeQueue_;
	}

	auto Context::computeQueueFamilyIndex() const -> uint32_t
	{
		return queueFamilyIndices_.compute;
	}

	auto Context::graphicsQueueFamilyIndex() const -> uint32_t
	{
		return queueFamilyIndices_.graphics;
	}

	auto Context::getQueueFamilyIndex(VkQueueFlags queueFlags) const -> uint32_t
	{
		if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags) {
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties_.size()); i++) {
				if ((queueFamilyProperties_[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
					((queueFamilyProperties_[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
					return i;
				}
			}
		}

		if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags) {
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties_.size()); i++) {
				if ((queueFamilyProperties_[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
					((queueFamilyProperties_[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
					((queueFamilyProperties_[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
					return i;
				}
			}
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties_.size()); i++) {
			if ((queueFamilyProperties_[i].queueFlags & queueFlags) == queueFlags) {
				return i;
			}
		}

		exitWithMessage("Could not find a queue family that supports the requested queue flags: {}",
			to_string(queueFlags));

		return uint32_t(-1); // To avoid compiler warnings
	}

	bool Context::extensionSupported(string extension) {
		for (const auto& supported : supportedExtensions_) {
			if (supported == extension) {
				return true;
			}
		}
		return false;
	}
	/*
	GPU媛 媛吏??щ윭 醫낅쪟??硫붾え由?以??곕━媛 ?꾩슂??議곌굔????留욌뒗 硫붾え由ъ쓽 踰덊샇(?몃뜳??瑜?李얠븘二쇰뒗 ?⑥닔
	*/
	auto Context::getMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) -> uint32_t {
		for (uint32_t i = 0; i < deviceMemoryProperties_.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (deviceMemoryProperties_.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		exitWithMessage("failed to find suitable memory type!");
		return uint32_t(-1);
	}

	auto Context::createTransferCommandBuffer(VkCommandBufferLevel level, bool begin) -> CommandBuffer {
		return CommandBuffer(_device, transferCommandPool_, transferQueue_, level, begin);
	}

	auto Context::createComputeCommandBuffer(VkCommandBufferLevel level, bool begin) -> CommandBuffer {
		return CommandBuffer(_device, computeCommandPool_, computeQueue_, level, begin);
	}

	auto Context::createCommandPool(uint32_t queueFamilyIndex,
		VkCommandPoolCreateFlags createFlags) -> VkCommandPool {
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndex;
		poolInfo.flags = createFlags;

		VkCommandPool commandPool;
		if (vkCreateCommandPool(_device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			exitWithMessage("failed to create command pool!");
		}

		return commandPool;
	}

}
