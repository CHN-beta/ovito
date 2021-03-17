////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 OVITO GmbH, Germany
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/core/Core.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/rendering/RenderSettings.h>
#include "VulkanSceneRenderer.h"

#include <QVulkanDeviceFunctions>

namespace Ovito {
    
// Note that the vertex data and the projection matrix assume OpenGL. With
// Vulkan Y is negated in clip space and the near/far plane is at 0/1 instead
// of -1/1. These will be corrected for by an extra transformation when
// calculating the modelview-projection matrix.
static float vertexData[] = { // Y up, front = CCW
     0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
    -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f
};

static const int UNIFORM_DATA_SIZE = 16 * sizeof(float);


IMPLEMENT_OVITO_CLASS(VulkanSceneRenderer);

/******************************************************************************
* Is called by OVITO to query the class for any information that should be 
* included in the application's system report.
******************************************************************************/
void VulkanSceneRenderer::OOMetaClass::querySystemInformation(QTextStream& stream) const
{
	if(this == &VulkanSceneRenderer::OOClass()) {
		stream << "======== Vulkan info =======" << "\n";
	}
}

/******************************************************************************
* Constructor.
******************************************************************************/
VulkanSceneRenderer::VulkanSceneRenderer(DataSet* dataset, std::shared_ptr<VulkanDevice> vulkanDevice, int concurrentFrameCount) 
    : SceneRenderer(dataset), 
    _device(std::move(vulkanDevice)),
    _concurrentFrameCount(concurrentFrameCount)
{
	OVITO_ASSERT(_device);
    OVITO_ASSERT(_concurrentFrameCount >= 1);

    // Release our own Vulkan resources right before the logical device is destroyed.
    connect(_device.get(), &VulkanDevice::releaseResourcesRequested, this, &VulkanSceneRenderer::releaseResources);
}

/******************************************************************************
* Determines if this renderer can share geometry data and other resources with
* the given other renderer.
******************************************************************************/
bool VulkanSceneRenderer::sharesResourcesWith(SceneRenderer* otherRenderer) const
{
	// Two Vulkan renderers are always compatible.
	if(VulkanSceneRenderer* otherGLRenderer = dynamic_object_cast<VulkanSceneRenderer>(otherRenderer)) {
		return true;
	}
	return false;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void VulkanSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	SceneRenderer::beginFrame(time, params, vp);

	// This method may only be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == device()->thread());

    // Make sure our Vulkan objects have been created.
    initResources();

    // Specify Vulkan viewport area.
    VkViewport viewport;
    viewport.x = viewport.y = 0;
    viewport.width = frameBufferSize().width();
    viewport.height = frameBufferSize().height();
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    deviceFunctions()->vkCmdSetViewport(currentCommandBuffer(), 0, 1, &viewport);

    // Specify Vulkan scissor rectangle.
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    deviceFunctions()->vkCmdSetScissor(currentCommandBuffer(), 0, 1, &scissor);

    // Full view-projection matrix including OpenGL -> Vulkan convention correction.
    QMatrix4x4 proj = _clipCorrection * projParams().projectionMatrix * projParams().viewMatrix;

    quint8* p;
    VkResult err = deviceFunctions()->vkMapMemory(logicalDevice(), m_bufMem, m_uniformBufInfo[currentSwapChainFrame()].offset, UNIFORM_DATA_SIZE, 0, reinterpret_cast<void **>(&p));
    if(err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);
    memcpy(p, proj.constData(), 16 * sizeof(float));
    deviceFunctions()->vkUnmapMemory(logicalDevice(), m_bufMem);

    deviceFunctions()->vkCmdBindPipeline(currentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    deviceFunctions()->vkCmdBindDescriptorSets(currentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descSet[currentSwapChainFrame()], 0, nullptr);
    VkDeviceSize vbOffset = 0;
    deviceFunctions()->vkCmdBindVertexBuffers(currentCommandBuffer(), 0, 1, &m_buf, &vbOffset);
    deviceFunctions()->vkCmdDraw(currentCommandBuffer(), 3, 1, 0, 0);
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void VulkanSceneRenderer::endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer)
{
    if(!isInteractive()) {
        releaseResources();
    }

	SceneRenderer::endFrame(renderingSuccessful, frameBuffer);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool VulkanSceneRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation)
{
	return !operation.isCanceled();
}

/******************************************************************************
* Renders a 2d polyline in the viewport.
******************************************************************************/
void VulkanSceneRenderer::render2DPolyline(const Point2* points, int count, const ColorA& color, bool closed)
{
	if(isBoundingBoxPass())
		return;
}

/******************************************************************************
* Sets the frame buffer background color.
******************************************************************************/
void VulkanSceneRenderer::setClearColor(const ColorA& color)
{
}

/******************************************************************************
* Sets the rendering region in the frame buffer.
******************************************************************************/
void VulkanSceneRenderer::setRenderingViewport(int x, int y, int width, int height)
{
}

/******************************************************************************
* Clears the frame buffer contents.
******************************************************************************/
void VulkanSceneRenderer::clearFrameBuffer(bool clearDepthBuffer, bool clearStencilBuffer)
{
}

/******************************************************************************
* Temporarily enables/disables the depth test while rendering.
******************************************************************************/
void VulkanSceneRenderer::setDepthTestEnabled(bool enabled)
{
}

/******************************************************************************
* Activates the special highlight rendering mode.
******************************************************************************/
void VulkanSceneRenderer::setHighlightMode(int pass)
{
}

void VulkanSceneRenderer::initResources()
{
    if(m_bufMem != VK_NULL_HANDLE)
        return;

    // Prepare the vertex and uniform data. The vertex data will never
    // change so one buffer is sufficient regardless of the value of
    // CONCURRENT_FRAME_COUNT. Uniform data is changing per
    // frame however so active frames have to have a dedicated copy.

    // Use just one memory allocation and one buffer. We will then specify the
    // appropriate offsets for uniform buffers in the VkDescriptorBufferInfo.
    // Have to watch out for
    // VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment, though.

    // The uniform buffer is not strictly required in this example, we could
    // have used push constants as well since our single matrix (64 bytes) fits
    // into the spec mandated minimum limit of 128 bytes. However, once that
    // limit is not sufficient, the per-frame buffers, as shown below, will
    // become necessary.

    const int concurrentFrameCount = this->concurrentFrameCount();
    const VkPhysicalDeviceLimits* pdevLimits = &device()->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    qDebug("uniform buffer offset alignment is %u", (uint) uniAlign);
    VkBufferCreateInfo bufInfo;
    memset(&bufInfo, 0, sizeof(bufInfo));
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // Our internal layout is vertex, uniform, uniform, ... with each uniform buffer start offset aligned to uniAlign.
    const VkDeviceSize vertexAllocSize = VulkanDevice::aligned(sizeof(vertexData), uniAlign);
    const VkDeviceSize uniformAllocSize = VulkanDevice::aligned(UNIFORM_DATA_SIZE, uniAlign);
    bufInfo.size = vertexAllocSize + concurrentFrameCount * uniformAllocSize;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkResult err = deviceFunctions()->vkCreateBuffer(logicalDevice(), &bufInfo, nullptr, &m_buf);
    if(err != VK_SUCCESS)
        qFatal("Failed to create buffer: %d", err);

    VkMemoryRequirements memReq;
    deviceFunctions()->vkGetBufferMemoryRequirements(logicalDevice(), m_buf, &memReq);

    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        device()->hostVisibleMemoryIndex()
    };

    err = deviceFunctions()->vkAllocateMemory(logicalDevice(), &memAllocInfo, nullptr, &m_bufMem);
    if(err != VK_SUCCESS)
        qFatal("Failed to allocate memory: %d", err);

    err = deviceFunctions()->vkBindBufferMemory(logicalDevice(), m_buf, m_bufMem, 0);
    if(err != VK_SUCCESS)
        qFatal("Failed to bind buffer memory: %d", err);

    quint8 *p;
    err = deviceFunctions()->vkMapMemory(logicalDevice(), m_bufMem, 0, memReq.size, 0, reinterpret_cast<void **>(&p));
    if(err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);
    memcpy(p, vertexData, sizeof(vertexData));
    QMatrix4x4 ident;
    memset(m_uniformBufInfo, 0, sizeof(m_uniformBufInfo));
    for(int i = 0; i < concurrentFrameCount; ++i) {
        const VkDeviceSize offset = vertexAllocSize + i * uniformAllocSize;
        memcpy(p + offset, ident.constData(), 16 * sizeof(float));
        m_uniformBufInfo[i].buffer = m_buf;
        m_uniformBufInfo[i].offset = offset;
        m_uniformBufInfo[i].range = uniformAllocSize;
    }
    deviceFunctions()->vkUnmapMemory(logicalDevice(), m_bufMem);

    VkVertexInputBindingDescription vertexBindingDesc = {
        0, // binding
        5 * sizeof(float),
        VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position
            0, // location
            0, // binding
            VK_FORMAT_R32G32_SFLOAT,
            0
        },
        { // color
            1,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            2 * sizeof(float)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;

    // Set up descriptor set and its layout.
    VkDescriptorPoolSize descPoolSizes = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(concurrentFrameCount) };
    VkDescriptorPoolCreateInfo descPoolInfo;
    memset(&descPoolInfo, 0, sizeof(descPoolInfo));
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolInfo.maxSets = concurrentFrameCount;
    descPoolInfo.poolSizeCount = 1;
    descPoolInfo.pPoolSizes = &descPoolSizes;
    err = deviceFunctions()->vkCreateDescriptorPool(logicalDevice(), &descPoolInfo, nullptr, &m_descPool);
    if(err != VK_SUCCESS)
        qFatal("Failed to create descriptor pool: %d", err);

    VkDescriptorSetLayoutBinding layoutBinding = {
        0, // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        1,
        VK_SHADER_STAGE_VERTEX_BIT,
        nullptr
    };
    VkDescriptorSetLayoutCreateInfo descLayoutInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        1,
        &layoutBinding
    };
    err = deviceFunctions()->vkCreateDescriptorSetLayout(logicalDevice(), &descLayoutInfo, nullptr, &m_descSetLayout);
    if(err != VK_SUCCESS)
        qFatal("Failed to create descriptor set layout: %d", err);

