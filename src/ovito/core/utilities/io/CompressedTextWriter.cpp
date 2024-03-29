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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/DataSet.h>
#include "CompressedTextWriter.h"

#include <boost/spirit/include/karma.hpp>

namespace Ovito {

/******************************************************************************
* Opens the output file for writing.
******************************************************************************/
CompressedTextWriter::CompressedTextWriter(QFileDevice& output, DataSet* context) :
	_device(output), 
#ifdef OVITO_ZLIB_SUPPORT
	_compressor(&output), 
#endif
	_context(context)
{
	_filename = output.fileName();

	// Check if file should be compressed (i.e. filename ends with .gz).
	if(_filename.endsWith(".gz", Qt::CaseInsensitive)) {
#ifdef OVITO_ZLIB_SUPPORT
		// Open file for writing.
		_compressor.setStreamFormat(GzipIODevice::GzipFormat);
		if(!_compressor.open(QIODevice::WriteOnly))
			throw Exception(tr("Failed to open output file '%1' for writing: %2").arg(_filename).arg(_compressor.errorString()), _context);
		_stream = &_compressor;
#else
		throw Exception(tr("Cannot open file '%1' for writing. This version of OVITO was built without I/O support for gzip compressed files.").arg(_filename), _context);
#endif
	}
	else {
		// Open file for writing.
		if(!output.open(QIODevice::WriteOnly | QIODevice::Text))
			throw Exception(tr("Failed to open output file '%1' for writing: %2").arg(_filename).arg(output.errorString()), _context);
		_stream = &output;
	}
}

/******************************************************************************
* Writes an integer number to the text-based output file.
******************************************************************************/
CompressedTextWriter& CompressedTextWriter::operator<<(qint32 i)
{
	using namespace boost::spirit;

	char buffer[16];
	char *s = buffer;
	karma::generate(s, karma::int_generator<qint32>(), i);
	OVITO_ASSERT(s - buffer < sizeof(buffer));
	if(_stream->write(buffer, s - buffer) == -1)
		reportWriteError();

	return *this;
}

/******************************************************************************
* Writes an integer number to the text-based output file.
******************************************************************************/
CompressedTextWriter& CompressedTextWriter::operator<<(quint32 i)
{
	using namespace boost::spirit;

	char buffer[16];
	char *s = buffer;
	karma::generate(s, karma::uint_generator<quint32>(), i);
	OVITO_ASSERT(s - buffer < sizeof(buffer));
	if(_stream->write(buffer, s - buffer) == -1)
		reportWriteError();

	return *this;
}

/******************************************************************************
* Writes an integer number to the text-based output file.
******************************************************************************/
CompressedTextWriter& CompressedTextWriter::operator<<(qint64 i)
{
	using namespace boost::spirit;

	char buffer[32];
	char *s = buffer;
	karma::generate(s, karma::int_generator<qint64>(), i);
	OVITO_ASSERT(s - buffer < sizeof(buffer));
	if(_stream->write(buffer, s - buffer) == -1)
		reportWriteError();

	return *this;
}

/******************************************************************************
* Writes an integer number to the text-based output file.
******************************************************************************/
CompressedTextWriter& CompressedTextWriter::operator<<(quint64 i)
{
	using namespace boost::spirit;

	char buffer[32];
	char *s = buffer;
	karma::generate(s, karma::uint_generator<quint64>(), i);
	OVITO_ASSERT(s - buffer < sizeof(buffer));
	if(_stream->write(buffer, s - buffer) == -1)
		reportWriteError();

	return *this;
}

#if (!defined(Q_OS_WIN) && (QT_POINTER_SIZE != 4)) || defined(Q_OS_WASM)
/******************************************************************************
* Writes an integer number to the text-based output file.
******************************************************************************/
CompressedTextWriter& CompressedTextWriter::operator<<(size_t i)
{
	using namespace boost::spirit;

	char buffer[32];
	char *s = buffer;
	karma::generate(s, karma::uint_generator<size_t>(), i);
	OVITO_ASSERT(s - buffer < sizeof(buffer));
	if(_stream->write(buffer, s - buffer) == -1)
		reportWriteError();

	return *this;
}
#endif

/******************************************************************************
* Writes an integer number to the text-based output file.
******************************************************************************/
CompressedTextWriter& CompressedTextWriter::operator<<(FloatType f)
{
	char buffer[std::numeric_limits<FloatType>::max_digits10 + 8];
	char *s = buffer;

	// Workaround for Boost bug #5983 (https://svn.boost.org/trac/boost/ticket/5983)
	// Karma cannot format subnormal floating point numbers.
	// Have to switch to a slower formatting method (sprintf) in this case.
	if(std::fpclassify(f) != FP_SUBNORMAL) {

		using namespace boost::spirit;

		// Define Boost Karma generator to control floating-point to string conversion.
		struct floattype_format_policy : karma::real_policies<FloatType> {
			floattype_format_policy(unsigned int precision) : _precision(precision) {}
		    unsigned int precision(FloatType n) const { return _precision; }
			unsigned int _precision;
		};
		karma::generate(s, karma::real_generator<FloatType, floattype_format_policy>(_floatPrecision), f);
	}
	else {
		// Use sprintf() to handle denormalized floating point numbers.
#if !defined(Q_CC_MSVC) || _MSC_VER >= 1900
		s += std::snprintf(buffer, sizeof(buffer), "%.*g", _floatPrecision, f);
#else
		// MSVC 2013 is not fully C++11 standard compliant. Need to use _snprintf() instead.
		s += _snprintf(buffer, sizeof(buffer), "%.*g", _floatPrecision, f);
#endif
	}

	OVITO_ASSERT(s - buffer < sizeof(buffer));
	if(_stream->write(buffer, s - buffer) == -1)
		reportWriteError();

	return *this;
}

/******************************************************************************
* Throws an exception to report an I/O error.
******************************************************************************/
void CompressedTextWriter::reportWriteError()
{
	throw Exception(tr("Failed to write output file '%1': %2").arg(filename()).arg(_stream->errorString()), _context);
}

}	// End of namespace
