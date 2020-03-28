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

#include <ovito/core/Core.h>
#include <ovito/core/utilities/concurrent/Future.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/dataset/DataSetContainer.h>
#ifdef OVITO_SSH_CLIENT
	#include <ovito/core/utilities/io/ssh/SshConnection.h>
#endif
#include "FileManager.h"
#include "RemoteFileJob.h"

namespace Ovito {

#ifdef OVITO_SSH_CLIENT
using namespace Ovito::Ssh;
#endif

/******************************************************************************
* Create a QIODevice that permits reading data from the file referred to by this handle.
******************************************************************************/
std::unique_ptr<QIODevice> FileHandle::createIODevice() const 
{
	if(!localFilePath().isEmpty()) {
		return std::make_unique<QFile>(localFilePath());
	}
	else {
		auto buffer = std::make_unique<QBuffer>();
		buffer->setData(_fileData);
		OVITO_ASSERT(buffer->data().constData() == _fileData.constData()); // Rely on a shallow copy of the buffer being created.
		return buffer;
	}
}

/******************************************************************************
* Destructor.
******************************************************************************/
FileManager::~FileManager()
{
#ifdef OVITO_SSH_CLIENT
    for(SshConnection* connection : _unacquiredConnections) {
        disconnect(connection, nullptr, this, nullptr);
        delete connection;
    }
    Q_ASSERT(_acquiredConnections.empty());
#endif
}

/******************************************************************************
* Makes a file available on this computer.
******************************************************************************/
SharedFuture<FileHandle> FileManager::fetchUrl(TaskManager& taskManager, const QUrl& url)
{
	if(url.isLocalFile()) {
		// Nothing to do to fetch local files. Simply return a finished Future object.

		// But first check if the file exists.
		QString filePath = url.toLocalFile();
		if(!QFileInfo(filePath).exists())
			return Future<FileHandle>::createFailed(Exception(tr("File does not exist:\n%1").arg(filePath), taskManager.datasetContainer()));

		return FileHandle(url, std::move(filePath));
	}
	else if(url.scheme() == QStringLiteral("sftp")) {
#ifdef OVITO_SSH_CLIENT
		QUrl normalizedUrl = normalizeUrl(url);
		QMutexLocker lock(&mutex());

		// Check if requested URL is already in the cache.
		if(auto cacheEntry = _downloadedFiles.object(normalizedUrl)) {
			return FileHandle(url, cacheEntry->fileName());
		}

		// Check if requested URL is already being loaded.
		auto inProgressEntry = _pendingFiles.find(normalizedUrl);
		if(inProgressEntry != _pendingFiles.end()) {
			SharedFuture<FileHandle> future = inProgressEntry->second.lock();
			if(future.isValid())
				return future;
			else
				_pendingFiles.erase(inProgressEntry);
		}

		// Start the background download job.
		DownloadRemoteFileJob* job = new DownloadRemoteFileJob(url, taskManager);
		auto future = job->sharedFuture();
		_pendingFiles.emplace(normalizedUrl, future);
		return future;
#else
		return Future<FileHandle>::createFailed(Exception(tr("URL scheme not supported. This version of OVITO was built without support for the sftp:// protocol and can open local files only."), taskManager.datasetContainer()));
#endif
	}
	else {
		return Future<FileHandle>::createFailed(Exception(tr("URL scheme '%1' not supported. The program supports only the sftp:// scheme and local file paths.").arg(url.scheme()), taskManager.datasetContainer()));
	}
}

/******************************************************************************
* Lists all files in a remote directory.
******************************************************************************/
Future<QStringList> FileManager::listDirectoryContents(TaskManager& taskManager, const QUrl& url)
{
	if(url.scheme() == QStringLiteral("sftp")) {
#ifdef OVITO_SSH_CLIENT
		ListRemoteDirectoryJob* job = new ListRemoteDirectoryJob(url, taskManager);
		return job->future();
#else
		return Future<QStringList>::createFailed(Exception(tr("URL scheme not supported. This version fo OVITO was built without support for the sftp:// protocol and can open local files only."), taskManager.datasetContainer()));
#endif
	}
	else {
		return Future<QStringList>::createFailed(Exception(tr("URL scheme '%1' not supported. The program supports only the sftp:// scheme and local file paths.").arg(url.scheme()), taskManager.datasetContainer()));
	}
}

/******************************************************************************
* Removes a cached remote file so that it will be downloaded again next
* time it is requested.
******************************************************************************/
void FileManager::removeFromCache(const QUrl& url)
{
	QMutexLocker lock(&_mutex);
	_downloadedFiles.remove(normalizeUrl(url));
}

/******************************************************************************
* Is called when a remote file has been fetched.
******************************************************************************/
void FileManager::fileFetched(QUrl url, QTemporaryFile* localFile)
{
	QUrl normalizedUrl = normalizeUrl(std::move(url));
	QMutexLocker lock(&mutex());

	auto inProgressEntry = _pendingFiles.find(normalizedUrl);
	if(inProgressEntry != _pendingFiles.end())
		_pendingFiles.erase(inProgressEntry);

	if(localFile) {
		// Store downloaded file in local cache.
		OVITO_ASSERT(localFile->thread() == this->thread());
		localFile->setParent(this);
		if(!_downloadedFiles.insert(normalizedUrl, localFile, 0))
			throw Exception(tr("Failed to insert downloaded file into file cache."));
	}
}

/******************************************************************************
* Constructs a URL from a path entered by the user.
******************************************************************************/
QUrl FileManager::urlFromUserInput(const QString& path)
{
	if(path.isEmpty())
		return QUrl();
	else if(path.startsWith(QStringLiteral("sftp://")))
		return QUrl(path);
	else
		return QUrl::fromLocalFile(path);
}

#ifdef OVITO_SSH_CLIENT
/******************************************************************************
* Create a new SSH connection or returns an existing connection having the same parameters.
******************************************************************************/
SshConnection* FileManager::acquireSshConnection(const SshConnectionParameters& sshParams)
{
    OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    // Check in-use connections:
    for(SshConnection* connection : _acquiredConnections) {
        if(connection->connectionParameters() != sshParams)
            continue;

        _acquiredConnections.append(connection);
        return connection;
    }

    // Check cached open connections:
    for(SshConnection* connection : _unacquiredConnections) {
        if(!connection->isConnected() || connection->connectionParameters() != sshParams)
            continue;

        _unacquiredConnections.removeOne(connection);
        _acquiredConnections.append(connection);
        return connection;
    }

    // Create a new connection:
    SshConnection* const connection = new SshConnection(sshParams);
    connect(connection, &SshConnection::disconnected, this, &FileManager::cleanupSshConnection);
    connect(connection, &SshConnection::unknownHost, this, &FileManager::unknownSshServer);
	connect(connection, &SshConnection::needPassword, this, &FileManager::needSshPassword);
	connect(connection, &SshConnection::needKbiAnswers, this, &FileManager::needKbiAnswers);
	connect(connection, &SshConnection::authFailed, this, &FileManager::sshAuthenticationFailed);
	connect(connection, &SshConnection::needPassphrase, this, &FileManager::needSshPassphrase);
    _acquiredConnections.append(connection);

    return connection;
}

/******************************************************************************
* Releases an SSH connection after it is no longer used.
******************************************************************************/
void FileManager::releaseSshConnection(SshConnection* connection)
{
    OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    bool wasAquired = _acquiredConnections.removeOne(connection);
    OVITO_ASSERT(wasAquired);
    if(_acquiredConnections.contains(connection))
        return;

    if(!connection->isConnected()) {
        disconnect(connection, nullptr, this, nullptr);
        connection->deleteLater();
    }
    else {
        Q_ASSERT(!_unacquiredConnections.contains(connection));
        _unacquiredConnections.append(connection);
    }
}

/******************************************************************************
*  Is called whenever an SSH connection is closed.
******************************************************************************/
void FileManager::cleanupSshConnection()
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

