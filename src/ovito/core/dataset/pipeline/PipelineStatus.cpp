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

#include <ovito/core/Core.h>
#include "PipelineStatus.h"

namespace Ovito {

/******************************************************************************
* Writes a status object to a file stream.
******************************************************************************/
SaveStream& operator<<(SaveStream& stream, const PipelineStatus& s)
{
	stream.beginChunk(0x02);
	stream << s._type;
	stream << s._text;
	stream.endChunk();
	return stream;
}

/******************************************************************************
* Reads a status object from a binary input stream.
******************************************************************************/
LoadStream& operator>>(LoadStream& stream, PipelineStatus& s)
{
	quint32 version = stream.expectChunkRange(0x0, 0x02);
	stream >> s._type;
	stream >> s._text;
	if(version <= 0x01)
		stream >> s._text;
	stream.closeChunk();
	return stream;
}

/******************************************************************************
* Writes a status object to the log stream.
******************************************************************************/
QDebug operator<<(QDebug debug, const PipelineStatus& s)
{
	switch(s.type()) {
	case PipelineStatus::Success: debug << "Success"; break;
	case PipelineStatus::Warning: debug << "Warning"; break;
	case PipelineStatus::Error: debug << "Error"; break;
	}
	if(s.text().isEmpty() == false)
		debug << s.text();
	return debug;
}

}	// End of namespace
