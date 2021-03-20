#######################################################################################
#
#  Copyright 2021 OVITO GmbH, Germany
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify it either under the
#  terms of the GNU General Public License version 3 as published by the Free Software
#  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
#  If you do not alter this notice, a recipient may use your version of this
#  file under either the GPL or the MIT License.
#
#  You should have received a copy of the GPL along with this program in a
#  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
#  with this program in a file LICENSE.MIT.txt
#
#  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
#  either express or implied. See the GPL or the MIT License for the specific language
#  governing rights and limitations.
#
#######################################################################################

# Find Vulkan library.
FIND_PACKAGE(Vulkan REQUIRED)

function(add_vulkan_shader target shaderfile)

	# Locate glslc shader compiler.
	# CMake 3.19 and newer define the Vulkan::glslc imported target for the shader compiler.
	IF(TARGET Vulkan::glslc)
		SET(glslc_executable "$<TARGET_FILE:Vulkan::glslc>")
	#ELSE()
	#	find_program(GLSLC glslc)
	ENDIF()

    SET(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/${shaderfile})
    SET(current-output-path ${CMAKE_CURRENT_SOURCE_DIR}/${shaderfile}.spv)

    # Add a custom command to compile GLSL to SPIR-V.
    ADD_CUSTOM_COMMAND(
           OUTPUT "${current-output-path}"
           COMMAND "${glslc_executable}" -o "${current-output-path}" "${current-shader-path}"
           DEPENDS "${current-shader-path}"
           IMPLICIT_DEPENDS CXX "${current-shader-path}"
		   COMMENT "Compiling Vulkan shader ${shaderfile}"
           VERBATIM)

    # Make sure our build depends on this output.
    SET_SOURCE_FILES_PROPERTIES("${current-output-path}" PROPERTIES GENERATED TRUE)
    TARGET_SOURCES(${target} PRIVATE "${current-output-path}")
endfunction()

# Define plugin module.
OVITO_STANDARD_PLUGIN(VulkanRenderer
	SOURCES
		VulkanDevice.cpp
		VulkanSceneRenderer.cpp
		OffscreenVulkanSceneRenderer.cpp
		VulkanLinePrimitive.cpp
		vma/vk_mem_alloc.cpp
		resources/vulkan.qrc
)

# Make sure the Vulkan headers are available.
# Note that linking to the Vulkan library is not necessary, because Qt will load it at runtime.
TARGET_INCLUDE_DIRECTORIES(VulkanRenderer PUBLIC "${Vulkan_INCLUDE_DIRS}")

# Recompile the Vulkan shader source files if the shader compiler is available.
IF(NOT OVITO_DONT_RECOMPILE_VULKAN_SHADERS AND TARGET Vulkan::glslc)
	add_vulkan_shader(VulkanRenderer resources/lines/lines_thin_with_colors.frag)
	add_vulkan_shader(VulkanRenderer resources/lines/lines_thin_with_colors.vert)
	add_vulkan_shader(VulkanRenderer resources/lines/lines_thin_uniform_color.frag)
	add_vulkan_shader(VulkanRenderer resources/lines/lines_thin_uniform_color.vert)
	add_vulkan_shader(VulkanRenderer resources/lines/lines_thin_picking.frag)
	add_vulkan_shader(VulkanRenderer resources/lines/lines_thin_picking.vert)
ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	IF(NOT OVITO_QML_GUI)
		ADD_SUBDIRECTORY(gui)
	ENDIF()
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)