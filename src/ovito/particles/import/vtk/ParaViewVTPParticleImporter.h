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


#include <ovito/particles/Particles.h>
#include <ovito/particles/import/ParticleImporter.h>
#include <ovito/mesh/io/ParaViewVTMImporter.h>

#include <QXmlStreamReader>

namespace Ovito { namespace Particles {

/**
 * \brief File reader for point-like particle data from a ParaView VTP (PolyData) file as written by the Aspherix simulation code.
 */
class OVITO_PARTICLES_EXPORT ParaViewVTPParticleImporter : public ParticleImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public ParticleImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleImporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.vtp"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("Aspherix VTP Particle File"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(ParaViewVTPParticleImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructor.
	Q_INVOKABLE ParaViewVTPParticleImporter(DataSet *dataset) : ParticleImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("VTP"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const LoadOperationRequest& request) override {
		return std::make_shared<FrameLoader>(request, std::move(_particleShapeFiles));
	}

	/// Stores the list of particle type names and corresponding shape file URLs to be loaded.
	void setParticleShapeFileList(std::vector<std::pair<QString, QUrl>> particleShapeFiles) {
		_particleShapeFiles = std::move(particleShapeFiles);
	}

private:

	/// The format-specific task object that is responsible for reading an input file in a separate thread.
	class FrameLoader : public ParticleImporter::FrameLoader
	{
	public:

		/// Constructor.
		FrameLoader(const LoadOperationRequest& request, std::vector<std::pair<QString, QUrl>> particleShapeFiles) 
			: ParticleImporter::FrameLoader(request), _particleShapeFiles(std::move(particleShapeFiles)) {}

	protected:

		/// Reads the frame data from the external file.
		virtual void loadFile() override;

		/// Creates the right kind of OVITO property object that will receive the data read from a <DataArray> element.
		PropertyObject* createParticlePropertyForDataArray(QXmlStreamReader& xml, int& vectorComponent, bool preserveExistingData);

		/// Helper method that loads the shape of a particle type from an external geometry file.
		void loadParticleShape(ParticleType* particleType);

		/// The list of particle type names and corresponding files containing the particle shapes.
		/// This list is extracted by the ParticlesParaViewVTMFileFilter class from the VTM multi-block structure.
		std::vector<std::pair<QString, QUrl>> _particleShapeFiles;
	};

	/// The list of particle type names and corresponding files containing the particle shapes.
	/// This list is extracted by the ParticlesParaViewVTMFileFilter class from the VTM multi-block structure.
	std::vector<std::pair<QString, QUrl>> _particleShapeFiles;
};

/**
 * \brief Plugin filter used to customize the loading of VTM files referencing a ParaView VTP file.
 *        This filter is needed to correctly load VTM/VTP file combinations written by the Aspherix simulation code.
 */
class ParticlesParaViewVTMFileFilter : public ParaViewVTMFileFilter
{
	Q_OBJECT
	OVITO_CLASS(ParticlesParaViewVTMFileFilter)

public:

	/// Constructor.
	Q_INVOKABLE ParticlesParaViewVTMFileFilter() = default;

	/// \brief Is called once before the datasets referenced in a multi-block VTM file will be loaded.
	virtual void preprocessDatasets(std::vector<ParaViewVTMBlockInfo>& blockDatasets, FileSourceImporter::LoadOperationRequest& request, const ParaViewVTMImporter& vtmImporter) override;

	/// Is called before parsing of a dataset reference in a multi-block VTM file begins.
	virtual void configureImporter(const ParaViewVTMBlockInfo& blockInfo, FileSourceImporter::LoadOperationRequest& loadRequest, FileSourceImporter* importer) override;

private:

	/// The list of shape files for particle types. 
	std::vector<std::pair<QString, QUrl>> _particleShapeFiles;
};

}	// End of namespace
}	// End of namespace
