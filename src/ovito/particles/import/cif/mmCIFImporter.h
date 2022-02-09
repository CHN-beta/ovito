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
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito::Particles {

/**
 * \brief File parser for Crystallographic Information Files (CIF).
 */
class OVITO_PARTICLES_EXPORT mmCIFImporter : public ParticleImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public ParticleImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleImporter::OOMetaClass::OOMetaClass;

		/// Returns the list of file formats that can be read by this importer class.
		virtual Ovito::span<const SupportedFormat> supportedFormats() const override {
			static const SupportedFormat formats[] = {{ QStringLiteral("*.cif *.mcif *.cif.gz *.mcif.gz"), tr("PDBx/mmCIF Files") }};
			return formats;
		}

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(mmCIFImporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE mmCIFImporter(ObjectCreationParams params) : ParticleImporter(params) { 
		setGenerateBonds(true); 
	}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("PDBx/mmCIF"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const LoadOperationRequest& request) override {
		activateCLocale();
		return std::make_shared<FrameLoader>(request, recenterCell(), generateBonds());
	}

private:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class FrameLoader : public ParticleImporter::FrameLoader
	{
	public:

		/// Constructor.
		FrameLoader(const LoadOperationRequest& request, bool recenterCell, bool generateBonds) : ParticleImporter::FrameLoader::FrameLoader(request, recenterCell), _generateBonds(generateBonds) {}

	protected:

		/// Reads the frame data from the external file.
		virtual void loadFile() override;

	private:

		/// Controls the generation of ad-hoc bonds during data import.
		bool _generateBonds;
	};
};

}	// End of namespace
