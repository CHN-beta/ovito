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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/import/lammps/LAMMPSDataImporter.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include "LAMMPSDataImporterEditor.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(LAMMPSDataImporterEditor);
SET_OVITO_OBJECT_EDITOR(LAMMPSDataImporter, LAMMPSDataImporterEditor);

/******************************************************************************
* This method is called by the FileSource each time a new source
* file has been selected by the user.
******************************************************************************/
bool LAMMPSDataImporterEditor::inspectNewFile(FileImporter* importer, const QUrl& sourceFile, QWidget* parent)
{
	LAMMPSDataImporter* dataImporter = static_object_cast<LAMMPSDataImporter>(importer);

	// Inspect the data file and try to detect the LAMMPS atom style.
	Future<LAMMPSDataImporter::LAMMPSAtomStyle> inspectFuture = dataImporter->inspectFileHeader(FileSourceImporter::Frame(sourceFile));
	if(!importer->dataset()->taskManager().waitForFuture(inspectFuture))
		return false;
	LAMMPSDataImporter::LAMMPSAtomStyle detectedAtomStyle = inspectFuture.result();

	// Show dialog to ask user for the right LAMMPS atom style if it could not be detected.
	if(detectedAtomStyle == LAMMPSDataImporter::AtomStyle_Unknown) {

		QMap<QString, LAMMPSDataImporter::LAMMPSAtomStyle> styleList = {
				{ QStringLiteral("angle"), LAMMPSDataImporter::AtomStyle_Angle },
				{ QStringLiteral("atomic"), LAMMPSDataImporter::AtomStyle_Atomic },
				{ QStringLiteral("body"), LAMMPSDataImporter::AtomStyle_Body },
				{ QStringLiteral("bond"), LAMMPSDataImporter::AtomStyle_Bond },
				{ QStringLiteral("charge"), LAMMPSDataImporter::AtomStyle_Charge },
				{ QStringLiteral("dipole"), LAMMPSDataImporter::AtomStyle_Dipole },
				{ QStringLiteral("dpd"), LAMMPSDataImporter::AtomStyle_DPD },
				{ QStringLiteral("edpd"), LAMMPSDataImporter::AtomStyle_EDPD },
				{ QStringLiteral("mdpd"), LAMMPSDataImporter::AtomStyle_MDPD },
				{ QStringLiteral("electron"), LAMMPSDataImporter::AtomStyle_Electron },
				{ QStringLiteral("ellipsoid"), LAMMPSDataImporter::AtomStyle_Ellipsoid },
				{ QStringLiteral("full"), LAMMPSDataImporter::AtomStyle_Full },
				{ QStringLiteral("line"), LAMMPSDataImporter::AtomStyle_Line },
				{ QStringLiteral("meso"), LAMMPSDataImporter::AtomStyle_Meso },
				{ QStringLiteral("molecular"), LAMMPSDataImporter::AtomStyle_Molecular },
				{ QStringLiteral("peri"), LAMMPSDataImporter::AtomStyle_Peri },
				{ QStringLiteral("smd"), LAMMPSDataImporter::AtomStyle_SMD },
				{ QStringLiteral("sphere"), LAMMPSDataImporter::AtomStyle_Sphere },
				{ QStringLiteral("template"), LAMMPSDataImporter::AtomStyle_Template },
				{ QStringLiteral("tri"), LAMMPSDataImporter::AtomStyle_Tri },
				{ QStringLiteral("wavepacket"), LAMMPSDataImporter::AtomStyle_Wavepacket }
		};
		QStringList itemList = styleList.keys();

		QSettings settings;
		settings.beginGroup(LAMMPSDataImporter::OOClass().plugin()->pluginId());
		settings.beginGroup(LAMMPSDataImporter::OOClass().name());

		QString currentItem;
		for(int i = 0; i < itemList.size(); i++) {
			if(dataImporter->atomStyle() == styleList[itemList[i]]) {
				currentItem = itemList[i];
				break;
			}
		}
		if(currentItem.isEmpty())
			currentItem = settings.value("DefaultAtomStyle").toString();
		if(currentItem.isEmpty())
			currentItem = QStringLiteral("atomic");

		QInputDialog dlg(parent);
		dlg.setLabelText((dataImporter->atomStyle() == LAMMPSDataImporter::AtomStyle_Unknown) ? 
				tr("<html><p>Please select the LAMMPS <i>atom style</i> for this LAMMPS data file. "
				"OVITO could not detect it automatically, because the file does not "
				"contain a <a href=\"https://docs.lammps.org/read_data.html#format-of-the-body-of-a-data-file\">style hint</a> in its <i>Atoms</i> section.</p>"
				"<p>If you don't know what the correct atom style is, see the <a href=\"https://docs.lammps.org/atom_style.html\">LAMMPS documentation</a> or " 
				"check the value of the <i>atom_style</i> command in your LAMMPS input script.</p>"
				"<p>LAMMPS atom style:</p></html>") :
				tr("LAMMPS atom style:"));
		dlg.setWindowTitle(tr("LAMMPS data file"));
		dlg.setComboBoxEditable(false);
		dlg.setComboBoxItems(itemList);
		dlg.setInputMode(QInputDialog::TextInput);
		dlg.setTextValue(currentItem);
		if(QLabel* label = dlg.findChild<QLabel*>()) {
			label->setTextInteractionFlags(Qt::TextBrowserInteraction);
			label->setOpenExternalLinks(true);
			label->setWordWrap(true);
		}
		if(dlg.exec() != QDialog::Accepted)
			return false;
		currentItem = dlg.textValue();
		settings.setValue("DefaultAtomStyle", currentItem);
		dataImporter->setAtomStyle(styleList[currentItem]);
	}
	else {
		dataImporter->setAtomStyle(detectedAtomStyle);
	}

	return true;
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void LAMMPSDataImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("LAMMPS data reader"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* optionsBox = new QGroupBox(tr("Options"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(optionsBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(optionsBox);

	// Sort particles
	BooleanParameterUI* sortParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::sortParticles));
	sublayout->addWidget(sortParticlesUI->checkBox());
}

}	// End of namespace
}	// End of namespace
