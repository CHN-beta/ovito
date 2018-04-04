///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////


#pragma once

#include <core/Core.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util)

struct ZLibState;   // Internal data structure

/**
 * \brief A QIODevice adapter that can compress/uncompress a stream of data on the fly.
 * 
 * A GzipIODevice object is constructed with a pointer to an
 * underlying QIODevice.  Data written to the GzipIODevice object
 * will be compressed before it is written to the underlying
 * QIODevice. Similary, if you read from the GzipIODevice object,
 * the data will be read from the underlying device and then
 * decompressed.
 * 
 * GzipIODevice is a sequential device, which means that it does
 * not support seeks or random access. Internally, GzipIODevice
 * uses the zlib library to compress and uncompress data.
 */
class OVITO_CORE_EXPORT GzipIODevice : public QIODevice
{
	Q_OBJECT

public:

    /// The compression formats supported by this class.
	enum StreamFormat {
		ZlibFormat,
		GzipFormat,
		RawZipFormat
	};

    /// Constructor.
    ///
    /// The allowed value range for \a compressionLevel is 0 to 9, where 0 means no compression
    ///and 9 means maximum compression. The default value is 6.
    ///
    /// bufferSize specifies the size of the internal buffer used when reading from and writing to the
    /// underlying device. The default value is 65KB. Using a larger value allows for faster compression and
    /// decompression at the expense of memory usage.
    GzipIODevice(QIODevice* device, int compressionLevel = 6, int bufferSize = 65500);

    /// Destructor.
    virtual ~GzipIODevice();

    /// Selects the compression format to read/write.
    void setStreamFormat(StreamFormat format) { _streamFormat = format; }

    /// Returns the compression format being read/written.
    StreamFormat streamFormat() const { return _streamFormat; }

    /// Stream is always sequential.
    bool isSequential() const override { return true; }

    bool open(OpenMode mode) override;
    void close() override;
    void flush();
    qint64 bytesAvailable() const override;
    bool seek(qint64 pos) override;

protected:

    qint64 readData(char * data, qint64 maxSize) override;
    qint64 writeData(const char * data, qint64 maxSize) override;

private:

    // The states this class can be in:
    enum State {
        // Read state
        NotReadFirstByte,
        InStream,
        EndOfStream,
        // Write state
        NoBytesWritten,
        BytesWritten,
        // Common
        Closed,
        Error
    };

    /// Sets the error string to errorMessage + zlib error string for zlibErrorCode
    void setZlibError(const QString& errorMessage, int zlibErrorCode);    

    /// Flushes the zlib stream.
    void flushZlib(int flushMode);

    /// Writes outputSize bytes from buffer to the inderlying device.
    bool writeBytes(qint64 outputSize);

    bool _manageDevice = false;
    int _compressionLevel;
    QIODevice* _device;
    State _state = Closed;
    StreamFormat _streamFormat = ZlibFormat;
    ZLibState* _zlibStruct;
    qint64 _bufferSize;
    std::unique_ptr<unsigned char[]> _buffer;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace