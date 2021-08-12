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

#include <ovito/gui/base/GUIBase.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/app/PluginManager.h>
#include "MainWindowInterface.h"

#include <QOperatingSystemVersion>

namespace Ovito {

/******************************************************************************
* Queries the system's information and graphics capabilities.
******************************************************************************/
QString MainWindowInterface::generateSystemReport()
{
	QString text;
	QTextStream stream(&text, QIODevice::WriteOnly | QIODevice::Text);
	stream << "======= System info =======\n";
	stream << "Date: " << QDateTime::currentDateTime().toString() << "\n";
	stream << "Application: " << Application::applicationName() << " " << Application::applicationVersionString() << "\n";
	stream << "Operating system: " <<  QOperatingSystemVersion::current().name() << " (" << QOperatingSystemVersion::current().majorVersion() << "." << QOperatingSystemVersion::current().minorVersion() << ")" << "\n";
#if defined(Q_OS_LINUX)
	// Get 'uname' output.
	QProcess unameProcess;
	unameProcess.start("uname -m -i -o -r -v", QIODevice::ReadOnly);
	unameProcess.waitForFinished();
	QByteArray unameOutput = unameProcess.readAllStandardOutput();
	unameOutput.replace('\n', ' ');
	stream << "uname output: " << unameOutput << "\n";
	// Get 'lsb_release' output.
	QProcess lsbProcess;
	lsbProcess.start("lsb_release -s -i -d -r", QIODevice::ReadOnly);
	lsbProcess.waitForFinished();
	QByteArray lsbOutput = lsbProcess.readAllStandardOutput();
	lsbOutput.replace('\n', ' ');
	stream << "LSB output: " << lsbOutput << "\n";
#endif
	stream << "Processor architecture: " << (QT_POINTER_SIZE*8) << "-bit" << "\n";
	stream << "Floating-point type: " << (sizeof(FloatType)*8) << "-bit" << "\n";
	stream << "Qt framework version: " << QT_VERSION_STR << "\n";
	stream << "Command line: " << QCoreApplication::arguments().join(' ') << "\n";
	// Let the plugin class add their information to their system report.
	for(Plugin* plugin : PluginManager::instance().plugins()) {
		for(OvitoClassPtr clazz : plugin->classes()) {
			clazz->querySystemInformation(stream, datasetContainer());
		}
	}
	return text;
}

}	// End of namespace
