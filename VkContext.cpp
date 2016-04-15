#include "stdafx.h"
#include "VkContext.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "linmath.h"

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback (
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char*                                 pLayerPrefix,
    const char*                                 pMessage,
    void*                                       pUserData)
{
	std::ostringstream message;
	if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
		message << "ERROR: ";
	}
	else if( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
		message << "WARNING: ";
	}
	else if( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) {
		message << "PERFORMANCE WARNING: ";
	}
	else if( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
		message << "INFO: ";
	}
	else if( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
		message << "DEBUG: ";
	}

	message << "[" << pLayerPrefix << "] Code " << messageCode << " : " << pMessage;
	std::cout << message.str() << std::endl;

	assert((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) == 0);

	return false;
}


VkContext::VkContext(): m_hinstance( nullptr )
, m_hwnd( nullptr )
, m_VkInstance(VK_NULL_HANDLE)
, m_GPU(VK_NULL_HANDLE)
, m_Device(VK_NULL_HANDLE)
, m_Queue(VK_NULL_HANDLE)
, m_Surface(VK_NULL_HANDLE)
, m_SwapChain(VK_NULL_HANDLE)
, m_SwapChainImageCount(0)
, m_QueueFamilyIndex(-1)
, m_SurfaceFormat(VK_FORMAT_UNDEFINED)
, m_CommandPool(VK_NULL_HANDLE)
, m_CurrentSwapBuffer(0)
, m_AcquireImageSemaphore(VK_NULL_HANDLE)
, m_RenderingDoneSemaphore(VK_NULL_HANDLE)
, m_InitBuffer(VK_NULL_HANDLE)
, m_DescriptorPool(VK_NULL_HANDLE)
, m_RenderPass(VK_NULL_HANDLE)
{
}


VkContext::~VkContext()
{
}

VkContext* VkContext::GetInstance()
{
	static VkContext _instance;
	return &_instance;
}

bool VkContext::Initialize(HINSTANCE _hinstance, HWND _hwnd)
{
	m_hinstance = _hinstance;
	m_hwnd		= _hwnd;

	if( !InitializeInstanceLayerAndExt() )
		return false;
	if( !InitializeInstance() )
		return false;
	if( !InitializeDebugLayer() )
		return false;
	if( !InitializeSurfaces() )
		return false;
	if( !InitializeDevice() )
		return false;
	if( !InitializeSwapChain() )
		return false;
	if( !InitializeCmdBuffers() )
		return false;
	if( !InitializeDescriptorPool())
		return false;
	if( !InitializeDescriptorAndPipelineLayout() )
		return false;
	//Initialize global resourc3
	if ( BeginCommandBuffer(m_InitBuffer) ) {

		if( !InitializeSwapBuffers() )
			return false;
		if( !InitializeDepthBuffer() )
			return false;
		if( !LoadTextures() )
			return false;
		EndCommandBuffer(m_InitBuffer);
		FlushInitCommand(m_InitBuffer);
	} else {
		return false;
	}

	if( !InitializeRenderPass() )
		return false;

	if( !InitializeFrameBuffers() )
		return false;
	//
	// Initialize resources
	if( !LoadMeshData() )
		return false;

	if( !loadShader() )
		return false;

	if( !InitializePipeline())
		return false;

	if( !InitializeDescriptorSet() )
		return false;

	//draw something here
	for (uint32_t i = 0; i < m_SwapChainImageCount; i++) {
		if( !DebugDraw(m_SwapBuffers[i]) )
			return false;
	}
	//
	if( !InitializeFrameSync() )
		return false;

	return true;
}

bool VkContext::InitializeInstance()
{
	VkApplicationInfo _appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };

	_appInfo.apiVersion						= VK_MAKE_VERSION(1,0,3);//VK_API_VERSION;
	_appInfo.applicationVersion				= VK_MAKE_VERSION(0,1,0);
	_appInfo.engineVersion					= VK_MAKE_VERSION(0,1,0);
	_appInfo.pApplicationName				= "VkDemo";
	_appInfo.pEngineName					= "vkDemoEngine";

	VkInstanceCreateInfo _createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

	_createInfo.pApplicationInfo			= &_appInfo;
	_createInfo.enabledLayerCount			= m_InstanceLayerNames.size();
	_createInfo.ppEnabledLayerNames			= m_InstanceLayerNames.data();
	_createInfo.enabledExtensionCount		= m_InstanceExtensionNames.size();
	_createInfo.ppEnabledExtensionNames		= m_InstanceExtensionNames.data();

	VK_RETURN_IF_FAILED( vkCreateInstance(&_createInfo, nullptr, &m_VkInstance) );
	return true;
}

bool VkContext::InitializeDevice()
{
	//Enumerate the physical devices
	uint32_t _gpuCount = 0;
	VK_RETURN_IF_FAILED(vkEnumeratePhysicalDevices(m_VkInstance,&_gpuCount, nullptr));
	if( _gpuCount > 0 ) {
		std::unique_ptr<VkPhysicalDevice[]> _gpu ( new VkPhysicalDevice[_gpuCount] );
		VK_RETURN_IF_FAILED(vkEnumeratePhysicalDevices(m_VkInstance, &_gpuCount, _gpu.get()));
		m_GPU = _gpu[0];
	}

	if( !InitializeDeviceLayerAndExt() )
		return false;


	
	//Get the queue family index
	if( !InitializeQueueFamilyIndex() )
		return false;
	
	//prepare queue create info
	float _priorities_value[] = {1.0f};
	VkDeviceQueueCreateInfo _queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	_queueInfo.queueCount					= 1;
	_queueInfo.pQueuePriorities				= _priorities_value;
	_queueInfo.queueFamilyIndex				= m_QueueFamilyIndex;

	//Create Device
	VkDeviceCreateInfo _deviceInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	_deviceInfo.enabledLayerCount			= m_DeviceLayerNames.size();
	_deviceInfo.ppEnabledLayerNames			= m_DeviceLayerNames.data();
	_deviceInfo.enabledExtensionCount		= m_DeviceExtensionNames.size();
	_deviceInfo.ppEnabledExtensionNames		= m_DeviceExtensionNames.data();
	_deviceInfo.queueCreateInfoCount		= 1;									//It is a must to create 1 queue for graphic
	_deviceInfo.pQueueCreateInfos			= &_queueInfo;
	VK_RETURN_IF_FAILED( vkCreateDevice(m_GPU,&_deviceInfo, nullptr, &m_Device) );

	//Get the device queue
	vkGetDeviceQueue(m_Device, m_QueueFamilyIndex,0, &m_Queue);
	//Get the momery properties
	vkGetPhysicalDeviceMemoryProperties(m_GPU,&m_MemoryProperties);
	return true; 
}

