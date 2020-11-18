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

/**
 * \file
 * \brief Contains the definition of the Ovito::IO::SaveStream class.
 */

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/utilities/concurrent/Promise.h>

namespace Ovito {

/**
 * \brief An output stream class that writes binary data to a file in a platform-independent way.
 *
 * The SaveStream class is wrapper for a Qt \c QDataStream object that receives the binary data after conversion to a platform-independent representation.
 * The SaveStream class writes a file header to the output data stream that contains
 * meta information about the platform architecture (32 or 64 bit memory model), the default floating-point precision (single or double)
 * and the endian type used by the application. When reading the data on a different computer, the corresponding LoadStream class
 * will take care of converting the stored data to the architecture-dependent representation used by the local computer.
 *
 * The SaveStream class allows to structure the data using chunks. A new data chunk can be created using
 * the beginChunk() method. Any data subsequently written to the stream before a closing call to endChunk() will
 * become part of the current chunk. Chunks can be nested by calling beginChunk() multiple times
 * before calling endChunk() an equal number of times.
 *
 * Each chunk possesses an integer ID, which is specified on the call to beginChunk(). The ID is stored in the
 * output file and can be used as meta information to describe the contents of the chunk.
 * The interpretation of the chunk ID is completely left to the user of the class.
 *
 * The writePointer() method allows to serialize C++ pointers. The SaveStream class generates a unique
 * ID for each unique pointer written to the stream using this method. On subsequent calls to writePointer()
 * with the same C++ pointer, the same ID will be written to the stream.
 *
 * \sa LoadStream
 */
class OVITO_CORE_EXPORT SaveStream : public QObject
{
	Q_OBJECT

public:

	/// \brief Constructs the stream wrapper.
	/// \param destination The sink that will receive the data. This Qt output stream must support random access.
	/// \param operation The task handle that allows to cancel the save process.
	/// \throw Exception if the given data stream supports only sequential access or if an I/O error occured while writing the file header.
	SaveStream(QDataStream& destination, SynchronousOperation operation);

	/// \brief Automatically closes the stream by calling close().
	virtual ~SaveStream() { SaveStream::close(); }

	/// \brief Closes this stream, but not the underlying output stream passed to the constructor.
	/// \throw Exception if an I/O error has occurred.
	/// \sa isOpen()
	virtual void close();

	/// \brief Returns whether the stream is still open for write operations. Returns \c false after close() has been called.
	/// \sa close()
	bool isOpen() const { return _isOpen; }

	/// \brief Writes an array of raw bytes to the output stream.
	/// \param buffer A pointer to the beginning of the data.
	/// \param numBytes The number of bytes to be written.
	/// \note No conversion is done for the data written to the stream, i.e. the data will not be stored in a platform-independent format.
	/// \throw Exception if an I/O error has occurred.
	void write(const void* buffer, size_t numBytes);

	/// \brief Start a new chunk with the given identifier.
	/// \param chunkId A identifier for this chunk. This identifier can be used
	///                to identify the type of data contained in the chunk during file loading.
	/// The chunk must be closed using endChunk().
	/// \throw Exception if an I/O error has occurred.
	/// \sa endChunk()
    void beginChunk(quint32 chunkId);

	/// \brief Closes the current chunk.
    ///
    /// This method closes the last chunk previously opened using beginChunk().
	/// \throw Exception if an I/O error has occurred.
    /// \sa beginChunk()
	void endChunk();

	/// \brief Returns the current writing position of the underlying output stream in bytes.
	qint64 filePosition() const { return _os.device()->pos(); }

	/// \brief Writes a platform-dependent unsigned integer value (can be 32 or 64 bits) to the stream.
	/// \param value The value to be written to the stream.
	/// \throw Exception when the I/O error has occurred.
	void writeSizeT(size_t value) { _os << (quint64)value; }

	/// \brief Writes a pointer to the stream.
	/// \param pointer The pointer to be written to the stream (can be \c nullptr).
	///
	/// This method generates a unique ID for the pointer and writes the ID to the stream
	/// instead of the pointer itself.
	///
	/// \throw Exception if an I/O error has occurred.
	/// \sa pointerID()
	/// \sa LoadStream::readPointer()
	void writePointer(void* pointer);

	/// \brief Returns the ID for a pointer that was previously written to the stream using writePointer().
	/// \param pointer A pointer.
	/// \return The ID the given pointer was mapped to, or 0 if the pointer hasn't been written to the stream yet.
	/// \sa writePointer()
	quint64 pointerID(void* pointer) const;

	/// Provides direct access to the underlying Qt data stream.
	QDataStream& dataStream() { return _os; }

	/// Returns the task object that represent the save operation.
	SynchronousOperation& operation() { return _operation; }

private:

	/// Checks the status of the underlying output stream and throws an exception if an error has occurred.
	void checkErrorCondition();

	/// Writes a C++ enum to the stream.
	template<typename T>
	void writeValue(T enumValue, const std::true_type&) {
		dataStream() << (qint32)enumValue;
		checkErrorCondition();
	}