    if(_unacquiredConnections.removeOne(connection)) {
        disconnect(connection, nullptr, this, nullptr);
        connection->deleteLater();
    }
}

/******************************************************************************
* Is called whenever a SSH connection to an yet unknown server is being established.
******************************************************************************/
void FileManager::unknownSshServer()
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

	if(detectedUnknownSshServer(connection->hostname(), connection->unknownHostMessage(), connection->hostPublicKeyHash())) {
		if(connection->markCurrentHostKnown())
			return;
	}
	connection->cancel();
}

/******************************************************************************
* Informs the user about an unknown SSH host.
******************************************************************************/
bool FileManager::detectedUnknownSshServer(const QString& hostname, const QString& unknownHostMessage, const QString& hostPublicKeyHash)
{
	std::cout << "OVITO is connecting to remote host '" << qPrintable(hostname) << "' via SSH." << std::endl;
	std::cout << qPrintable(unknownHostMessage) << std::endl;
	std::cout << "Host key fingerprint is " << qPrintable(hostPublicKeyHash) << std::endl;
	std::cout << "Are you sure you want to continue connecting (yes/no)? " << std::flush;
	std::string reply;
	std::cin >> reply;
	return reply == "yes";
}

/******************************************************************************
* Is called when an authentication attempt for a SSH connection failed.
******************************************************************************/
void FileManager::sshAuthenticationFailed(int auth)
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

	SshConnection::AuthMethods supported = connection->supportedAuthMethods();
	if(auth & SshConnection::UseAuthPassword && supported & SshConnection::AuthMethodPassword) {
        connection->usePasswordAuth(true);
	}
	else if(auth & SshConnection::UseAuthKbi && supported & SshConnection::AuthMethodKbi) {
        connection->useKbiAuth(true);
	}
}