bool VkContext::InitializeQueueFamilyIndex()
{
	uint32_t _familyPropCount = 0;
	uint32_t _queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_GPU, &_familyPropCount, nullptr);
	if (_familyPropCount > 0) {

		//Check which queue supports preset
		std::unique_ptr<VkBool32[]> _support_present(new VkBool32[_familyPropCount]);
		for (auto i = 0u; i < _familyPropCount; ++i)
		{
			VK_RETURN_IF_FAILED(vkGetPhysicalDeviceSurfaceSupportKHR(m_GPU, i, m_Surface, &_support_present[i]));
		}
		//Check which queue supports both preset and graphics
		std::unique_ptr<VkQueueFamilyProperties[]> _properties(new VkQueueFamilyProperties[_familyPropCount]);
		vkGetPhysicalDeviceQueueFamilyProperties(m_GPU, &_familyPropCount, _properties.get());
		for (auto i = 0u; i < _familyPropCount; ++i) {
			if (_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && _support_present[i] == VK_TRUE) {
				m_QueueFamilyIndex = i;
				break;
			}
		}
		if (m_QueueFamilyIndex == -1)
		{
			assert(0 && "failed to find propers queue family index!");
			return false;
		}

		VkPhysicalDeviceProperties _physical_properties;
		vkGetPhysicalDeviceProperties(m_GPU, &_physical_properties);
		std::cout << "GPU 0 : [" << _physical_properties.deviceName << "]" << std::endl;
	}
	return true;
}

bool VkContext::InitializeDeviceLayerAndExt()
{
	assert(m_GPU);
	//enumerate layer
	uint32_t _count = 0;
	VK_RETURN_IF_FAILED(vkEnumerateDeviceLayerProperties(m_GPU, &_count, nullptr));
	std::unique_ptr<VkLayerProperties[]> _vkLayerProperties(new VkLayerProperties[_count]);
	VK_RETURN_IF_FAILED( vkEnumerateDeviceLayerProperties(m_GPU, &_count, _vkLayerProperties.get()) );

	for( auto i = 0u ; i < _count ; ++i ) {
		std::cout << _vkLayerProperties[i].layerName << " | " << _vkLayerProperties[i].description << std::endl;
		m_DeviceLayerNames.push_back( _strdup( _vkLayerProperties[i].layerName ) );
	}

	//enumerate extension
	VK_RETURN_IF_FAILED(vkEnumerateDeviceExtensionProperties(m_GPU, nullptr, &_count, nullptr));
	std::unique_ptr<VkExtensionProperties[]> _vkExtensionProperties(new VkExtensionProperties[_count]);
	VK_RETURN_IF_FAILED(vkEnumerateDeviceExtensionProperties(m_GPU, nullptr, &_count, _vkExtensionProperties.get()));

	for (auto i = 0u ; i < _count; ++i) {
		std::cout << _vkExtensionProperties[i].extensionName << std::endl;
		m_DeviceExtensionNames.push_back( _strdup ( _vkExtensionProperties[i].extensionName ));
	}
	
	return true;
}

