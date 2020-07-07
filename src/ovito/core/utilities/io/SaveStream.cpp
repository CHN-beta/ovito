////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
#include <ovito/core/utilities/Exception.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/oo/OvitoClass.h>
#include "SaveStream.h"

namespace Ovito {

using namespace std;

/******************************************************************************
* Opens the stream for writing.
******************************************************************************/
SaveStream::SaveStream(QDataStream& destination, SynchronousOperation operation) : 
	_os(destination), _operation(std::move(operation))
{
	OVITO_ASSERT(_operation.isValid());
	OVITO_ASSERT_MSG(!_os.device()->isSequential(), "SaveStream constructor", "SaveStream class requires a seekable output stream.");
	if(_os.device()->isSequential())
		throw Exception("SaveStream class requires a seekable output stream.");

	_isOpen = true;

	// Write file header.

	// This is used to recognize the application file format.
	*this << (quint32)0x0FACC5AB;	// The first magic file code.
	*this << (quint32)0x0AFCCA5A;	// The second magic file code.

	// This is the version of the stream file format.
	*this << (quint32)OVITO_FILE_FORMAT_VERSION;
	_os.setVersion(QDataStream::Qt_5_4);
	_os.setFloatingPointPrecision(sizeof(FloatType) == 4 ? QDataStream::SinglePrecision : QDataStream::DoublePrecision);

	// Store the floating-point precision used throughout the file.
	*this << (quint32)sizeof(FloatType);

	// Write application name.
	*this << Application::applicationName();

	// Write application version.
	*this << (quint32)Application::applicationVersionMajor();
	*this << (quint32)Application::applicationVersionMinor();
	*this << (quint32)Application::applicationVersionRevision();
	*this << Application::applicationVersionString();
}

/******************************************************************************
* Closes the stream.
******************************************************************************/
void SaveStream::close()
{
	if(_isOpen) {
		_isOpen = false;
	}
}

/******************************************************************************
* Writes an array of bytes to the ouput stream.
******************************************************************************/
void SaveStream::write(const void* buffer, size_t numBytes)
{
	if(_os.device()->write(reinterpret_cast<const char*>(buffer), numBytes) != numBytes)
		throw Exception(tr("Failed to write output file. %1").arg(_os.device()->errorString()));
}

/******************************************************************************
* Start a new chunk with the given id and put it on the stack.
******************************************************************************/
void SaveStream::beginChunk(quint32 chunkId)
{
    *this << chunkId;
	*this << (quint32)0;	// This will be backpatched by endChunk().

	_chunks.push(filePosition());
}

/******************************************************************************
* Closes the chunk on the top of the chunk stack.
******************************************************************************/
void SaveStream::endChunk()
{
	OVITO_ASSERT(!_chunks.empty());
	qint64 chunkStart = _chunks.top();
	_chunks.pop();

	qint64 chunkSize = filePosition() - chunkStart;
	OVITO_ASSERT(chunkSize >= 0 && chunkSize <= 0xFFFFFFFF);

	// Write chunk end code.
	*this << (quint32)0x0FFFFFFF;

	// Seek to chunk size field.
	if(!_os.device()->seek(chunkStart - sizeof(unsigned int)) )
		throw Exception(tr("Failed to close chunk in output file."));

    // Patch chunk size field.
	*this << (quint32)chunkSize;

	// Jump back to end of file.
	if(!_os.device()->seek(_os.device()->size()))
		throw Exception(tr("Failed to close chunk in output file."));

	OVITO_ASSERT(filePosition() == chunkStart + chunkSize + sizeof(unsigned int));
}

/******************************************************************************
* Writes a pointer to the stream.
* This method generates a unique ID for the given pointer that
* is written to the stream instead of the pointer itself.
******************************************************************************/
void SaveStream::writePointer(void* pointer)
{
	if(pointer == nullptr) *this << (quint64)0;
	else {
		quint64& id = _pointerMap[pointer];
		if(id == 0) id = (quint64)_pointerMap.size();
		*this << id;
	}
}

/******************************************************************************
* Returns the ID for a pointer that was used to write the pointer to the stream.
* Return 0 if the given pointer hasn't been written to the stream yet.
******************************************************************************/
quint64 SaveStream::pointerID(void* pointer) const
{
	auto iter = _pointerMap.find(pointer);
	if(iter == _pointerMap.end()) return 0;
	return iter->second;
}

/******************************************************************************
* Checks the status of the underlying output stream and throws an exception
* if an error has occurred.
******************************************************************************/
void SaveStream::checkErrorCondition()
{
	if(dataStream().status() != QDataStream::Ok) {
		throw Exception(tr("I/O error: Could not write to file."));
	}
}

/******************************************************************************
* Writes a reference to an OvitoObject derived class type to the stream.
******************************************************************************/
SaveStream& operator<<(SaveStream& stream, const OvitoClassPtr& clazz)
{
	OvitoClass::serializeRTTI(stream, clazz);
	return stream;
}

/******************************************************************************
* Writes a URL to a SaveStream.
******************************************************************************/
SaveStream& operator<<(SaveStream& stream, const QUrl& url)
{
	// Write original URL to stream.
	stream.writeValue(url, std::false_type());
	// Additionally write the path relative to current output file to stream.
	// Currently this only works if the file referenced by the URL is in the same directory as the stream destination file.
	QString relativePath;
	if(url.isLocalFile() && !url.isRelative()) {
		// Extract relative portion of path (only if both the scene file path and the external file path are absolute).
		if(QFileDevice* fileDevice = qobject_cast<QFileDevice*>(stream.dataStream().device())) {
			QFileInfo streamFile(fileDevice->fileName());
			if(streamFile.isAbsolute()) {
				QFileInfo dataFile(url.toLocalFile());
				if(dataFile.path() == streamFile.path()) {
					relativePath = dataFile.fileName();
				}
			}
		}
	}
	stream << relativePath;
	return stream;
}

}	// End of namespace