	/// Writes a non-enum to the stream.
	template<typename T>
	void writeValue(T v, const std::false_type&) {
		dataStream() << v;
		checkErrorCondition();
	}

	template<typename T> friend SaveStream& operator<<(SaveStream& stream, const T& v);
	friend OVITO_CORE_EXPORT SaveStream& operator<<(SaveStream& stream, const QUrl& url);

private:

	/// Indicates the output stream is still open.
	bool _isOpen = false;

	/// The output stream.
	QDataStream& _os;

	/// The task object.
	SynchronousOperation _operation;

	/// The stack of open chunks.
	std::stack<qint64> _chunks;

	/// Maps pointers to IDs.
	std::map<void*, quint64> _pointerMap;
};

/// \brief Writes a value to a SaveStream.
/// \relates SaveStream
///
/// \param stream The destination stream.
/// \param v The value to write to the stream. This must be a value type supported by the QDataStream class of Qt.
/// \return The destination stream.
/// \throw Exception if an I/O error has occurred.
///
/// This operator forwards the value to the output operator of the underlying Qt \c QDataStream class.
template<typename T>
inline SaveStream& operator<<(SaveStream& stream, const T& v)
{
	stream.writeValue(v, std::is_enum<T>());
	return stream;
}

/// \brief Writes a vector container to a SaveStream.
/// \relates SaveStream
///
/// \param stream The destination stream.
/// \param v The vector to write to the stream.
/// \return The destination stream.
/// \throw Exception if an I/O error has occurred.
template<typename T>
inline SaveStream& operator<<(SaveStream& stream, const QVector<T>& v)
{
	stream.writeSizeT(v.size());
	for(const auto& el : v)
		stream << el;
	return stream;
}

/// \brief Writes a vector container to a SaveStream.
/// \relates SaveStream
///
/// \param stream The destination stream.
/// \param v The vector to write to the stream.
/// \return The destination stream.
/// \throw Exception if an I/O error has occurred.
template<typename T>
inline SaveStream& operator<<(SaveStream& stream, const std::vector<T>& v)
{
	stream.writeSizeT(v.size());
	for(const auto& el : v)
		stream << el;
	return stream;
}

/// \brief Writes an array of values to the output stream.
/// \relates SaveStream
///
/// \param stream The destination stream.
/// \param a The array to be written to the stream.
/// \return The destination stream.
/// \throw Exception if an I/O error has occurred.
template<typename T, std::size_t N>
inline SaveStream& operator<<(SaveStream& stream, const std::array<T, N>& a)
{
	for(typename std::array<T, N>::size_type i = 0; i < N; ++i)
		stream << a[i];
	return stream;
}

/// \brief Writes Qt flag to the output stream.
/// \relates SaveStream
///
/// \param stream The destination stream.
/// \param a The flag value
/// \return The destination stream.
/// \throw Exception if an I/O error has occurred.
template<typename Enum>
inline SaveStream& operator<<(SaveStream& stream, const QFlags<Enum>& a)
{
	return stream << (typename QFlags<Enum>::enum_type)(typename QFlags<Enum>::Int)a;
}

/// \brief Writes a bit vector to a SaveStream.
/// \relates SaveStream
///
/// \param stream The destination stream.
/// \param bs The bit vector to write to the stream.
/// \return The destination stream.
/// \throw Exception if an I/O error has occurred.
inline SaveStream& operator<<(SaveStream& stream, const boost::dynamic_bitset<>& bs)
{
	stream.writeSizeT(bs.size());
	std::vector<boost::dynamic_bitset<>::block_type> blocks(bs.num_blocks());
	boost::to_block_range(bs, blocks.begin());
	stream.write(blocks.data(), blocks.size() * sizeof(boost::dynamic_bitset<>::block_type));
	return stream;
}

/// \brief Writes a URL to a SaveStream.
/// \relates SaveStream
///
/// \param stream The destination stream.
/// \param url The URL to store.
/// \return The destination stream.
/// \throw Exception if an I/O error has occurred.
extern OVITO_CORE_EXPORT SaveStream& operator<<(SaveStream& stream, const QUrl& url);

/// \brief Writes a reference to an OvitoObject derived class type to the stream.
/// \relates SaveStream
///
/// \param stream The destination stream.
/// \param clazz The OvitoObject class type
/// \return The destination stream.
/// \throw Exception if an I/O error has occurred.
extern OVITO_CORE_EXPORT SaveStream& operator<<(SaveStream& stream, const OvitoClassPtr& clazz);

/// \brief Writes a reference to an OvitoObject derived class type to the stream.
/// \relates SaveStream
///
/// \param stream The destination stream.
/// \param clazz The OvitoObject class type
/// \return The destination stream.
/// \throw Exception if an I/O error has occurred.
template<class OvitoSubclass>
auto operator<<(SaveStream& stream, const OvitoSubclass* const clazz)
	-> std::enable_if_t<std::is_base_of<OvitoClass, OvitoSubclass>::value, SaveStream&>
{
	return stream << static_cast<const OvitoClassPtr&>(clazz);
}

}	// End of namespace
