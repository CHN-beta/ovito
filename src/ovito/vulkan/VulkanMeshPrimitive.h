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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/rendering/MeshPrimitive.h>
#include "VulkanContext.h"
#include "VulkanPipeline.h"

namespace Ovito {

class VulkanSceneRenderer;

/**
 * \brief This class is responsible for rendering mesh primitives using Vulkan.
 */
class VulkanMeshPrimitive : public MeshPrimitive, public std::enable_shared_from_this<VulkanMeshPrimitive>
{
public:

	struct Pipelines {
		/// Creates the Vulkan pipelines for this rendering primitive.
		void init(VulkanSceneRenderer* renderer) {}
		/// Destroys the Vulkan pipelines for this rendering primitive.
		void release(VulkanSceneRenderer* renderer);
		/// Initializes a specific pipeline on demand.
		VulkanPipeline& create(VulkanSceneRenderer* renderer, VulkanPipeline& pipeline);

		VulkanPipeline mesh;
		VulkanPipeline mesh_picking;
		VulkanPipeline mesh_wireframe;
		VulkanPipeline mesh_wireframe_instanced;
		VulkanPipeline mesh_instanced;
		VulkanPipeline mesh_instanced_picking;
		VulkanPipeline mesh_instanced_with_colors;
		VulkanPipeline mesh_color_mapping;
		VkDescriptorSetLayout colormap_descriptorSetLayout = VK_NULL_HANDLE;
	};

	/// Sets the mesh to be stored in this buffer object.
	virtual void setMesh(const TriMesh& mesh, DepthSortingMode depthSortingMode) override {
		MeshPrimitive::setMesh(mesh, depthSortingMode);
		_depthSortingMode = depthSortingMode;
		_wireframeLines.reset();
	}

	/// Renders the geometry.
	void render(VulkanSceneRenderer* renderer, Pipelines& pipelines);

private:

	/// Renders the mesh wireframe edges.
	void renderWireframe(VulkanSceneRenderer* renderer, Pipelines& pipelines, const QMatrix4x4& mvp);

	/// Generates the list of wireframe line elements.
	const ConstDataBufferPtr& wireframeLines(VulkanSceneRenderer* renderer);

	/// Prepares the Vulkan buffer with the per-instance transformation matrices.
	VkBuffer getInstanceTMBuffer(VulkanSceneRenderer* renderer);

	/// Stores data of a single vertex passed to the shader.
	struct ColoredVertexWithNormal {
		Point_3<float> position;
		Vector_3<float> normal;
		ColorAT<float> color;
	};

	/// The list of wireframe line elements.
	ConstDataBufferPtr _wireframeLines;	// Array of Point3, two per line element.

	/// Controls how the renderer performs depth-correct rendering of semi-transparent meshes.
	DepthSortingMode _depthSortingMode = AnyShapeMode;
};

}	// End of namespace