bool VkContext::InitializeDebugLayer()
{
	assert( m_VkInstance );
	PFN_vkCreateDebugReportCallbackEXT	_create_debug_callback	= (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(m_VkInstance, "vkCreateDebugReportCallbackEXT" );
	assert(_create_debug_callback);

	VkDebugReportCallbackCreateInfoEXT _createInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
	_createInfo.pNext		= nullptr;
	_createInfo.pUserData	= nullptr;
	_createInfo.flags		= VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT 
								| VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ;
	_createInfo.pfnCallback = &DebugCallback;

	VK_RETURN_IF_FAILED(_create_debug_callback(m_VkInstance, &_createInfo, nullptr, &m_DebugReportCallback));
	return true;
}

bool VkContext::DestroyDebugLayer()
{
	PFN_vkDestroyDebugReportCallbackEXT _destroy_debug_callback = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(m_VkInstance, "vkDestroyDebugReportCallbackEXT" );
	_destroy_debug_callback(m_VkInstance,m_DebugReportCallback,nullptr);
	return true;
}


bool VkContext::InitializeSwapChain()
{
	//Get cap 
	VkSurfaceCapabilitiesKHR _capabilities;
	VK_RETURN_IF_FAILED(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_GPU, m_Surface, &_capabilities));

	VkExtent2D _imageExtent				= _capabilities.currentExtent;
	assert( _imageExtent.width != -1 && _imageExtent.height != -1 );
	m_SurfaceExtent						= _imageExtent;

	uint32_t _minImageCount				= min( _capabilities.minImageCount + 1 , _capabilities.maxImageCount );

	//Get format
	uint32_t _format_count = 0;
	VK_RETURN_IF_FAILED(vkGetPhysicalDeviceSurfaceFormatsKHR(m_GPU, m_Surface, &_format_count, nullptr));

	std::unique_ptr<VkSurfaceFormatKHR[]> _surface_formats ( new VkSurfaceFormatKHR[_format_count]);

	VK_RETURN_IF_FAILED(vkGetPhysicalDeviceSurfaceFormatsKHR(m_GPU, m_Surface, &_format_count, _surface_formats.get()));
	
	m_SurfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
	if( _format_count >= 1 && _surface_formats[0].format != VK_FORMAT_UNDEFINED)
		m_SurfaceFormat = _surface_formats[0].format;

	//Get present mode
	uint32_t _present_mode_count = 0;
	VK_RETURN_IF_FAILED( vkGetPhysicalDeviceSurfacePresentModesKHR(m_GPU, m_Surface, &_present_mode_count, nullptr) );

	std::unique_ptr<VkPresentModeKHR[]> _present_modes ( new VkPresentModeKHR[_present_mode_count]);
	VK_RETURN_IF_FAILED( vkGetPhysicalDeviceSurfacePresentModesKHR(m_GPU, m_Surface, &_present_mode_count, _present_modes.get() ) );
	// 1. Mailbox , less letency , less tearing
	// 2. Immediate, fast but tearing
	// 3. FIFO, all support
	VkPresentModeKHR _presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for( auto i = 0u ; i < _present_mode_count; ++i ) {
		if( _present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR )	{
			_presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		if( _present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR && _presentMode != VK_PRESENT_MODE_MAILBOX_KHR ) {
			_presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}
	
	// Get preTransform
	VkSurfaceTransformFlagBitsKHR _preTransform = _capabilities.currentTransform;
	if( _preTransform & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
		_preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	// create swap chain
	VkSwapchainCreateInfoKHR _create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	_create_info.clipped				= VK_TRUE;
	_create_info.compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	_create_info.flags					= 0;
	_create_info.imageArrayLayers		= 1;
	_create_info.imageColorSpace		= VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	_create_info.imageExtent			= _imageExtent;			
	_create_info.imageFormat			= m_SurfaceFormat;			
	_create_info.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE;
	_create_info.imageUsage				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	_create_info.minImageCount			= _minImageCount;	
	_create_info.oldSwapchain			= NULL;
	_create_info.pNext					= nullptr;
	_create_info.presentMode			= _presentMode;
	_create_info.preTransform			= _preTransform;
	_create_info.queueFamilyIndexCount	= 0;
	_create_info.pQueueFamilyIndices	= nullptr;
	_create_info.surface				= m_Surface;

	VK_RETURN_IF_FAILED(vkCreateSwapchainKHR(m_Device, &_create_info, nullptr, &m_SwapChain));
	// get the image count
	VK_RETURN_IF_FAILED(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &m_SwapChainImageCount, nullptr ));

	return true;
}

bool VkContext::InitializeSwapBuffers()
{
	assert(m_SwapChainImageCount > 0);
	if( m_SwapChainImageCount > 0 ) {
		
		std::unique_ptr<VkImage[]> _swap_images ( new VkImage[m_SwapChainImageCount] );
		std::unique_ptr<VkImageView[]> _swap_image_views( new VkImageView[m_SwapChainImageCount]);
		VK_RETURN_IF_FAILED(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &m_SwapChainImageCount, _swap_images.get()));
		// create image views
		for( auto i = 0u; i < m_SwapChainImageCount ; ++i ) {

			VkImageViewCreateInfo _image_view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
			_image_view_info.image								= _swap_images[i];
			_image_view_info.viewType							= VK_IMAGE_VIEW_TYPE_2D;
			_image_view_info.format								= m_SurfaceFormat;
			_image_view_info.components.r						= VK_COMPONENT_SWIZZLE_R;
			_image_view_info.components.g						= VK_COMPONENT_SWIZZLE_G;
			_image_view_info.components.b						= VK_COMPONENT_SWIZZLE_B;
			_image_view_info.components.a						= VK_COMPONENT_SWIZZLE_A;
			_image_view_info.subresourceRange.levelCount		= 1;
			_image_view_info.subresourceRange.baseMipLevel		= 0;
			_image_view_info.subresourceRange.layerCount		= 1;
			_image_view_info.subresourceRange.baseArrayLayer	= 0;
			_image_view_info.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;

			VK_RETURN_IF_FAILED( vkCreateImageView(m_Device,&_image_view_info,nullptr, &_swap_image_views[i] ) );

			m_SwapBuffers[i].image				= _swap_images[i];
			m_SwapBuffers[i].imageView			= _swap_image_views[i];

			// transition image from presentable state to color attachment state
			SetImageLayout(m_InitBuffer, VK_IMAGE_ASPECT_COLOR_BIT, _swap_images[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, (VkAccessFlagBits)0);

		}
	} else {
		return false;
	}

	return true;
}

bool VkContext::InitializeSurfaces()
{
	assert( m_VkInstance );
	assert( m_hinstance );
	assert( m_hwnd );
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR _create_info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	_create_info.hinstance			= m_hinstance;
	_create_info.hwnd				= m_hwnd;
	VK_RETURN_IF_FAILED( vkCreateWin32SurfaceKHR(m_VkInstance,&_create_info, nullptr,&m_Surface) );
#endif
	return true;
}

bool VkContext::InitializeCmdBuffers()
{
	//Create command buffer pool
	VkCommandPoolCreateInfo _cmd_pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	_cmd_pool_info.queueFamilyIndex = m_QueueFamilyIndex;
	_cmd_pool_info.flags			= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_RETURN_IF_FAILED( vkCreateCommandPool(m_Device, &_cmd_pool_info, nullptr, &m_CommandPool) );

	VkCommandBufferAllocateInfo _alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	_alloc_info.commandBufferCount	= 1;
	_alloc_info.commandPool			= m_CommandPool;
	_alloc_info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	for( auto i = 0u ; i < m_SwapChainImageCount; ++i ) {
		VKSwapBuffer _b = {};
		VK_RETURN_IF_FAILED(vkAllocateCommandBuffers(m_Device, &_alloc_info, &_b.commandBuffer));
		m_SwapBuffers.push_back(_b);
	}

	VK_RETURN_IF_FAILED(vkAllocateCommandBuffers(m_Device, &_alloc_info, &m_InitBuffer));
	
	return true;
}

bool VkContext::InitializeFrameSync()
{
	VkSemaphoreCreateInfo _semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	
	VK_RETURN_IF_FAILED( vkCreateSemaphore(m_Device, &_semaphore_info, nullptr, &m_AcquireImageSemaphore));
	VK_RETURN_IF_FAILED( vkCreateSemaphore(m_Device, &_semaphore_info, nullptr, &m_RenderingDoneSemaphore));

	VkFenceCreateInfo _fence_create_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	for( auto i = 0u ; i < m_SwapChainImageCount ; ++i ) {
		VK_RETURN_IF_FAILED( vkCreateFence(m_Device, &_fence_create_info, nullptr, &m_SwapBuffers[i].submitFence));
	}

	return true;
}

bool VkContext::InitializeDepthBuffer()
{
	VkImage _depth_buffer_image		= VK_NULL_HANDLE;
	VkImageView _depth_buffer_view	= VK_NULL_HANDLE;

	VkImageCreateInfo _create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    _create_info.imageType						= VkImageType::VK_IMAGE_TYPE_2D;
    _create_info.format							= VkFormat::VK_FORMAT_D16_UNORM;
	_create_info.extent							= { m_SurfaceExtent.width, m_SurfaceExtent.height, 1 };
    _create_info.mipLevels						= 1;
    _create_info.arrayLayers					= 1;
    _create_info.samples						= VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    _create_info.tiling							= VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
    _create_info.usage							= VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VK_RETURN_IF_FAILED( vkCreateImage(m_Device,&_create_info,nullptr,&_depth_buffer_image));
	m_DepthBuffer.image							= _depth_buffer_image;
	m_DepthBuffer.format						= VkFormat::VK_FORMAT_D16_UNORM;

	VkMemoryRequirements _memory_requirement;

	vkGetImageMemoryRequirements(m_Device,_depth_buffer_image,&_memory_requirement);
	//alloc memory
	VkDeviceMemory _device_memory;
	if( !AllocVideoMemory( _memory_requirement, 0, &_device_memory))
		return false;
	//bind memory
	VK_RETURN_IF_FAILED( vkBindImageMemory(m_Device,_depth_buffer_image,_device_memory,0) );
	m_DepthBuffer.memory						= _device_memory;

	SetImageLayout(m_InitBuffer,VK_IMAGE_ASPECT_DEPTH_BIT,_depth_buffer_image,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,(VkAccessFlagBits)0);

	VkImageSubresourceRange _subresource = {};
	_subresource.aspectMask						= VK_IMAGE_ASPECT_DEPTH_BIT;
	_subresource.baseArrayLayer					= 0;
	_subresource.baseMipLevel					= 0;
	_subresource.layerCount						= 1;
	_subresource.levelCount						= 1;

	VkImageViewCreateInfo _create_view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	_create_view_info.format					= VK_FORMAT_D16_UNORM;
	_create_view_info.image						= _depth_buffer_image;
	_create_view_info.subresourceRange			= _subresource;
	_create_view_info.viewType					= VK_IMAGE_VIEW_TYPE_2D;

	VK_RETURN_IF_FAILED(vkCreateImageView(m_Device,&_create_view_info,nullptr,&_depth_buffer_view));
	m_DepthBuffer.imageView						= _depth_buffer_view;

	return true;
}

bool VkContext::InitializeFrameBuffers()
{
	for( auto i = 0u; i < m_SwapChainImageCount ; ++i ) {
		VkImageView attachment[2] = {
			m_SwapBuffers[i].imageView,
			m_DepthBuffer.imageView,
		};
		VkFramebufferCreateInfo _create_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		_create_info.renderPass						= m_RenderPass;
		_create_info.attachmentCount				= 2;
		_create_info.pAttachments					= attachment;
		_create_info.width							= m_SurfaceExtent.width;
		_create_info.height							= m_SurfaceExtent.height;
		_create_info.layers							= 1;
		VK_RETURN_IF_FAILED( vkCreateFramebuffer(m_Device, &_create_info, nullptr, &m_SwapBuffers[i].frameBuffer) );
	}
	return true;
}

bool VkContext::InitializeRenderPass()
{
	VkAttachmentDescription		_attachments[2] = {};
	_attachments[0].format							= m_SurfaceFormat;
	_attachments[0].samples							= VK_SAMPLE_COUNT_1_BIT;
	_attachments[0].loadOp							= VK_ATTACHMENT_LOAD_OP_CLEAR;
	_attachments[0].storeOp							= VK_ATTACHMENT_STORE_OP_STORE;
	_attachments[0].stencilLoadOp					= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	_attachments[0].stencilStoreOp					= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	_attachments[0].initialLayout					= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	_attachments[0].finalLayout						= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	_attachments[1].format							= m_DepthBuffer.format;
	_attachments[1].samples							= VK_SAMPLE_COUNT_1_BIT;
	_attachments[1].loadOp							= VK_ATTACHMENT_LOAD_OP_CLEAR;
	_attachments[1].storeOp							= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	_attachments[1].stencilLoadOp					= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	_attachments[1].stencilStoreOp					= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	_attachments[1].initialLayout					= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	_attachments[1].finalLayout						= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference		_color_reference = {
		0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference		_depth_reference = {
		1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription	_subpass = {};
	_subpass.pipelineBindPoint						= VK_PIPELINE_BIND_POINT_GRAPHICS;
	_subpass.inputAttachmentCount					= 0;
	_subpass.pInputAttachments						= nullptr;
	_subpass.colorAttachmentCount					= 1;
	_subpass.pColorAttachments						= &_color_reference;
	_subpass.pResolveAttachments					= nullptr;
	_subpass.pDepthStencilAttachment				= &_depth_reference;
	_subpass.preserveAttachmentCount				= 0;
	_subpass.pPreserveAttachments					= nullptr;

	VkRenderPassCreateInfo _create_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	_create_info.attachmentCount					= 2;
	_create_info.pAttachments						= _attachments;
	_create_info.subpassCount						= 1;
	_create_info.pSubpasses							= &_subpass;
	_create_info.dependencyCount					= 0;
	_create_info.pDependencies						= nullptr;

	VK_RETURN_IF_FAILED( vkCreateRenderPass(m_Device,&_create_info,nullptr,&m_RenderPass));

	return true;
}

bool VkContext::InitializeDescriptorPool()
{
	VkDescriptorPoolSize _pool_sizes[2];
	_pool_sizes[0].descriptorCount					= 1;
	_pool_sizes[0].type								= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	_pool_sizes[1].descriptorCount					= 1;					// the size of textures	
	_pool_sizes[1].type								= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo _create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };

	_create_info.maxSets							= 1;
	_create_info.poolSizeCount						= 2;
	_create_info.pPoolSizes							= _pool_sizes;
	VK_RETURN_IF_FAILED( vkCreateDescriptorPool(m_Device,&_create_info,nullptr, &m_DescriptorPool) );
	return true;
}

bool VkContext::InitializeDescriptorAndPipelineLayout()
{
	VkDescriptorSetLayoutBinding _layout_bindings[2] = {}; 
	_layout_bindings[0].binding							= 0;
	_layout_bindings[0].descriptorType					= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	_layout_bindings[0].descriptorCount					= 1;
	_layout_bindings[0].stageFlags						= VK_SHADER_STAGE_VERTEX_BIT;
	_layout_bindings[0].pImmutableSamplers				= NULL;

	_layout_bindings[1].binding							= 1;
	_layout_bindings[1].descriptorType					= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	_layout_bindings[1].descriptorCount					= 1;
	_layout_bindings[1].stageFlags						= VK_SHADER_STAGE_FRAGMENT_BIT;
	_layout_bindings[1].pImmutableSamplers				= NULL;

	VkDescriptorSetLayoutCreateInfo _descriptor_layout = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	_descriptor_layout.pNext							= NULL;
	_descriptor_layout.bindingCount						= sizeof(_layout_bindings)/sizeof(VkDescriptorSetLayoutBinding);
	_descriptor_layout.pBindings						= _layout_bindings;

	VK_RETURN_IF_FAILED(vkCreateDescriptorSetLayout(m_Device, &_descriptor_layout, nullptr, &m_DescriptorSetLayout));

	VkPipelineLayoutCreateInfo _pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	_pipelineLayoutCreateInfo.setLayoutCount			= 1;
	_pipelineLayoutCreateInfo.pSetLayouts				= &m_DescriptorSetLayout;

	VK_RETURN_IF_FAILED(vkCreatePipelineLayout(m_Device, &_pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));
	return true;
}

bool VkContext::InitializeDescriptorSet()
{
	VkDescriptorSetAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	alloc_info.descriptorPool				= m_DescriptorPool;
	alloc_info.descriptorSetCount			= 1;
	alloc_info.pSetLayouts					= &m_DescriptorSetLayout;

	VK_RETURN_IF_FAILED(vkAllocateDescriptorSets(m_Device, &alloc_info, &m_DescriptorSet));

	VkDescriptorImageInfo _tex_descs = {};
	_tex_descs.sampler						= m_TextureObject.sampler;
	_tex_descs.imageView					= m_TextureObject.imageView;
	_tex_descs.imageLayout					= VK_IMAGE_LAYOUT_GENERAL;

	VkWriteDescriptorSet _writes[2]	= {};

	_writes[0].sType						= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	_writes[0].dstSet						= m_DescriptorSet;
	_writes[0].descriptorCount				= 1;
	_writes[0].descriptorType				= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	_writes[0].pBufferInfo					= &m_UniformBufferObject.buffer_info;

	_writes[1].sType						= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	_writes[1].dstSet						= m_DescriptorSet;
	_writes[1].dstBinding					= 1;
	_writes[1].descriptorCount				= 1;
	_writes[1].descriptorType				= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	_writes[1].pImageInfo					= &_tex_descs;

	vkUpdateDescriptorSets(m_Device, 2, _writes, 0, NULL);
	return true;
}

bool VkContext::InitializePipeline()
{
	//create pipeline cache
	VkPipelineCacheCreateInfo pipelineCache = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	VK_RETURN_IF_FAILED(vkCreatePipelineCache(m_Device, &pipelineCache, NULL, &m_PipelineCache));
	//
	VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE] = {};
	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO } ;
	dynamicState.pDynamicStates							= dynamicStateEnables;


	VkPipelineVertexInputStateCreateInfo vi = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	VkPipelineInputAssemblyStateCreateInfo ia = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rs = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rs.polygonMode										= VK_POLYGON_MODE_FILL;
	rs.cullMode											= VK_CULL_MODE_BACK_BIT;
	rs.frontFace										= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rs.depthClampEnable									= VK_FALSE;
	rs.rasterizerDiscardEnable							= VK_FALSE;
	rs.depthBiasEnable									= VK_FALSE;


	VkPipelineColorBlendAttachmentState att_state[1] = {};
	att_state[0].colorWriteMask							= 0xf;
	att_state[0].blendEnable							= VK_FALSE;

	VkPipelineColorBlendStateCreateInfo cb = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	cb.attachmentCount									= 1;
	cb.pAttachments										= att_state;

	VkPipelineViewportStateCreateInfo vp = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	vp.viewportCount									= 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
	vp.scissorCount										= 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

	VkPipelineDepthStencilStateCreateInfo ds = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	ds.depthTestEnable									= VK_TRUE;
	ds.depthWriteEnable									= VK_TRUE;
	ds.depthCompareOp									= VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.depthBoundsTestEnable							= VK_FALSE;
	ds.back.failOp										= VK_STENCIL_OP_KEEP;
	ds.back.passOp										= VK_STENCIL_OP_KEEP;
	ds.back.compareOp									= VK_COMPARE_OP_ALWAYS;
	ds.stencilTestEnable								= VK_FALSE;
	ds.front											= ds.back;

	VkPipelineMultisampleStateCreateInfo ms = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	ms.pSampleMask = NULL;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Two stages: vs and fs
	VkPipelineShaderStageCreateInfo shaderStages[2] = {};

	shaderStages[0].sType								= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage								= VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module								= m_VertexShaderModule;
	shaderStages[0].pName								= "main";

	shaderStages[1].sType								= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage								= VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module								= m_PixelShaderModule;
	shaderStages[1].pName								= "main";

	VkGraphicsPipelineCreateInfo pipeline = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipeline.layout										= m_PipelineLayout;
	pipeline.pVertexInputState							= &vi;
	pipeline.pInputAssemblyState						= &ia;
	pipeline.pRasterizationState						= &rs;
	pipeline.pColorBlendState							= &cb;
	pipeline.pMultisampleState							= &ms;
	pipeline.pViewportState								= &vp;
	pipeline.pDepthStencilState							= &ds;
	pipeline.stageCount									= 2;
	pipeline.pStages									= shaderStages;
	pipeline.renderPass									= m_RenderPass;
	pipeline.pDynamicState								= &dynamicState;

	VK_RETURN_IF_FAILED(vkCreateGraphicsPipelines(m_Device, m_PipelineCache, 1, &pipeline, NULL, &m_Pipeline));

	vkDestroyShaderModule(m_Device, m_VertexShaderModule, NULL);
	m_VertexShaderModule								= VK_NULL_HANDLE;
	vkDestroyShaderModule(m_Device, m_PixelShaderModule, NULL);
	m_PixelShaderModule									= VK_NULL_HANDLE;

	return true;
}
/* Load a ppm file into memory */
static bool loadTexture(const char *filename, uint8_t *rgba_data,
	VkSubresourceLayout *layout, int32_t *width, int32_t *height) {

	FILE *fPtr = NULL; 
	fopen_s(&fPtr,filename, "rb");
	char header[256], *cPtr, *tmp;

	if (!fPtr)
		return false;

	cPtr = fgets(header, 256, fPtr); // P6
	if (cPtr == NULL || strncmp(header, "P6\n", 3)) {
		fclose(fPtr);
		return false;
	}

	do {
		cPtr = fgets(header, 256, fPtr);
		if (cPtr == NULL) {
			fclose(fPtr);
			return false;
		}
	} while (!strncmp(header, "#", 1));

	sscanf_s(header, "%u %u", height, width);
	if (rgba_data == NULL) {
		fclose(fPtr);
		return true;
	}
	tmp = fgets(header, 256, fPtr); // Format
	(void)tmp;
	if (cPtr == NULL || strncmp(header, "255\n", 3)) {
		fclose(fPtr);
		return false;
	}

	for (int y = 0; y < *height; y++) {
		uint8_t *rowPtr = rgba_data;
		for (int x = 0; x < *width; x++) {
			size_t s = fread(rowPtr, 3, 1, fPtr);
			(void)s;
			rowPtr[3] = 255; /* Alpha of 1 */
			rowPtr += 4;
		}
		rgba_data += layout->rowPitch;
	}
	fclose(fPtr);
	return true;
}

bool VkContext::LoadTextures()
{
	VKTextureObject	_tex_obj;
	const VkFormat _tex_format						= VK_FORMAT_R8G8B8A8_UNORM;
	const VkImageTiling _tiling						= VK_IMAGE_TILING_LINEAR;
	const VkImageUsageFlags _usage					= VK_IMAGE_USAGE_SAMPLED_BIT;
	
	VkFormatProperties	_tex_properties;

	vkGetPhysicalDeviceFormatProperties(m_GPU, _tex_format, &_tex_properties);

	if( _tex_properties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) {

		int32_t _width								= 0;
		int32_t _height								= 0;

    	if (!loadTexture("lunarg.ppm", NULL, NULL, &_width, &_height)) {
			assert(0);
			return false;
    	}

    	_tex_obj.width								= _width;
    	_tex_obj.height								= _height;

    	VkImageCreateInfo image_create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    	image_create_info.imageType					= VK_IMAGE_TYPE_2D;
    	image_create_info.format					= _tex_format;
    	image_create_info.extent					= {_width, _height, 1};
    	image_create_info.mipLevels					= 1;
    	image_create_info.arrayLayers				= 1;
    	image_create_info.samples					= VK_SAMPLE_COUNT_1_BIT;
    	image_create_info.tiling					= _tiling;
    	image_create_info.usage						= _usage;
    	image_create_info.flags						= 0;
    	image_create_info.initialLayout				= VK_IMAGE_LAYOUT_PREINITIALIZED;

    	VkMemoryRequirements _mem_reqs;

    	VK_RETURN_IF_FAILED( vkCreateImage(m_Device, &image_create_info, NULL, &_tex_obj.image) );

    	vkGetImageMemoryRequirements(m_Device, _tex_obj.image, &_mem_reqs);
		// alloc memory 
		if( !AllocVideoMemory(_mem_reqs,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,&_tex_obj.memory) ) {
			assert(0);
			return false;
		}

    	VK_RETURN_IF_FAILED( vkBindImageMemory(m_Device, _tex_obj.image, _tex_obj.memory, 0) );

		//only when VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT is set
		VkImageSubresource _subres = {};
		_subres.aspectMask							= VK_IMAGE_ASPECT_COLOR_BIT;
		_subres.mipLevel							= 0;
		_subres.arrayLayer							= 0;

		VkSubresourceLayout _layout;
		void *_data;

		vkGetImageSubresourceLayout(m_Device, _tex_obj.image, &_subres, &_layout);

		VK_RETURN_IF_FAILED(vkMapMemory(m_Device, _tex_obj.memory, 0, _mem_reqs.size, 0, &_data));

		if (!loadTexture("lunarg.ppm", (uint8_t*)_data, &_layout, &_width, &_height)) {
			assert(0);
			return false;
		}

		vkUnmapMemory(m_Device, _tex_obj.memory);

    	_tex_obj.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    	SetImageLayout(m_InitBuffer, VK_IMAGE_ASPECT_COLOR_BIT,_tex_obj.image, VK_IMAGE_LAYOUT_PREINITIALIZED, _tex_obj.imageLayout, VK_ACCESS_HOST_WRITE_BIT);
    	/* setting the image layout does not reference the actual memory so no need
    	 * to add a mem ref */

		//create sampler
		VkSamplerCreateInfo _sampler = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		_sampler.magFilter								= VK_FILTER_NEAREST;
		_sampler.minFilter								= VK_FILTER_NEAREST;
		_sampler.mipmapMode								= VK_SAMPLER_MIPMAP_MODE_NEAREST;
		_sampler.addressModeU							= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		_sampler.addressModeV							= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		_sampler.addressModeW							= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		_sampler.mipLodBias								= 0.0f;
		_sampler.anisotropyEnable						= VK_FALSE;
		_sampler.maxAnisotropy							= 1;
		_sampler.compareOp								= VK_COMPARE_OP_NEVER;
		_sampler.minLod									= 0.0f;
		_sampler.maxLod									= 0.0f;
		_sampler.borderColor							= VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		_sampler.unnormalizedCoordinates				= VK_FALSE;

		VkImageViewCreateInfo _view = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		_view.image										= _tex_obj.image;
		_view.viewType									= VK_IMAGE_VIEW_TYPE_2D;
		_view.format									= _tex_format;
		_view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A, };
		_view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		/* create sampler */
		VK_RETURN_IF_FAILED(vkCreateSampler(m_Device, &_sampler, NULL, &_tex_obj.sampler));

		/* create image view */
		VK_RETURN_IF_FAILED(vkCreateImageView(m_Device, &_view, NULL, &_tex_obj.imageView));

		m_TextureObject = _tex_obj;

	}


	return true;
}

