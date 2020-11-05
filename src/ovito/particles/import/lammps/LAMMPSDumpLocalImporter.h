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


#include <ovito/particles/Particles.h>
#include <ovito/particles/import/ParticleImporter.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito { namespace Particles {

/**
 * \brief File parser for LAMMPS dump local files, which contain per-bond information.
 */
class OVITO_PARTICLES_EXPORT LAMMPSDumpLocalImporter : public ParticleImporter
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
		virtual QString fileFilterDescription() const override { return tr("LAMMPS Dump Local Files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(LAMMPSDumpLocalImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE LAMMPSDumpLocalImporter(DataSet* dataset) : ParticleImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("LAMMPS Dump Local"); }

	/// Indicates whether this file importer type loads particle trajectories.
	virtual bool isTrajectoryFormat() const override { return true; } 

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const Frame& frame, const FileHandle& file, const DataCollection* masterCollection, PipelineObject* dataSource) override {
		activateCLocale();
		return std::make_shared<FrameLoader>(dataset(), frame, file, masterCollection, dataSource, columnMapping());
	}

	/// Creates an asynchronous frame discovery object that scans the input file for contained animation frames.
	virtual std::shared_ptr<FileSourceImporter::FrameFinder> createFrameFinder(const FileHandle& file) override {
		activateCLocale();
		return std::make_shared<FrameFinder>(file);
	}

	/// Inspects the header of the given file and returns the number of file columns.
	Future<BondInputColumnMapping> inspectFileHeader(const Frame& frame);

private:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class FrameLoader : public ParticleImporter::FrameLoader
	{
	public:

		/// Constructor.
		FrameLoader(DataSet* dataset, const FileSourceImporter::Frame& frame, const FileHandle& file, 
				const DataCollection* masterCollection, PipelineObject* dataSource, 
				const BondInputColumnMapping& columnMapping)
			: ParticleImporter::FrameLoader(dataset, frame, file, masterCollection, dataSource),
				_columnMapping(columnMapping) {}

		/// Returns the file column mapping used to load the file.
		const BondInputColumnMapping& columnMapping() const { return _columnMapping; }

	protected:

		/// Reads the frame data from the external file.
		virtual void loadFile() override;

	private:

		BondInputColumnMapping _columnMapping;
	};

	/// The format-specific task object that is responsible for scanning the input file for animation frames.
	class FrameFinder : public FileSourceImporter::FrameFinder
	{
	public:

		/// Inherit constructor from base class.
		using FileSourceImporter::FrameFinder::FrameFinder;

	protected:

		/// Scans the data file and builds a list of source frames.
		virtual void discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames) override;
	};

private:

	/// The user-defined mapping of input file columns to OVITO's bond properties.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(BondInputColumnMapping, columnMapping, setColumnMapping);
};

}	// End of namespace
}	// End of namespace
