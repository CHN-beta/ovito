////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/particles/import/ParticleImporter.h>

namespace Ovito { namespace CrystalAnalysis {

/**
 * \brief Importer for data files written by the ParaDiS discrete dislocation simulation code.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ParaDiSImporter : public ParticleImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public ParticleImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleImporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("ParaDiS Data Files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(ParaDiSImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ParaDiSImporter(DataSet* dataset) : ParticleImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("ParaDiS File"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const Frame& frame, const FileHandle& file, const DataCollection* masterCollection, PipelineObject* dataSource) override {
		return std::make_shared<FrameLoader>(dataset(), frame, file, masterCollection, dataSource);
	}

protected:

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

		/// Returns the type of crystal structure.
		ParticleType::PredefinedStructureType latticeStructure() const { return _latticeStructure; }

		/// Sets the type of crystal ("fcc", "bcc", etc.)
		void setLatticeStructure(ParticleType::PredefinedStructureType latticeStructure) {
			_latticeStructure = latticeStructure;
		}

        /// Parses a control parameter from the ParaDiS file.
        static std::pair<QString, QVariant> parseControlParameter(CompressedTextReader& stream);

		/// The type of crystal ("fcc", "bcc", etc.)
		ParticleType::PredefinedStructureType _latticeStructure = ParticleType::PredefinedStructureType::OTHER;
	};
};

}	// End of namespace
}	// End of namespace
