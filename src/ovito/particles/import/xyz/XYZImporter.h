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


#include <ovito/particles/Particles.h>
#include <ovito/particles/import/ParticleImporter.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/InputColumnMapping.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito { namespace Particles {

/**
 * \brief File parser for the text-based XYZ file format.
 */
class OVITO_PARTICLES_EXPORT XYZImporter : public ParticleImporter
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
		virtual QString fileFilterDescription() const override { return tr("XYZ Files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(XYZImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE XYZImporter(DataSet* dataset) : ParticleImporter(dataset), _autoRescaleCoordinates(true) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("XYZ File"); }

	/// Indicates whether this file importer type loads particle trajectories.
	virtual bool isTrajectoryFormat() const override { return true; } 

	/// Guesses the mapping of input file columns to internal particle properties.
	static bool mapVariableToProperty(ParticleInputColumnMapping& columnMapping, int column, QString name, int dataType, int vec);

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const LoadOperationRequest& request) override {
		activateCLocale();
		return std::make_shared<FrameLoader>(request, recenterCell(), sortParticles(), columnMapping(), autoRescaleCoordinates());
	}

	/// Creates an asynchronous frame discovery object that scans the input file for contained animation frames.
	virtual std::shared_ptr<FileSourceImporter::FrameFinder> createFrameFinder(const FileHandle& file) override {
		activateCLocale();
		return std::make_shared<FrameFinder>(file);
	}

	/// Inspects the header of the given file and returns the number of file columns.
	Future<ParticleInputColumnMapping> inspectFileHeader(const Frame& frame);

private:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class FrameLoader : public ParticleImporter::FrameLoader
	{
	public:

		/// Normal constructor.
		FrameLoader(const LoadOperationRequest& request, bool recenterCell, bool sortParticles, const ParticleInputColumnMapping& columnMapping, bool autoRescaleCoordinates)
		  : ParticleImporter::FrameLoader(request, recenterCell),
			_sortParticles(sortParticles),
			_columnMapping(columnMapping),
			_autoRescaleCoordinates(autoRescaleCoordinates) {}

	protected:

		/// Reads the frame data from the external file.
		virtual void loadFile() override;

	private:

		bool _sortParticles;
		bool _autoRescaleCoordinates;
		ParticleInputColumnMapping _columnMapping;
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

protected:

	/// \brief Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) const override;

	/// \brief Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Interprets the comment line of an extended XYZ file.
	static ParticleInputColumnMapping parseExtendedXYZColumnSpecification(const QString& commentLine);

private:

	/// The user-defined mapping of input file columns to OVITO's particle properties.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticleInputColumnMapping, columnMapping, setColumnMapping);

	/// Controls the automatic detection of reduced atom coordinates in the input file.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, autoRescaleCoordinates, setAutoRescaleCoordinates);
};

}	// End of namespace
}	// End of namespace
