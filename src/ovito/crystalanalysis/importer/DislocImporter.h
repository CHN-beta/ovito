////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/particles/import/ParticleImporter.h>

namespace Ovito::CrystalAnalysis {

/**
 * \brief Reader for output files generated by the LAMMPS "fix disloc" module.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocImporter : public ParticleImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public ParticleImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleImporter::OOMetaClass::OOMetaClass;

		/// Returns the list of file formats that can be read by this importer class.
		virtual Ovito::span<const SupportedFormat> supportedFormats() const override {
			static const SupportedFormat formats[] = {{ QStringLiteral("*"), tr("Fix Disloc Files") }};
			return formats;
		}

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(DislocImporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE DislocImporter(ObjectCreationParams params) : ParticleImporter(params) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("Disloc"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const LoadOperationRequest& request) override {
		return std::make_shared<FrameLoader>(request);
	}

protected:

	/// This method is called when the pipeline node for the FileSource is created.
	virtual void setupPipeline(PipelineSceneNode* pipeline, FileSource* importObj) override;

	/// The format-specific task object that is responsible for reading an input file in a worker thread.
	class FrameLoader : public ParticleImporter::FrameLoader
	{
	public:

		/// Inherit constructor from base class.
		using ParticleImporter::FrameLoader::FrameLoader;

	protected:

		/// Reads the frame data from the external file.
		virtual void loadFile() override;

	private:

		/// Connects the slip faces to form two-dimensional manifolds.
		static void connectSlipFaces(MicrostructureAccess& microstructure, const std::vector<std::pair<qlonglong,qlonglong>>& slipSurfaceMap);

		/// Sets the type of crystal ("fcc", "bcc", etc.)
		void setLatticeStructure(ParticleType::PredefinedStructureType latticeStructure, const Matrix3& latticeOrientation) {
			_latticeStructure = latticeStructure;
			_latticeOrientation = latticeOrientation;
		}

		/// Returns the type of crystal structure.
		ParticleType::PredefinedStructureType latticeStructure() const { return _latticeStructure; }

		/// The type of crystal ("fcc", "bcc", etc.)
		ParticleType::PredefinedStructureType _latticeStructure;

		/// The lattice orientation matrix.
		Matrix3 _latticeOrientation;
	};
};

}	// End of namespace