bool VkContext::LoadMeshData()
{
	vec3 eye = { 0.0f, 3.0f, 5.0f };
	vec3 origin = { 0, 0, 0 };
	vec3 up = { 0.0f, 1.0f, 0.0 };

	mat4x4 projection_matrix;
	mat4x4 view_matrix;
	mat4x4 model_matrix;

	float aspect = m_SurfaceExtent.width / float( m_SurfaceExtent.height);	
	mat4x4_perspective(projection_matrix, (float)degreesToRadians(45.0f),
		aspect, 0.1f, 100.0f);
	mat4x4_look_at(view_matrix, eye, origin, up);
	mat4x4_identity(model_matrix);

	struct vktexcube_vs_uniform {
		// Must start with MVP
		float mvp[4][4];
		float position[12 * 3][4];
		float attr[12 * 3][4];
	};
	mat4x4 MVP, VP;
	struct vktexcube_vs_uniform data;

	mat4x4_mul(VP, projection_matrix, view_matrix);
	mat4x4_mul(MVP, VP, model_matrix);
	memcpy(data.mvp, MVP, sizeof(MVP));
	//    dumpMatrix("MVP", MVP);

	static const float g_vertex_buffer_data[] = {
		-1.0f, -1.0f, -1.0f,  // -X side
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,  // -Z side
		1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,  // -Y side
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		-1.0f, 1.0f, -1.0f,  // +Y side
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,

		1.0f, 1.0f, -1.0f,  // +X side
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		-1.0f, 1.0f, 1.0f,  // +Z side
		-1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
	};

	static const float g_uv_buffer_data[] = {
		0.0f, 0.0f,  // -X side
		1.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,

		1.0f, 0.0f,  // -Z side
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		1.0f, 1.0f,  // -Y side
		1.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,

		1.0f, 1.0f,  // +Y side
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,

		1.0f, 1.0f,  // +X side
		0.0f, 1.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,  // +Z side
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
	};


	for ( auto i = 0; i < 12 * 3; i++) {
		data.position[i][0]			= g_vertex_buffer_data[i * 3];
		data.position[i][1] 		= g_vertex_buffer_data[i * 3 + 1];
		data.position[i][2] 		= g_vertex_buffer_data[i * 3 + 2];
		data.position[i][3] 		= 1.0f;
		data.attr[i][0]				= g_uv_buffer_data[2 * i];
		data.attr[i][1] 			= g_uv_buffer_data[2 * i + 1];
		data.attr[i][2] 			= 0;
		data.attr[i][3] 			= 0;
	}

	VkBufferCreateInfo buf_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = sizeof(data);
	VK_RETURN_IF_FAILED(vkCreateBuffer(m_Device, &buf_info, NULL, &m_UniformBufferObject.buffer));

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(m_Device, m_UniformBufferObject.buffer, &mem_reqs);

	if( !AllocVideoMemory( mem_reqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_UniformBufferObject.memory)) {
		assert(0);
		return false;
	}
	uint8_t *pData = nullptr;
	VK_RETURN_IF_FAILED( vkMapMemory(m_Device, m_UniformBufferObject.memory, 0, mem_reqs.size, 0, (void **)&pData));

	memcpy(pData, &data, sizeof data);

	vkUnmapMemory(m_Device, m_UniformBufferObject.memory);

	VK_RETURN_IF_FAILED( vkBindBufferMemory(m_Device, m_UniformBufferObject.buffer, m_UniformBufferObject.memory, 0)); 

	m_UniformBufferObject.buffer_info.buffer		= m_UniformBufferObject.buffer;
	m_UniformBufferObject.buffer_info.offset		= 0;
	m_UniformBufferObject.buffer_info.range			= sizeof(data);
	return true;
}

