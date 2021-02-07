////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 OVITO GmbH, Germany
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

#include <ovito/gui/desktop/GUI.h>
#ifdef OVITO_VIDEO_OUTPUT_SUPPORT
	#include <ovito/core/utilities/io/video/VideoEncoder.h>
#endif
#include "SaveImageFileDialog.h"

namespace Ovito {

/******************************************************************************
* Constructs the dialog window.
******************************************************************************/
SaveImageFileDialog::SaveImageFileDialog(QWidget* parent, const QString& caption, bool includeVideoFormats, const ImageInfo& imageInfo) :
	HistoryFileDialog("save_image", parent, caption), _imageInfo(imageInfo)
{
	connect(this, &QFileDialog::fileSelected, this, &SaveImageFileDialog::onFileSelected);
	connect(this, &QFileDialog::filterSelected, this, &SaveImageFileDialog::onFilterSelected);

	// Build filter string.
	QStringList filterStrings;
	QList<QByteArray> supportedFormats = QImageWriter::supportedImageFormats();

	// Add image formats.
	if(supportedFormats.contains("png")) { filterStrings << tr("PNG image file (*.png)"); _formatList << "png"; }
	if(supportedFormats.contains("jpg")) { filterStrings << tr("JPEG image file (*.jpg *.jpeg)"); _formatList << "jpg"; }
	if(supportedFormats.contains("eps")) { filterStrings << tr("EPS Encapsulated PostScript (*.eps)"); _formatList << "eps"; }
	if(supportedFormats.contains("tiff")) { filterStrings << tr("TIFF Tagged image file (*.tif *.tiff)"); _formatList << "tiff"; }
	if(supportedFormats.contains("tga")) { filterStrings << tr("TGA Targa image file (*.tga)"); _formatList << "tga"; }

#ifdef OVITO_VIDEO_OUTPUT_SUPPORT
	if(includeVideoFormats) {
		// Add video formats.
		for(const auto& videoFormat : VideoEncoder::supportedFormats()) {
			QString filterString = videoFormat.longName + " (";
			for(const QString& ext : videoFormat.extensions)
				filterString += "*." + ext;
			filterString += ")";
			filterStrings << filterString;
			_formatList << videoFormat.name;
		}
	}
#endif

	if(filterStrings.isEmpty())
		throw Exception(tr("There are no image format plugins available."));

	setNameFilters(filterStrings);
	setAcceptMode(QFileDialog::AcceptSave);
	setLabelText(QFileDialog::FileType, tr("Save as type"));
	if(_imageInfo.filename().isEmpty() == false)
		selectFile(_imageInfo.filename());

	int index = _formatList.indexOf(_imageInfo.format().toLower());
	if(index >= 0) selectNameFilter(filterStrings[index]);

	// Select the default suffix.
	onFilterSelected(selectedNameFilter());
}

/******************************************************************************
* This is called when the user has selected a file format.
******************************************************************************/
void SaveImageFileDialog::onFilterSelected(const QString& filter)
{
	int index = nameFilters().indexOf(filter);
	if(index >= 0 && index < _formatList.size())
		setDefaultSuffix(_formatList[index]);
}

/******************************************************************************
* This is called when the user has pressed the OK button of the dialog.
******************************************************************************/
void SaveImageFileDialog::onFileSelected(const QString& file)
{
	_imageInfo.setFilename(file);
	int index = nameFilters().indexOf(selectedNameFilter());
	if(index >= 0 && index < _formatList.size())
		_imageInfo.setFormat(_formatList[index]);
}

}	// End of namespace
