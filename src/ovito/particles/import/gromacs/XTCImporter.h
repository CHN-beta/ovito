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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito { namespace Particles {

/**
 * File parser for GROMACS XTC trajectory files.
 */
class OVITO_PARTICLES_EXPORT XTCImporter : public ParticleImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public ParticleImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleImporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.xtc"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("Gromacs Trajectory Files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(XTCImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE XTCImporter(DataSet* dataset) : ParticleImporter(dataset) {
		setMultiTimestepFile(true);
	}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("XTC"); }

	/// Indicates whether this file importer type loads particle trajectories.
	virtual bool isTrajectoryFormat() const override { return true; } 

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const Frame& frame, const FileHandle& file, const DataCollection* masterCollection, PipelineObject* dataSource) override {
		return std::make_shared<FrameLoader>(dataset(), frame, std::move(file), masterCollection, dataSource);
	}

	/// Creates an asynchronous frame discovery object that scans the input file for contained animation frames.
	virtual std::shared_ptr<FileSourceImporter::FrameFinder> createFrameFinder(const FileHandle& file) override {
		return std::make_shared<FrameFinder>(file);
	}

private:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class FrameLoader : public ParticleImporter::FrameLoader
	{
	public:

		/// Inherit constructor from base class.
		using ParticleImporter::FrameLoader::FrameLoader;

	protected:

		/// Reads the frame data from the external file.
		virtual void loadFile() override;
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
};

}	// End of namespace
}	// End of namespace