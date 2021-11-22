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
#include <ovito/core/app/Application.h>

namespace Ovito::Particles {

/**
 * \brief File parser for data files of the oxDNA code.
 * 
 * File format documentation:
 * 
 * https://dna.physics.ox.ac.uk/index.php/Documentation#Visualisation_of_structures
 */
class OVITO_OXDNA_EXPORT OXDNAImporter : public ParticleImporter
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
		virtual QString fileFilterDescription() const override { return tr("oxDNA Configuration Files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(OXDNAImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructor.
	Q_INVOKABLE OXDNAImporter(DataSet *dataset) : ParticleImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("oxDNA"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const LoadOperationRequest& request) override {
		activateCLocale();
		return std::make_shared<FrameLoader>(request, topologyFileUrl());
	}

	/// Creates an asynchronous frame discovery object that scans the input file for contained animation frames.
	virtual std::shared_ptr<FileSourceImporter::FrameFinder> createFrameFinder(const FileHandle& file) override {
		activateCLocale();
		return std::make_shared<FrameFinder>(file);
	}

private:

	/// The format-specific task object that is responsible for reading an input file in a separate thread.
	class FrameLoader : public ParticleImporter::FrameLoader
	{
	public:

		/// Constructor.
		FrameLoader(const LoadOperationRequest& request, const QUrl& userSpecifiedTopologyUrl) :
			ParticleImporter::FrameLoader(request), 
			_userSpecifiedTopologyUrl(userSpecifiedTopologyUrl) {}

	protected:

		/// Reads the frame data from the external file.
		virtual void loadFile() override;

		/// URL of the topology file if explicitly specified by the user.
		QUrl _userSpecifiedTopologyUrl;
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

	/// oxDNA files come in pairs: a topology file and a configuration file. 
	/// The configuration file is the primary file passed to the file importer by the system.
	/// This extra field stores the URL of the oxDNA topology file belonging to the configuration file
	/// if explicitly specified by the user.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QUrl, topologyFileUrl, setTopologyFileUrl);
};

}	// End of namespace