bool VkContext::loadShader()
{
	VkShaderModule*		_loaded[2] = {
		&m_VertexShaderModule,
		&m_PixelShaderModule,
	};

	const char*			_shader_names[2] = {
		"cube-vert.spv",
		"cube-frag.spv",
	};

	for( auto i = 0 ; i < 2 ; ++i ) {

		FILE *fp = NULL;
		fopen_s(&fp,_shader_names[i], "rb");
		assert(fp);
		fseek(fp, 0L, SEEK_END);
		long size = ftell(fp);

		fseek(fp, 0L, SEEK_SET);

		std::unique_ptr<uint8_t[]> shader_code(new uint8_t[size]);
		fread_s(shader_code.get(), size, 1, size, fp);

		fclose(fp);

		VkShaderModuleCreateInfo moduleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		moduleCreateInfo.codeSize					= size;
		moduleCreateInfo.pCode						= (uint32_t*)shader_code.get();
		VK_RETURN_IF_FAILED(vkCreateShaderModule(m_Device, &moduleCreateInfo, NULL, _loaded[i]));
	}
	return true;
}

bool VkContext::AllocVideoMemory(const VkMemoryRequirements & _memory_requirement, VkFlags required_mask, VkDeviceMemory* _device_memory)
{
	VkMemoryAllocateInfo _memory_alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	_memory_alloc_info.allocationSize					= _memory_requirement.size;

	uint32_t _type_bits = _memory_requirement.memoryTypeBits;

	for( auto i = 0u; i < m_MemoryProperties.memoryTypeCount; ++i ) {
		if( (_type_bits & 1 ) ) {
			if( (m_MemoryProperties.memoryTypes[i].propertyFlags & required_mask ) == required_mask ) {
				_memory_alloc_info.memoryTypeIndex			= i;
				break;
			}
		}
		
		_type_bits >>= 1;
	}

	assert(_device_memory);
	//alloc memory
	VK_RETURN_IF_FAILED(vkAllocateMemory(m_Device,&_memory_alloc_info,nullptr,_device_memory));

	return true;
}

