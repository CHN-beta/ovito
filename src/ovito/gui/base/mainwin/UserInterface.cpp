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

#include <ovito/gui/base/GUIBase.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/rendering/FrameBuffer.h>
#include "UserInterface.h"

#include <QOperatingSystemVersion>

namespace Ovito {

/******************************************************************************
* Creates a frame buffer of the requested size and displays it as a window in the user interface.
******************************************************************************/
std::shared_ptr<FrameBuffer> UserInterface::createAndShowFrameBuffer(int width, int height) 
{ 
	return std::make_shared<FrameBuffer>(width, height);
}

/******************************************************************************
* Queries the system's information and graphics capabilities.
******************************************************************************/
QString UserInterface::generateSystemReport()
{
	QString text;
	QTextStream stream(&text, QIODevice::WriteOnly | QIODevice::Text);
	stream << "======= System info =======\n";
	stream << "Current date: " << QDateTime::currentDateTime().toString() << "\n";
	stream << "Application: " << Application::applicationName() << " " << Application::applicationVersionString() << "\n";
	stream << "Operating system: " <<  QOperatingSystemVersion::current().name() << " (" << QOperatingSystemVersion::current().majorVersion() << "." << QOperatingSystemVersion::current().minorVersion() << ")" << "\n";
#if defined(Q_OS_LINUX)
	// Get 'uname' output.
	QProcess unameProcess;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	unameProcess.start("uname", QStringList() << "-m" << "-i" << "-o" << "-r" << "-v", QIODevice::ReadOnly);
#else
	unameProcess.start("uname -m -i -o -r -v", QIODevice::ReadOnly);
#endif
	unameProcess.waitForFinished();
	QByteArray unameOutput = unameProcess.readAllStandardOutput();
	unameOutput.replace('\n', ' ');
	stream << "uname output: " << unameOutput << "\n";
	// Get 'lsb_release' output.
	QProcess lsbProcess;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	lsbProcess.start("lsb_release", QStringList() << "-s" << "-i" << "-d" << "-r", QIODevice::ReadOnly);
#else
	lsbProcess.start("lsb_release -s -i -d -r", QIODevice::ReadOnly);
#endif
	lsbProcess.waitForFinished();
	QByteArray lsbOutput = lsbProcess.readAllStandardOutput();
	lsbOutput.replace('\n', ' ');
	stream << "LSB output: " << lsbOutput << "\n";
#endif
	stream << "Processor architecture: " << QSysInfo::currentCpuArchitecture() << "\n";
	stream << "Floating-point type: " << (sizeof(FloatType)*8) << "-bit" << "\n";
	stream << "Qt version: " << QT_VERSION_STR << " (" << QSysInfo::buildCpuArchitecture() << ")\n";
#ifdef OVITO_DISABLE_THREADING
	stream << "Multi-threading: disabled\n";
#endif
	stream << "Command line: " << QCoreApplication::arguments().join(' ') << "\n";
	// Let the plugin class add their information to their system report.
	for(Plugin* plugin : PluginManager::instance().plugins()) {
		for(OvitoClassPtr clazz : plugin->classes()) {
			clazz->querySystemInformation(stream, datasetContainer());
		}
	}
	return text;
}

/******************************************************************************
* Shows the online manual and opens the given help page.
******************************************************************************/
void UserInterface::openHelpTopic(const QString& helpTopicId)
{
	// Determine the filesystem path where OVITO's documentation files are installed.
#ifndef Q_OS_WASM
	QDir prefixDir(QCoreApplication::applicationDirPath());
	QDir helpDir = QDir(prefixDir.absolutePath() + QChar('/') + QStringLiteral(OVITO_DOCUMENTATION_PATH));	
	QUrl url;
#else
	QDir helpDir(QStringLiteral(":/doc/manual/"));
	QUrl baseUrl(QStringLiteral("https://docs.ovito.org/"));
	QUrl url = baseUrl;
#endif

	// Resolve the help topic ID.
	if(helpTopicId.endsWith(".html") || helpTopicId.contains(".html#")) {
		// If a HTML file name has been specified, open it directly.
		url = QUrl::fromLocalFile(helpDir.absoluteFilePath(helpTopicId));
	}
	else if(helpTopicId.startsWith("manual:")) {
		// If a Sphinx link target has been specified, resolve it to a HTML file path using the
		// Intersphinx inventory. The file 'objects.txt' is generated by the script 'ovito/doc/manual/CMakeLists.txt'
		// and gets distributed together with the application.
		QFile inventoryFile(helpDir.absoluteFilePath("objects.txt"));
		if(!inventoryFile.open(QIODevice::ReadOnly | QIODevice::Text))
			qWarning() << "WARNING: Could not open Intersphinx inventory file to resolve help topic reference:" << inventoryFile.fileName() << inventoryFile.errorString();			
		else {
			QTextStream stream(&inventoryFile);
			// Skip file until to the line "std:label":
			while(!stream.atEnd()) {
				QString line = stream.readLine();
				if(line.startsWith("std:label"))
					break;
			}
			// Now parse the link target list.
			QString searchString = QChar('\t') + helpTopicId.mid(7) + QChar(' ');
			while(!stream.atEnd()) {
				QString line = stream.readLine();
				if(line.startsWith(searchString)) {
					int startIndex = line.lastIndexOf(QChar(' '));
					QString filePath = line.mid(startIndex + 1).trimmed();
					QString anchor;
					int anchorIndex = filePath.indexOf(QChar('#'));
					if(anchorIndex >= 0) {
						anchor = filePath.mid(anchorIndex + 1);
						filePath.truncate(anchorIndex);
					}
#ifndef Q_OS_WASM
					url = QUrl::fromLocalFile(helpDir.absoluteFilePath(filePath));
#else
					url.setPath(QChar('/') + filePath);
#endif
					url.setFragment(anchor);
					break;
				}
			}
			OVITO_ASSERT(!url.isEmpty());
		}
	}
	
#ifndef Q_OS_WASM
	if(url.isEmpty()) {
		// If no help topic has been specified, open the main index page of the user manual.
		url = QUrl::fromLocalFile(helpDir.absoluteFilePath(QStringLiteral("index.html")));
	}
#endif

	// Workaround for a limitation of the Microsoft Edge browser:
	// The browser drops any # fragment in local URLs to be opened, thus making it difficult to reference sub-topics within a HTML help page.
	// Solution is to generate a temporary HTML file which redirects to the actual help page including the # fragment.
	// See also https://forums.madcapsoftware.com/viewtopic.php?f=9&t=28376#p130613
	// and https://stackoverflow.com/questions/26305322/shellexecute-fails-for-local-html-or-file-urls
#ifdef Q_OS_WIN
	if(url.isLocalFile() && url.hasFragment()) {
		static QTemporaryFile* temporaryHtmlFile = nullptr;
		if(temporaryHtmlFile) delete temporaryHtmlFile;
		temporaryHtmlFile = new QTemporaryFile(QDir::temp().absoluteFilePath(QStringLiteral("ovito-help-XXXXXX.html")), qApp);
		if(temporaryHtmlFile->open()) {
			// Write a small HTML file that just contains a redirect directive to the actual help page including the # fragment.
			QTextStream(temporaryHtmlFile) << QStringLiteral("<html><meta http-equiv=Refresh content=\"0; url=%1\"><body></body></html>").arg(url.toString(QUrl::FullyEncoded));
			temporaryHtmlFile->close();
			// Let the web brwoser ppen the redirect page instead of the original help page. 
			url = QUrl::fromLocalFile(temporaryHtmlFile->fileName());
		}
	}
#endif

	// Use the local web browser to display the help page.
	if(!QDesktopServices::openUrl(url)) {
		Exception(QStringLiteral("Could not launch browser to display manual. The requested URL is:\n%1").arg(url.toDisplayString())).reportError();
	}
}

}	// End of namespace
