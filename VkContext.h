#pragma once
#include "Predefined.h"
#include <vulkan/vulkan.h>
#include <vector>

class VkContext
{
private:
	VkContext();
	virtual ~VkContext();
public:
	static VkContext*				GetInstance();
	bool							Initialize(HINSTANCE _hinstance, HWND _hwnd);
	void							Destroy();

	void							Run();
	bool							BeginFrame();
	bool							EndFrame();
	bool							Present();
private:

	struct VKSwapBuffer {
		VkImage				image;
		VkImageView			imageView;
		VkFramebuffer		frameBuffer;
		VkCommandBuffer		commandBuffer;
		VkFence				submitFence;
	};
	struct VKRenderBuffer {
		VkFormat				format;
		VkImage					image;
		VkImageView				imageView;
		VkDeviceMemory			memory;
	};
	struct VKTextureObject {
		VkSampler				sampler;
		VkImage					image;
		VkImageLayout			imageLayout;
		VkDeviceMemory			memory;
		VkImageView				imageView;
		int32_t					width;
		int32_t					height;
	};
	struct VKUniformBufferObject {
		VkBuffer				buffer;
		VkDeviceMemory			memory;
		VkDescriptorBufferInfo  buffer_info;
	};

	bool							InitializeInstance();
	bool							InitializeDevice();
	bool							InitializeInstanceLayerAndExt();
	bool							InitializeDeviceLayerAndExt();
	bool							InitializeDebugLayer();
	bool							InitializeSwapChain();
	bool							InitializeSwapBuffers();
	bool							InitializeSurfaces();
	bool							InitializeCmdBuffers();
	bool							InitializeFrameSync();
	bool							InitializeDepthBuffer();
	bool							InitializeFrameBuffers();
	bool							InitializeRenderPass();
	bool							InitializeDescriptorPool();
	bool							InitializeDescriptorLayout();
	bool							InitializeDescriptorSet();
	bool							InitializePipeline();

	bool							LoadTextures();
	bool							LoadMeshData();
	bool							loadShader();

	bool							AllocVideoMemory( const VkMemoryRequirements & _memory_requirement, VkFlags required_mask, VkDeviceMemory* _device_memory);

	bool							FlushInitCommand( VkCommandBuffer _cmd );

	bool							BeginCommandBuffer( VkCommandBuffer _cmd );
	bool							EndCommandBuffer( VkCommandBuffer _cmd );
	void							SetImageLayout( VkCommandBuffer _cmd, VkImageAspectFlags  _aspectMask, VkImage _image, VkImageLayout _old_layout, VkImageLayout _new_layout, VkAccessFlagBits _srcAccessFlags);

	bool							DebugDraw( VKSwapBuffer& _swapBufer);

	std::vector<char*>				m_InstanceLayerNames;
	std::vector<char*>				m_InstanceExtensionNames;
	std::vector<char*>				m_DeviceLayerNames;
	std::vector<char*>				m_DeviceExtensionNames;

	HINSTANCE						m_hinstance;
	HWND							m_hwnd;
	VkInstance						m_VkInstance;
	VkPhysicalDevice				m_GPU;
	VkDevice						m_Device;
	VkQueue							m_Queue;
	VkSurfaceKHR					m_Surface;
	VkSwapchainKHR					m_SwapChain;
	uint32_t						m_SwapChainImageCount;
	uint32_t						m_QueueFamilyIndex;
	VkCommandPool					m_CommandPool;
	VkFormat						m_SurfaceFormat;
	VkExtent2D						m_SurfaceExtent;
	std::vector<VKSwapBuffer>		m_SwapBuffers;

	uint32_t						m_CurrentSwapBuffer;
	VkSemaphore						m_AcquireImageSemaphore;
	VkSemaphore						m_RenderingDoneSemaphore;	
	VkCommandBuffer					m_InitBuffer;
	VkPhysicalDeviceMemoryProperties	m_MemoryProperties;
	VKRenderBuffer					m_DepthBuffer;
	VkDescriptorPool				m_DescriptorPool;
	VkRenderPass					m_RenderPass;

	VKTextureObject					m_TextureObject;
	VKUniformBufferObject			m_UniformBufferObject;
	VkDescriptorSet					m_DescriptorSet;
	VkDescriptorSetLayout			m_DescriptorSetLayout;
	VkPipelineLayout				m_PipelineLayout;
	VkShaderModule					m_VertexShaderModule;
	VkShaderModule					m_PixelShaderModule;
	VkPipelineCache					m_PipelineCache;
	VkPipeline						m_Pipeline;


};