bool VkContext::FlushInitCommand(VkCommandBuffer _cmd)
{
	VkSubmitInfo _submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	_submit_info.commandBufferCount = 1;
	_submit_info.pCommandBuffers = &_cmd;
	VK_RETURN_IF_FAILED(vkQueueSubmit(m_Queue, 1, &_submit_info, VK_NULL_HANDLE));
	VK_RETURN_IF_FAILED(vkQueueWaitIdle(m_Queue));

	return true;
}

bool VkContext::BeginCommandBuffer(VkCommandBuffer _cmd)
{
	VkCommandBufferBeginInfo _begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VK_RETURN_IF_FAILED( vkBeginCommandBuffer( _cmd, &_begin_info) );
	return true;
}

bool VkContext::EndCommandBuffer(VkCommandBuffer _cmd)
{
	VK_RETURN_IF_FAILED(vkEndCommandBuffer(_cmd));
	return true;
}

void VkContext::SetImageLayout( VkCommandBuffer _cmd, VkImageAspectFlags  _aspectMask, VkImage _image, VkImageLayout _old_layout, VkImageLayout _new_layout, VkAccessFlagBits _srcAccessFlags)
{
	VkImageMemoryBarrier _image_memory_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	_image_memory_barrier.srcAccessMask						= _srcAccessFlags;
	_image_memory_barrier.dstAccessMask						= 0;
	_image_memory_barrier.oldLayout							= _old_layout;
	_image_memory_barrier.newLayout							= _new_layout;
	_image_memory_barrier.image								= _image;
	_image_memory_barrier.subresourceRange.aspectMask		= _aspectMask;
	_image_memory_barrier.subresourceRange.layerCount		= 1;
	_image_memory_barrier.subresourceRange.baseArrayLayer	= 0;
	_image_memory_barrier.subresourceRange.levelCount		= 1;
	_image_memory_barrier.subresourceRange.baseMipLevel		= 0;
#if 0
	if (_old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		_image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	
	if (_new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	
	if (_new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	
	if (_old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	
	if (_old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
		_image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	}
	
	if (_new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		_image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		_image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	
	if (_new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		_image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	
	if (_new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		_image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
#else
	if (_new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		/* Make sure anything that was copying from this image has completed */
		_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	if (_new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		_image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (_new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		_image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	if (_new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		/* Make sure any Copy or CPU writes to image are flushed */
		_image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	}

#endif
	
	VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	
	vkCmdPipelineBarrier(_cmd, src_stages, dest_stages, 0, 0, nullptr, 0, nullptr, 1, &_image_memory_barrier);
}

bool VkContext::InitializeInstanceLayerAndExt()
{
	//enumerate layer
	uint32_t _count = 0;
	VK_RETURN_IF_FAILED(vkEnumerateInstanceLayerProperties(&_count,nullptr));
	std::unique_ptr<VkLayerProperties[]> _vkLayerProperties(new VkLayerProperties[_count]);
	VK_RETURN_IF_FAILED(vkEnumerateInstanceLayerProperties(&_count, _vkLayerProperties.get()));
	for( auto i = 0u ; i < _count ; ++i ) {
		std::cout << _vkLayerProperties[i].layerName << " | " << _vkLayerProperties[i].description << std::endl;
		m_InstanceLayerNames.push_back( _strdup(_vkLayerProperties[i].layerName) );
	}
	//enumerate extension
	VK_RETURN_IF_FAILED(vkEnumerateInstanceExtensionProperties(nullptr, &_count, nullptr));
	std::unique_ptr<VkExtensionProperties[]> _vkExtensionProperties(new VkExtensionProperties[_count]);
	VK_RETURN_IF_FAILED(vkEnumerateInstanceExtensionProperties(nullptr, &_count, _vkExtensionProperties.get()));
	for (auto i = 0u ; i < _count; ++i) {
		std::cout << _vkExtensionProperties[i].extensionName << std::endl;
		m_InstanceExtensionNames.push_back(_strdup( _vkExtensionProperties[i].extensionName) );
	}
	
	return true;
}

void VkContext::Destroy()
{
	//destroy pipeline
	vkDestroyPipeline(m_Device,m_Pipeline,nullptr);
	vkDestroyPipelineCache(m_Device,m_PipelineCache,nullptr);
	//destroy descriptor set layout
	vkDestroyDescriptorSetLayout(m_Device,m_DescriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(m_Device,m_PipelineLayout, nullptr);
	//destroy uniform buffer
	vkDestroyBuffer(m_Device,m_UniformBufferObject.buffer,nullptr);
	vkFreeMemory(m_Device,m_UniformBufferObject.memory,nullptr);
	//destroy texture
	vkDestroyImageView(m_Device,m_TextureObject.imageView,nullptr);
	vkDestroyImage(m_Device,m_TextureObject.image,nullptr);
	vkDestroySampler(m_Device,m_TextureObject.sampler,nullptr);
	vkFreeMemory(m_Device,m_TextureObject.memory,nullptr);
	//destroy render pass
	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
	//destroy descriptor pool
	vkDestroyDescriptorPool(m_Device, m_DescriptorPool,nullptr);
	//destroy framebuffer
	for( auto i = 0u ; i < m_SwapChainImageCount; ++i) {
		vkDestroyFramebuffer(m_Device,m_SwapBuffers[i].frameBuffer,nullptr);
	}
	//destroy depth buffer
	vkDestroyImageView(m_Device,m_DepthBuffer.imageView,nullptr);
	vkDestroyImage(m_Device,m_DepthBuffer.image,nullptr);
	vkFreeMemory(m_Device,m_DepthBuffer.memory,nullptr);

	//
	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &m_InitBuffer);
	m_InitBuffer = VK_NULL_HANDLE;
	vkDestroySemaphore(m_Device, m_AcquireImageSemaphore, nullptr);
	m_AcquireImageSemaphore = VK_NULL_HANDLE;
	vkDestroySemaphore(m_Device, m_RenderingDoneSemaphore, nullptr);
	m_RenderingDoneSemaphore = VK_NULL_HANDLE;
	for( auto i = 0u ; i < m_SwapChainImageCount; ++i) {
		vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &m_SwapBuffers[i].commandBuffer);
		vkDestroyImageView(m_Device, m_SwapBuffers[i].imageView, nullptr );
		vkDestroyFence(m_Device, m_SwapBuffers[i].submitFence, nullptr);
	}
	m_SwapBuffers.clear();
	vkDestroyCommandPool(m_Device,m_CommandPool,nullptr);
	m_CommandPool = VK_NULL_HANDLE;
	vkDestroySwapchainKHR(m_Device, m_SwapChain,nullptr);
	m_SwapChain = VK_NULL_HANDLE;
	vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);
	m_Surface = VK_NULL_HANDLE;
	vkDestroyDevice(m_Device,nullptr);
	m_Device = nullptr;
	//remove debug callback
	DestroyDebugLayer();
	//destroy instance
	vkDestroyInstance(m_VkInstance, nullptr);
	m_VkInstance = nullptr;

	std::for_each( m_InstanceLayerNames.begin(), m_InstanceLayerNames.end(), free);
	m_InstanceLayerNames.clear();

	std::for_each( m_InstanceExtensionNames.begin(), m_InstanceExtensionNames.end(), free);
	m_InstanceExtensionNames.clear();

	std::for_each( m_DeviceLayerNames.begin(), m_DeviceLayerNames.end(), free );
	m_DeviceLayerNames.clear();

	std::for_each( m_DeviceExtensionNames.begin(), m_DeviceExtensionNames.end(), free );
	m_DeviceExtensionNames.clear();
}

void VkContext::Run()
{
	if( BeginFrame() )
	{
		EndFrame();
		Present();
	}
}

bool VkContext::BeginFrame()
{
	//continue until the device is idle
	VK_RETURN_IF_FAILED( vkDeviceWaitIdle(m_Device) );

	VK_RETURN_IF_FAILED( vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_AcquireImageSemaphore, VK_NULL_HANDLE, &m_CurrentSwapBuffer) );

	//BeginCommandBuffer(m_SwapBuffers[m_CurrentSwapBuffer].commandBuffer);

	BeginCommandBuffer(m_InitBuffer);
	SetImageLayout(m_InitBuffer, VK_IMAGE_ASPECT_COLOR_BIT,m_SwapBuffers[m_CurrentSwapBuffer].image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,(VkAccessFlagBits)0);
	EndCommandBuffer(m_InitBuffer);
	FlushInitCommand(m_InitBuffer);

	
	return true;
}

bool VkContext::EndFrame()
{

	//EndCommandBuffer(m_SwapBuffers[m_CurrentSwapBuffer].commandBuffer);

	VkPipelineStageFlags _pipeline_staget_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	VkSubmitInfo _submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO};
    _submit_info.waitSemaphoreCount					= 1;
    _submit_info.pWaitSemaphores					= &m_AcquireImageSemaphore;
    _submit_info.pWaitDstStageMask					= &_pipeline_staget_flags;
    _submit_info.commandBufferCount					= 1;
    _submit_info.pCommandBuffers					= &m_SwapBuffers[m_CurrentSwapBuffer].commandBuffer;
    _submit_info.signalSemaphoreCount				= 1;
    _submit_info.pSignalSemaphores					= &m_RenderingDoneSemaphore;

	VK_RETURN_IF_FAILED(vkQueueSubmit(m_Queue, 1, &_submit_info, m_SwapBuffers[m_CurrentSwapBuffer].submitFence));
	VK_RETURN_IF_FAILED(vkWaitForFences(m_Device, 1, &m_SwapBuffers[m_CurrentSwapBuffer].submitFence, VK_TRUE, UINT64_MAX));
	VK_RETURN_IF_FAILED(vkResetFences(m_Device, 1, &m_SwapBuffers[m_CurrentSwapBuffer].submitFence));

	return true;
}

bool VkContext::Present()
{
	VkPresentInfoKHR _present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	_present_info.waitSemaphoreCount				= 1;
	_present_info.pWaitSemaphores					= &m_RenderingDoneSemaphore;
	_present_info.swapchainCount					= 1;
	_present_info.pSwapchains						= &m_SwapChain;
	_present_info.pImageIndices						= &m_CurrentSwapBuffer;
	VK_RETURN_IF_FAILED(vkQueuePresentKHR(m_Queue,&_present_info));
	return true;
}

bool VkContext::DebugDraw(VKSwapBuffer& _swapBuffer)
{
	VkCommandBuffer cmd_buf = _swapBuffer.commandBuffer;

	VkCommandBufferInheritanceInfo cmd_buf_hinfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
	VkCommandBufferBeginInfo cmd_buf_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmd_buf_info.pInheritanceInfo = &cmd_buf_hinfo;

	VkClearValue clear_values[2];
	clear_values[0].color.float32[0] = 0.2f;
	clear_values[0].color.float32[1] = 0.2f;
	clear_values[0].color.float32[2] = 0.2f;
	clear_values[0].color.float32[3] = 0.2f;
	clear_values[1].depthStencil = { 1.0f, 0 }; 

	VkRenderPassBeginInfo rp_begin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	rp_begin.renderPass								= m_RenderPass;
	rp_begin.framebuffer							= _swapBuffer.frameBuffer;
	rp_begin.renderArea.offset.x					= 0;
	rp_begin.renderArea.offset.y 					= 0;
	rp_begin.renderArea.extent.width				= m_SurfaceExtent.width;
	rp_begin.renderArea.extent.height				= m_SurfaceExtent.height;
	rp_begin.clearValueCount						= 2;
	rp_begin.pClearValues							= clear_values;

	VK_RETURN_IF_FAILED( vkBeginCommandBuffer(cmd_buf, &cmd_buf_info) );

	vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, NULL);

	VkViewport viewport = {};
	viewport.height									= (float)m_SurfaceExtent.height;
	viewport.width									= (float)m_SurfaceExtent.width;
	viewport.minDepth								= (float)0.0f;
	viewport.maxDepth								= (float)1.0f;
	vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent.width							= m_SurfaceExtent.width;
	scissor.extent.height							= m_SurfaceExtent.height;
	scissor.offset.x								= 0;
	scissor.offset.y								= 0;
	vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

	vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);
	vkCmdEndRenderPass(cmd_buf);

	VkImageMemoryBarrier prePresentBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	prePresentBarrier.srcAccessMask					= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	prePresentBarrier.dstAccessMask					= VK_ACCESS_MEMORY_READ_BIT;
	prePresentBarrier.oldLayout						= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	prePresentBarrier.newLayout						= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	prePresentBarrier.srcQueueFamilyIndex			= VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.dstQueueFamilyIndex			= VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.subresourceRange				= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	prePresentBarrier.image							= _swapBuffer.image;

	VkImageMemoryBarrier *pmemory_barrier			= &prePresentBarrier;
	vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
		NULL, 1, pmemory_barrier);

	VK_RETURN_IF_FAILED(vkEndCommandBuffer(cmd_buf));
	return true;
}
