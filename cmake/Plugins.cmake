#######################################################################################
#
#  Copyright 2020 Alexander Stukowski
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

# This macro creates an OVITO plugin module.
MACRO(OVITO_STANDARD_PLUGIN target_name)

    # Parse macro parameters
    SET(options GUI_PLUGIN HAS_NO_EXPORTS)
    SET(oneValueArgs)
    SET(multiValueArgs SOURCES LIB_DEPENDENCIES PRIVATE_LIB_DEPENDENCIES PLUGIN_DEPENDENCIES OPTIONAL_PLUGIN_DEPENDENCIES PRECOMPILED_HEADERS)
    CMAKE_PARSE_ARGUMENTS(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	SET(plugin_sources ${ARG_SOURCES})
	SET(lib_dependencies ${ARG_LIB_DEPENDENCIES})
	SET(private_lib_dependencies ${ARG_PRIVATE_LIB_DEPENDENCIES})
	SET(plugin_dependencies ${ARG_PLUGIN_DEPENDENCIES})
	SET(optional_plugin_dependencies ${ARG_OPTIONAL_PLUGIN_DEPENDENCIES})
	SET(precompiled_headers ${ARG_PRECOMPILED_HEADERS})

	# Create the library target for the plugin.
	IF(BUILD_SHARED_LIBS AND ${ARG_HAS_NO_EXPORTS})
		# Define the library as a module if it doesn't export any symbols.
		ADD_LIBRARY(${target_name} MODULE ${plugin_sources})
	ELSE()
		ADD_LIBRARY(${target_name} ${plugin_sources})
	ENDIF()

    # Set default include directory.
    TARGET_INCLUDE_DIRECTORIES(${target_name} PUBLIC
        "$<BUILD_INTERFACE:${OVITO_SOURCE_BASE_DIR}/src>")

	# Speed up compilation by using precompiled headers.
	IF(OVITO_USE_PRECOMPILED_HEADERS AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
		FOREACH(precompiled_header ${precompiled_headers})
			TARGET_PRECOMPILE_HEADERS(${target_name} PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/${precompiled_header}>")
		ENDFOREACH()
	ENDIF()

	# Speed up compilation by using unity build.
	IF(OVITO_USE_UNITY_BUILD AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES UNITY_BUILD ON)
	ENDIF()

	IF(MSVC)
		# Turn off certain Microsoft compiler warnings.
		TARGET_COMPILE_OPTIONS(${target_name} 
			PRIVATE "/wd4267" # Suppress warning on conversion from size_t to int, possible loss of data.
			PRIVATE "/bigobj" # Compiling template code leads to large object files.
		)
		# Do not warn about use of unsafe CRT Library functions.
		TARGET_COMPILE_DEFINITIONS(${target_name} PRIVATE "_CRT_SECURE_NO_WARNINGS=")
	ENDIF()

	# Make the name of current plugin available to the source code.
	TARGET_COMPILE_DEFINITIONS(${target_name} PRIVATE "OVITO_PLUGIN_NAME=\"${target_name}\"")

	IF(WIN32 AND NOT OVITO_BUILD_MONOLITHIC)
		# Add a suffix to the shared library filename to avoid name clashes with other libraries in the installation directory.
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES OUTPUT_NAME "${target_name}.ovito")
	ENDIF()

	# Link to OVITO's core module (unless it's the core plugin itself we are defining).
	IF(NOT ${target_name} STREQUAL "Core")
		TARGET_LINK_LIBRARIES(${target_name} PUBLIC Core)
	ENDIF()

	# Link to OVITO's desktop GUI module when the plugin provides a GUI.
	IF(${ARG_GUI_PLUGIN})
		IF(OVITO_BUILD_GUI)
			TARGET_LINK_LIBRARIES(${target_name} PUBLIC Gui)
			TARGET_LINK_LIBRARIES(${target_name} PUBLIC Qt5::Widgets)
		ELSEIF(OVITO_BUILD_WEBGUI)
			TARGET_LINK_LIBRARIES(${target_name} PUBLIC GuiWeb)
			TARGET_LINK_LIBRARIES(${target_name} PUBLIC Qt5::Qml Qt5::Quick Qt5::QuickControls2 Qt5::QuickTemplates2)
		ELSE()
			MESSAGE(FATAL_ERROR "Cannot build plugin ${target_name} marked as GUI_PLUGIN if building the GUI has been completely disabled.")
		ENDIF()
	ENDIF()

	# Link to Qt5 libs.
	TARGET_LINK_LIBRARIES(${target_name} PUBLIC Qt5::Core Qt5::Gui)

	# Link to other third-party libraries needed by this specific plugin.
	TARGET_LINK_LIBRARIES(${target_name} PUBLIC ${lib_dependencies})

	# Link to other third-party libraries needed by this specific plugin, which should not be visible to dependent plugins.
	TARGET_LINK_LIBRARIES(${target_name} PRIVATE ${private_lib_dependencies})

	# Link to other plugin modules that are dependencies of this plugin.
	FOREACH(plugin_name ${plugin_dependencies})
    	STRING(TOUPPER "${plugin_name}" uppercase_plugin_name)
    	IF(DEFINED OVITO_BUILD_PLUGIN_${uppercase_plugin_name})
	    	IF(NOT OVITO_BUILD_PLUGIN_${uppercase_plugin_name})
	    		MESSAGE(FATAL_ERROR "To build the ${target_name} plugin, the ${plugin_name} plugin has to be enabled too. Please set the OVITO_BUILD_PLUGIN_${uppercase_plugin_name} option to ON.")
	    	ENDIF()
	    ENDIF()
    	TARGET_LINK_LIBRARIES(${target_name} PUBLIC ${plugin_name})
	ENDFOREACH()

	# Link to other plugin modules that are optional dependencies of this plugin.
	FOREACH(plugin_name ${optional_plugin_dependencies})
		STRING(TOUPPER "${plugin_name}" uppercase_plugin_name)
		IF(OVITO_BUILD_PLUGIN_${uppercase_plugin_name})
        	TARGET_LINK_LIBRARIES(${target_name} PUBLIC ${plugin_name})
		ENDIF()
	ENDFOREACH()

	IF(NOT EMSCRIPTEN)
		# Set prefix and suffix of library name.
		# This is needed so that the Python interpreter can load OVITO plugins as modules.
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES PREFIX "" SUFFIX "${OVITO_PLUGIN_LIBRARY_SUFFIX}")
	ENDIF()

	# Define macro for symbol export from shared library.
	STRING(TOUPPER "${target_name}" _uppercase_plugin_name)
	IF(BUILD_SHARED_LIBS)
		TARGET_COMPILE_DEFINITIONS(${target_name} PRIVATE "OVITO_${_uppercase_plugin_name}_EXPORT=Q_DECL_EXPORT")
		TARGET_COMPILE_DEFINITIONS(${target_name} PRIVATE "OVITO_${_uppercase_plugin_name}_EXPORT_TEMPLATE=")
		TARGET_COMPILE_DEFINITIONS(${target_name} INTERFACE "OVITO_${_uppercase_plugin_name}_EXPORT=Q_DECL_IMPORT")
		TARGET_COMPILE_DEFINITIONS(${target_name} INTERFACE "OVITO_${_uppercase_plugin_name}_EXPORT_TEMPLATE=Q_DECL_IMPORT")
	ELSE()
		TARGET_COMPILE_DEFINITIONS(${target_name} PUBLIC "OVITO_${_uppercase_plugin_name}_EXPORT=")
		TARGET_COMPILE_DEFINITIONS(${target_name} PUBLIC "OVITO_${_uppercase_plugin_name}_EXPORT_TEMPLATE=")
	ENDIF()

	# Set visibility of symbols in this shared library to hidden by default, except those exported in the source code.
	SET_TARGET_PROPERTIES(${target_name} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
	SET_TARGET_PROPERTIES(${target_name} PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

	IF(APPLE)
		# This is required to avoid error by install_name_tool.
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES LINK_FLAGS "-headerpad_max_install_names")
	ENDIF(APPLE)

	IF(NOT OVITO_BUILD_PYTHON_PACKAGE)
		IF(APPLE)
			IF(NOT OVITO_BUILD_CONDA)
				SET_TARGET_PROPERTIES(${target_name} PROPERTIES INSTALL_RPATH "@loader_path/;@executable_path/;@loader_path/../MacOS/;@executable_path/../Frameworks/")
			ELSE()
				# Look for other shared libraries in the parent directory ("lib/ovito/") and in the plugins directory ("lib/ovito/plugins/")
				SET_TARGET_PROPERTIES(${target_name} PROPERTIES INSTALL_RPATH "@loader_path/;@loader_path/../")
			ENDIF()
			# The build tree target should have rpath of install tree target.
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
		ELSEIF(UNIX)
			# Look for other shared libraries in the parent directory ("lib/ovito/") and in the plugins directory ("lib/ovito/plugins/")
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES INSTALL_RPATH "$ORIGIN:$ORIGIN/..")
		ENDIF()
	ELSE()
		IF(APPLE)
			# Use @loader_path on macOS when building the Python modules only.
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES INSTALL_RPATH "@loader_path/")
		ELSEIF(UNIX)
			# Look for other shared libraries in the same directory.
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES INSTALL_RPATH "$ORIGIN")
		ENDIF()

		IF(NOT BUILD_SHARED_LIBS)
			# Since we will link this library into the dynamically loaded Python extension module, we need to use the fPIC flag.
			SET_PROPERTY(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)
		ENDIF()
	ENDIF()

	# Make this module part of the installation package.
	IF(WIN32 AND (${target_name} STREQUAL "Core" OR ${target_name} STREQUAL "Gui" OR ${target_name} STREQUAL "GuiBase"))
		# On Windows, the Core and Gui DLLs need to be placed in the same directory
		# as the Ovito executable, because Windows won't find them if they are in the
		# plugins subdirectory.
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${OVITO_BINARY_DIRECTORY}")
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_BINARY_DIRECTORY}")
		INSTALL(TARGETS ${target_name} EXPORT OVITO
			RUNTIME DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}"
			LIBRARY DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}"
			ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
	ELSE()
		# Install all plugins into the plugins directory.
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${OVITO_PLUGINS_DIRECTORY}")
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_PLUGINS_DIRECTORY}")
		INSTALL(TARGETS ${target_name} EXPORT OVITO
			RUNTIME DESTINATION "${OVITO_RELATIVE_PLUGINS_DIRECTORY}"
			LIBRARY DESTINATION "${OVITO_RELATIVE_PLUGINS_DIRECTORY}"
			ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
	ENDIF()

	# Maintain the list of all plugins.
	LIST(APPEND OVITO_PLUGIN_LIST ${target_name})
	SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)

ENDMACRO()