    for (int i = 0; i < concurrentFrameCount; ++i) {
        VkDescriptorSetAllocateInfo descSetAllocInfo = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            nullptr,
            m_descPool,
            1,
            &m_descSetLayout
        };
        err = deviceFunctions()->vkAllocateDescriptorSets(logicalDevice(), &descSetAllocInfo, &m_descSet[i]);
        if(err != VK_SUCCESS)
            qFatal("Failed to allocate descriptor set: %d", err);

        VkWriteDescriptorSet descWrite;
        memset(&descWrite, 0, sizeof(descWrite));
        descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descWrite.dstSet = m_descSet[i];
        descWrite.descriptorCount = 1;
        descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descWrite.pBufferInfo = &m_uniformBufInfo[i];
        deviceFunctions()->vkUpdateDescriptorSets(logicalDevice(), 1, &descWrite, 0, nullptr);
    }

    // Pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    err = deviceFunctions()->vkCreatePipelineCache(logicalDevice(), &pipelineCacheInfo, nullptr, &m_pipelineCache);
    if(err != VK_SUCCESS)
        qFatal("Failed to create pipeline cache: %d", err);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descSetLayout;
    err = deviceFunctions()->vkCreatePipelineLayout(logicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if(err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", err);

    // Shaders
    VkShaderModule vertShaderModule = _device->createShader(QStringLiteral(":/vulkanrenderer/color.vert.spv"));
    VkShaderModule fragShaderModule = _device->createShader(QStringLiteral(":/vulkanrenderer/color.frag.spv"));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            vertShaderModule,
            "main",
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            fragShaderModule,
            "main",
            nullptr
        }
    };
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo ia;
    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.pInputAssemblyState = &ia;

    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo vp;
    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    pipelineInfo.pViewportState = &vp;

    VkPipelineRasterizationStateCreateInfo rs;
    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE; // we want the back face as well
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms;
    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enable multisampling if requested.
    ms.rasterizationSamples = _sampleCount;
    pipelineInfo.pMultisampleState = &ms;

    VkPipelineDepthStencilStateCreateInfo ds;
    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineInfo.pDepthStencilState = &ds;

    VkPipelineColorBlendStateCreateInfo cb;
    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // no blend, write out all of rgba
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
    cb.attachmentCount = 1;
    cb.pAttachments = &att;
    pipelineInfo.pColorBlendState = &cb;

    VkDynamicState dynEnable[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn;
    memset(&dyn, 0, sizeof(dyn));
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
    dyn.pDynamicStates = dynEnable;
    pipelineInfo.pDynamicState = &dyn;

    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = _defaultRenderPass;

    err = deviceFunctions()->vkCreateGraphicsPipelines(logicalDevice(), m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline);
    if(err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", err);

    if(vertShaderModule)
        deviceFunctions()->vkDestroyShaderModule(logicalDevice(), vertShaderModule, nullptr);
    if(fragShaderModule)
        deviceFunctions()->vkDestroyShaderModule(logicalDevice(), fragShaderModule, nullptr);
}

void VulkanSceneRenderer::releaseResources()
{
	// This method may only be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == device()->thread());

	if(!deviceFunctions())
		return;

    if(m_pipeline) {
        deviceFunctions()->vkDestroyPipeline(logicalDevice(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if(m_pipelineLayout) {
        deviceFunctions()->vkDestroyPipelineLayout(logicalDevice(), m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if(m_pipelineCache) {
        deviceFunctions()->vkDestroyPipelineCache(logicalDevice(), m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    if (m_descSetLayout) {
        deviceFunctions()->vkDestroyDescriptorSetLayout(logicalDevice(), m_descSetLayout, nullptr);
        m_descSetLayout = VK_NULL_HANDLE;
    }

    if (m_descPool) {
        deviceFunctions()->vkDestroyDescriptorPool(logicalDevice(), m_descPool, nullptr);
        m_descPool = VK_NULL_HANDLE;
    }

    if (m_buf) {
        deviceFunctions()->vkDestroyBuffer(logicalDevice(), m_buf, nullptr);
        m_buf = VK_NULL_HANDLE;
    }

    if (m_bufMem) {
        deviceFunctions()->vkFreeMemory(logicalDevice(), m_bufMem, nullptr);
        m_bufMem = VK_NULL_HANDLE;
    }
}

}	// End of namespace
