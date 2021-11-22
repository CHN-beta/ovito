////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 OVITO GmbH, Germany
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

#include <ovito/vorotop/VoroTopPlugin.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/core/utilities/io/NumberParsing.h>
#include <ovito/core/utilities/concurrent/Promise.h>
#include "Filter.h"

namespace Ovito::VoroTop {

/******************************************************************************
* Loads the filter definition from the given input stream.
******************************************************************************/
bool Filter::load(CompressedTextReader& stream, bool readHeaderOnly, Task& operation)
{
	// Parse comment lines starting with '#':
	_filterDescription.clear();
	const char* line;
	while(!stream.eof()) {
		line = stream.readLineTrimLeft();
		if(line[0] != '#') break;
		_filterDescription += QString::fromUtf8(line + 1).trimmed() + QChar('\n');
		if(operation.isCanceled()) return false;
	}

	// Create the default "Other" structure type.
	_structureTypeLabels.clear();
	_structureTypeLabels.push_back("Other");
	_structureTypeDescriptions.clear();
	_structureTypeDescriptions.push_back(QString());

	// Parse list of structure types (lines starting with '*').
	while(!stream.eof()) {
		if(line[0] != '*') break;
		int typeId;
		int stringLength;
		if(sscanf(line, "* %i%n", &typeId, &stringLength) != 1)
			throw Exception(QString("Invalid structure type definition in line %1 of VoroTop filter definition file").arg(stream.lineNumber()));
		if(typeId != _structureTypeLabels.size())
			throw Exception(QString("Invalid structure type definition in line %1 of VoroTop filter definition file: Type IDs must start at 1 and form a consecutive sequence.").arg(stream.lineNumber()));
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
		QStringList columns = QString::fromUtf8(line + stringLength).trimmed().split(QChar('\t'), Qt::SkipEmptyParts);
#else
		QStringList columns = QString::fromUtf8(line + stringLength).trimmed().split(QChar('\t'), QString::SkipEmptyParts);
#endif
		if(columns.empty())
			throw Exception(QString("Invalid structure type definition in line %1 of VoroTop filter definition file: Type label is missing.").arg(stream.lineNumber()));
		_structureTypeLabels.push_back(columns[0]);
		_structureTypeDescriptions.push_back(columns.size() >= 2 ? columns[1] : QString());

		line = stream.readLineTrimLeft();
		if(operation.isCanceled()) return false;
	}
	if(_structureTypeLabels.size() <= 1)
		throw Exception(QString("Invalid filter definition file"));

	if(readHeaderOnly)
		return !operation.isCanceled();

	operation.setProgressMaximum(stream.underlyingSize());

	// Parse Weinberg vector list.
	for(;;) {

		// Parse structure type the current Weinberg code will be mapped to.
		int typeId;
		int stringLength;
		if(sscanf(line, "%i (%n", &typeId, &stringLength) != 1 || typeId <= 0 || typeId >= _structureTypeLabels.size())
			throw Exception(QString("Invalid Weinberg vector in line %1 of VoroTop filter definition file").arg(stream.lineNumber()));
        line += stringLength;

		// Parse Weinberg code sequence.
		WeinbergVector wvector;
		for(;;) {
			const char* s = line;
			while(*s != '\0' && *s != ')' && *s != ',')
				++s;
			int label;
			if(*s == '\0' || !parseInt(line, s, label))
				throw Exception(QString("Invalid Weinberg vector in line %1 of VoroTop filter definition file").arg(stream.lineNumber()));
			wvector.push_back(label);
            if(label > maximumVertices) maximumVertices = label;

			if(*s == ')') break;
			line = s + 1;
		}
        int edges = (wvector.size()-1)/2;
        if(edges > maximumEdges) maximumEdges = edges;

		_entries.emplace(std::move(wvector), typeId);
		if(stream.eof()) break;

		line = stream.readNonEmptyLine();

		// Update progress indicator.
		if(!operation.setProgressValueIntermittent(stream.underlyingByteOffset()))
			return false;
	}

	return !operation.isCanceled();
}

}	// End of namespace