/******************************************************************************
* Is called whenever a SSH connection to a server requires password authentication.
******************************************************************************/
void FileManager::needSshPassword()
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

	QString password = connection->password();
	if(askUserForPassword(connection->hostname(), connection->username(), password)) {
		connection->setPassword(password);
	}
	else {
		connection->cancel();
	}
}

/******************************************************************************
* Is called whenever a SSH connection to a server requires keyboard interactive authentication.
******************************************************************************/
void FileManager::needKbiAnswers()
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

	QStringList answers;
	for(const SshConnection::KbiQuestion& question : connection->kbiQuestions()) {
		QString answer;
		if(askUserForKbiResponse(connection->hostname(), connection->username(), question.instruction, question.question, question.showAnswer, answer)) {
			answers << answer;
		}
		else {
			connection->cancel();
			return;
		}
	}
	connection->setKbiAnswers(std::move(answers));
}

/******************************************************************************
* Asks the user for the login password for a SSH server.
******************************************************************************/
bool FileManager::askUserForPassword(const QString& hostname, const QString& username, QString& password)
{
	std::string pw;
	std::cout << "Please enter the password for user '" << qPrintable(username) << "' ";
	std::cout << "on SSH remote host '" << qPrintable(hostname) << "' (set echo off beforehand!): " << std::flush;
	std::cin >> pw;
	password = QString::fromStdString(pw);
	return true;
}

/******************************************************************************
* Asks the user for the answer to a keyboard-interactive question sent by the SSH server.
******************************************************************************/
bool FileManager::askUserForKbiResponse(const QString& hostname, const QString& username, const QString& instruction, const QString& question, bool showAnswer, QString& answer)
{
	std::cout << "SSH keyboard interactive authentication";
	if(!showAnswer)
		std::cout << " (set echo off beforehand!)";
	std::cout << " - " << qPrintable(question) << std::flush;
	std::string pw;
	std::cin >> pw;
	answer = QString::fromStdString(pw);
	return true;
}

/******************************************************************************
* Is called whenever a private SSH key requires a passphrase.
******************************************************************************/
void FileManager::needSshPassphrase(const QString& prompt)
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

	QString passphrase;
	if(askUserForKeyPassphrase(connection->hostname(), prompt, passphrase)) {
		connection->setPassphrase(passphrase);
	}
}

/******************************************************************************
* Asks the user for the passphrase for a private SSH key.
******************************************************************************/
bool FileManager::askUserForKeyPassphrase(const QString& hostname, const QString& prompt, QString& passphrase)
{
	std::string pp;
	std::cout << qPrintable(prompt) << std::flush;
	std::cin >> pp;
	passphrase = QString::fromStdString(pp);
	return true;
}
#endif

}	// End of namespace
