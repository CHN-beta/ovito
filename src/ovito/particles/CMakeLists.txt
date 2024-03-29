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

# Find the required Qt modules.
FIND_PACKAGE(${OVITO_QT_MAJOR_VERSION} ${OVITO_MINIMUM_REQUIRED_QT_VERSION} COMPONENTS Xml REQUIRED)

SET(SourceFiles
	objects/ParticlesVis.cpp
	objects/ParticleType.cpp
	objects/ParticlesObject.cpp
	objects/BondType.cpp
	objects/ParticleBondMap.cpp
	objects/BondsObject.cpp
	objects/BondsVis.cpp
	objects/AnglesObject.cpp
	objects/DihedralsObject.cpp
	objects/ImpropersObject.cpp
	objects/TrajectoryObject.cpp
	objects/TrajectoryVis.cpp
	objects/VectorVis.cpp
	modifier/coloring/AmbientOcclusionModifier.cpp
	modifier/modify/WrapPeriodicImagesModifier.cpp
	modifier/modify/UnwrapTrajectoriesModifier.cpp
	modifier/modify/CreateBondsModifier.cpp
	modifier/modify/LoadTrajectoryModifier.cpp
	modifier/modify/CoordinationPolyhedraModifier.cpp
	modifier/analysis/StructureIdentificationModifier.cpp
	modifier/analysis/ReferenceConfigurationModifier.cpp
	modifier/analysis/ackland_jones/AcklandJonesModifier.cpp
	modifier/analysis/cna/CommonNeighborAnalysisModifier.cpp
	modifier/analysis/coordination/CoordinationAnalysisModifier.cpp
	modifier/analysis/chill_plus/ChillPlusModifier.cpp
	modifier/analysis/displacements/CalculateDisplacementsModifier.cpp
	modifier/analysis/centrosymmetry/CentroSymmetryModifier.cpp
	modifier/analysis/strain/AtomicStrainModifier.cpp
	modifier/analysis/diamond/IdentifyDiamondModifier.cpp
	modifier/analysis/voronoi/VoronoiAnalysisModifier.cpp
	modifier/analysis/cluster/ClusterAnalysisModifier.cpp
	modifier/analysis/wignerseitz/WignerSeitzAnalysisModifier.cpp
	modifier/analysis/ptm/PolyhedralTemplateMatchingModifier.cpp
	modifier/analysis/ptm/PTMAlgorithm.cpp
	modifier/analysis/surface/ConstructSurfaceModifier.cpp
	modifier/selection/ExpandSelectionModifier.cpp
	modifier/properties/SmoothTrajectoryModifier.cpp
	modifier/properties/GenerateTrajectoryLinesModifier.cpp
	import/ParticleImporter.cpp
	import/lammps/LAMMPSTextDumpImporter.cpp
	import/lammps/LAMMPSBinaryDumpImporter.cpp
	import/lammps/LAMMPSDataImporter.cpp
	import/lammps/LAMMPSDumpLocalImporter.cpp
	import/lammps/ReaxFFBondImporter.cpp
	import/vasp/POSCARImporter.cpp
	import/xyz/XYZImporter.cpp
	import/imd/IMDImporter.cpp
	import/parcas/ParcasFileImporter.cpp
	import/cfg/CFGImporter.cpp
	import/pdb/PDBImporter.cpp
	import/fhi_aims/FHIAimsImporter.cpp
	import/fhi_aims/FHIAimsLogFileImporter.cpp
	import/gsd/GSDImporter.cpp
	import/gsd/gsd.c
	import/castep/CastepCellImporter.cpp
	import/castep/CastepMDImporter.cpp
	import/cube/GaussianCubeImporter.cpp
	import/xsf/XSFImporter.cpp
	import/dl_poly/DLPOLYImporter.cpp
	import/quantumespresso/QuantumEspressoImporter.cpp
	import/cif/CIFImporter.cpp
	import/cif/mmCIFImporter.cpp
	import/gromacs/GroImporter.cpp
	import/gromacs/XTCImporter.cpp
	import/vtk/ParaViewVTPParticleImporter.cpp
	import/vtk/ParaViewVTPBondsImporter.cpp
	export/ParticleExporter.cpp
	export/FileColumnParticleExporter.cpp
	export/vasp/POSCARExporter.cpp
	export/fhi_aims/FHIAimsExporter.cpp
	export/lammps/LAMMPSDataExporter.cpp
	export/lammps/LAMMPSDumpExporter.cpp
	export/xyz/XYZExporter.cpp
	export/imd/IMDExporter.cpp
	export/gsd/GSDExporter.cpp
	util/NearestNeighborFinder.cpp
	util/CutoffNeighborFinder.cpp
	util/PTMNeighborFinder.cpp
	util/ParticleExpressionEvaluator.cpp
)

IF(OVITO_BUILD_PLUGIN_STDMOD)
	LIST(APPEND SourceFiles
		modifier/coloring/ParticlesColorCodingModifierDelegate.cpp
		modifier/coloring/TrajectoryColorCodingModifierDelegate.cpp
		modifier/coloring/ParticlesAssignColorModifierDelegate.cpp
		modifier/modify/ParticlesSliceModifierDelegate.cpp
		modifier/modify/ParticlesAffineTransformationModifierDelegate.cpp
		modifier/modify/ParticlesDeleteSelectedModifierDelegate.cpp
		modifier/modify/ParticlesReplicateModifierDelegate.cpp
		modifier/properties/ParticlesComputePropertyModifierDelegate.cpp
		modifier/properties/BondsComputePropertyModifierDelegate.cpp
		modifier/selection/ParticlesExpressionSelectionModifierDelegate.cpp
		modifier/modify/ParticlesCombineDatasetsModifierDelegate.cpp
	)
ENDIF()

IF(NOT EMSCRIPTEN)
	LIST(APPEND SourceFiles
		modifier/properties/InteractiveMolecularDynamicsModifier.cpp
	)
ENDIF()

# Define the plugin module.
OVITO_STANDARD_PLUGIN(Particles
	SOURCES ${SourceFiles}
	PLUGIN_DEPENDENCIES Mesh Grid StdObj Delaunay
	PRIVATE_LIB_DEPENDENCIES VoroPlusPlus PolyhedralTemplateMatching MinimumWeightMatchingCentrosymmetry xdrfile
	OPTIONAL_PLUGIN_DEPENDENCIES StdMod
	PRECOMPILED_HEADERS Particles.h
	LIB_DEPENDENCIES ${OVITO_QT_MAJOR_VERSION}::Xml
)

# Build corresponding GUI plugin.
IF(OVITO_BUILD_APP)
	IF(NOT OVITO_QML_GUI)
		ADD_SUBDIRECTORY(gui)
	ELSE()
		ADD_SUBDIRECTORY(guiqml)
	ENDIF()
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
