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
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/utilities/io/FileManager.h>
#include "Application.h"

#include <QLoggingCategory>
#include <QSurfaceFormat>

// Called from Application::initialize() to register the embedded Qt resource files
// when running a statically linked executable. Following the Qt documentation, this
// needs to be placed outside of any C++ namespace.
static void registerQtResources()
{
#ifdef OVITO_BUILD_MONOLITHIC
	Q_INIT_RESOURCE(core);
	Q_INIT_RESOURCE(opengl);
	#if defined(OVITO_BUILD_GUI)
		Q_INIT_RESOURCE(guibase);
		Q_INIT_RESOURCE(gui);
		#ifdef OVITO_QML_GUI
			Q_INIT_RESOURCE(stdobjgui);
			Q_INIT_RESOURCE(stdmodgui);
			Q_INIT_RESOURCE(particlesgui);
		#endif
	#endif
#endif
}

namespace Ovito {

/// The one and only instance of this class.
Application* Application::_instance = nullptr;

/// Stores a pointer to the original Qt message handler function, which has been replaced with our own handler.
QtMessageHandler Application::defaultQtMessageHandler = nullptr;

/******************************************************************************
* Handler method for Qt log messages.
* This can be used to set a debugger breakpoint for the OVITO_ASSERT macros.
******************************************************************************/
void Application::qtMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	// Forward message to default handler.
	if(defaultQtMessageHandler) defaultQtMessageHandler(type, context, msg);
	else std::cerr << qPrintable(qFormatLogMessage(type, context, msg)) << std::endl;
}

/******************************************************************************
* Handler method for Qt log messages that should be redirected to a file.
******************************************************************************/
static void qtMessageLogFile(QtMsgType type, const QMessageLogContext& context, const QString& msg) 
{
	// Format the message string to be written to the log file.
	QString formattedMsg = qFormatLogMessage(type, context, msg);

	// The log file object.
	static QFile logFile(QDir::fromNativeSeparators(qEnvironmentVariable("OVITO_LOG_FILE", QStringLiteral("ovito.log"))));
	
	// Synchronize concurrent access to the log file. 
	static QMutex ioMutex;
	QMutexLocker mutexLocker(&ioMutex);

	// Open the log file for writing if it is not open yet.
	if(!logFile.isOpen()) {
		if(!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			std::cerr << "WARNING: Failed to open log file '" << qPrintable(logFile.fileName()) << "' for writing: ";
			std::cerr << qPrintable(logFile.errorString()) << std::endl;
			Application::qtMessageOutput(type, context, msg);
			return;
		}
	}

	// Write to the text stream.
	static QTextStream stream(&logFile);
	stream << formattedMsg << '\n';
	stream.flush();
}

/******************************************************************************
* Constructor.
******************************************************************************/
Application::Application()
{
	// Set global application pointer.
	OVITO_ASSERT(_instance == nullptr);	// Only allowed to create one Application class instance.
	_instance = this;

	// Use all available processor cores by default, or the user-specified
	// number given by the OVITO_THREAD_COUNT environment variable.
	_idealThreadCount = std::max(1, QThread::idealThreadCount());
	if(qEnvironmentVariableIsSet("OVITO_THREAD_COUNT"))
		_idealThreadCount = std::max(1, qgetenv("OVITO_THREAD_COUNT").toInt());
}

/******************************************************************************
* Destructor.
******************************************************************************/
Application::~Application()
{
	_instance = nullptr;
}

/******************************************************************************
* Returns the major version number of the application.
******************************************************************************/
int Application::applicationVersionMajor()
{
	// This compile-time constant is defined by the CMake build script.
	return OVITO_VERSION_MAJOR;
}

/******************************************************************************
* Returns the minor version number of the application.
******************************************************************************/
int Application::applicationVersionMinor()
{
	// This compile-time constant is defined by the CMake build script.
	return OVITO_VERSION_MINOR;
}

/******************************************************************************
* Returns the revision version number of the application.
******************************************************************************/
int Application::applicationVersionRevision()
{
	// This compile-time constant is defined by the CMake build script.
	return OVITO_VERSION_REVISION;
}

/******************************************************************************
* Returns the complete version string of the application release.
******************************************************************************/
QString Application::applicationVersionString()
{
	// This compile-time constant is defined by the CMake build script.
	return QStringLiteral(OVITO_VERSION_STRING);
}

/******************************************************************************
* Returns the human-readable name of the application.
******************************************************************************/
QString Application::applicationName()
{
	// This compile-time constant is defined by the CMake build script.
	return QStringLiteral(OVITO_APPLICATION_NAME);
}

/******************************************************************************
* This is called on program startup.
******************************************************************************/
bool Application::initialize()
{
	// Install custom Qt error message handler to catch fatal errors in debug mode
	// or redirect log output to file instead of the console if requested by the user.
	if(qEnvironmentVariableIsSet("OVITO_LOG_FILE")) {
		// Install a message handler that writes log output to a text file. 
		defaultQtMessageHandler = qInstallMessageHandler(qtMessageLogFile);
		// QDebugStateSaver saver(qInfo());
		qInfo().noquote() << "#" << applicationName() << applicationVersionString() << "started on" << QDateTime::currentDateTime().toString();
	}
	else {
		// Install message handler that forwards to the default Qt handler or writes to stderr stream.
		defaultQtMessageHandler = qInstallMessageHandler(qtMessageOutput);
	}

#ifdef OVITO_DEBUG
	// Activate Qt logging messages related to Vulkan.
	QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
#endif

	// Activate default "C" locale, which will be used to parse numbers in strings.
	std::setlocale(LC_ALL, "C");

	// Suppress console messages "qt.network.ssl: QSslSocket: cannot resolve ..."
	qputenv("QT_LOGGING_RULES", "qt.network.ssl.warning=false");

	// Register our floating-point data type with the Qt type system.
	qRegisterMetaType<FloatType>("FloatType");

	// Register generic object reference type with the Qt type system.
	qRegisterMetaType<OORef<OvitoObject>>("OORef<OvitoObject>");

	// Register Qt stream operators for basic types.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	qRegisterMetaTypeStreamOperators<Vector2>("Ovito::Vector2");
	qRegisterMetaTypeStreamOperators<Vector3>("Ovito::Vector3");
	qRegisterMetaTypeStreamOperators<Vector4>("Ovito::Vector4");
	qRegisterMetaTypeStreamOperators<Point2>("Ovito::Point2");
	qRegisterMetaTypeStreamOperators<Point3>("Ovito::Point3");
	qRegisterMetaTypeStreamOperators<AffineTransformation>("Ovito::AffineTransformation");
	qRegisterMetaTypeStreamOperators<Matrix3>("Ovito::Matrix3");
	qRegisterMetaTypeStreamOperators<Matrix4>("Ovito::Matrix4");
	qRegisterMetaTypeStreamOperators<Box2>("Ovito::Box2");
	qRegisterMetaTypeStreamOperators<Box3>("Ovito::Box3");
	qRegisterMetaTypeStreamOperators<Rotation>("Ovito::Rotation");
	qRegisterMetaTypeStreamOperators<Scaling>("Ovito::Scaling");
	qRegisterMetaTypeStreamOperators<Quaternion>("Ovito::Quaternion");
	qRegisterMetaTypeStreamOperators<Color>("Ovito::Color");
	qRegisterMetaTypeStreamOperators<ColorA>("Ovito::ColorA");
#endif

	// Register Qt conversion operators for custom types.
	QMetaType::registerConverter<QColor, Color>();
	QMetaType::registerConverter<Color, QColor>();
	QMetaType::registerConverter<QColor, ColorA>();
	QMetaType::registerConverter<ColorA, QColor>();
	QMetaType::registerConverter<Vector2, QVector2D>(&Vector2::operator QVector2D);
	QMetaType::registerConverter<QVector2D, Vector2>();
	QMetaType::registerConverter<Vector3, QVector3D>(&Vector3::operator QVector3D);
	QMetaType::registerConverter<QVector3D, Vector3>();
	QMetaType::registerConverter<Color, Vector3>();
	QMetaType::registerConverter<Vector3, Color>();
	QMetaType::registerConverter<QVector3D, Color>();
	QMetaType::registerConverter<Color, QVector3D>(&Color::operator QVector3D);

	// Enable OpenGL context sharing globally.
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#if 1
	// Always use desktop OpenGL (avoid ANGLE on Windows):
	QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#else
	// Use ANGLE OpenGL-to-DirectX translation layer on Windows:
	QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif

	// Specify default OpenGL surface format.
	QSurfaceFormat format;
#ifndef Q_OS_WASM
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(1);
#ifdef Q_OS_MACOS
	// macOS only supports core profile contexts.
	format.setMajorVersion(3);
	format.setMinorVersion(2);
	format.setProfile(QSurfaceFormat::CoreProfile);
#endif
#else
	// When running in a web browser, try to request a context that supports OpenGL ES 2.0 (WebGL 1).
	format.setMajorVersion(2);
	format.setMinorVersion(0);
#endif
	QSurfaceFormat::setDefaultFormat(format);

	// Register Qt resources.
	::registerQtResources();

	// Create global FileManager object.
	_fileManager.reset(createFileManager());

	return true;
}

/******************************************************************************
* Create the global instance of the right QCoreApplication derived class.
******************************************************************************/
void Application::createQtApplication(int& argc, char** argv)
{
	// OVITO prefers the "C" locale over the system's default locale.
	QLocale::setDefault(QLocale::c());

	if(headlessMode()) {
#if defined(Q_OS_LINUX)
		// Determine font directory path.
		std::string applicationPath = argv[0];
		auto sepIndex = applicationPath.rfind('/');
		if(sepIndex != std::string::npos)
			applicationPath.resize(sepIndex + 1);
		std::string fontPath = applicationPath + "../share/ovito/fonts";
		if(!QDir(QString::fromStdString(fontPath)).exists())
			fontPath = "/usr/share/fonts";

		// On Linux, use the 'minimal' QPA platform plugin instead of the standard XCB plugin when no X server is available.
		// Still create a Qt GUI application object, because otherwise we cannot use (offscreen) font rendering functions.
		qputenv("QT_QPA_PLATFORM", "minimal");
		// Enable rudimentary font rendering support, which is implemented by the 'minimal' platform plugin:
		qputenv("QT_DEBUG_BACKINGSTORE", "1");
		qputenv("QT_QPA_FONTDIR", fontPath.c_str());

		new QGuiApplication(argc, argv);
#elif defined(Q_OS_MAC)
		new QGuiApplication(argc, argv);
#else
		new QCoreApplication(argc, argv);
#endif
	}
	else {
		new QGuiApplication(argc, argv);
	}
}

/******************************************************************************
* Returns a pointer to the main dataset container.
******************************************************************************/
DataSetContainer* Application::datasetContainer() const
{
	return _datasetContainer;
}

/******************************************************************************
* Creates the global FileManager class instance.
******************************************************************************/
FileManager* Application::createFileManager()
{
	return new FileManager();
}

/******************************************************************************
* Handler function for exceptions.
******************************************************************************/
void Application::reportError(const Exception& exception, bool /*blocking*/)
{
	for(int i = exception.messages().size() - 1; i >= 0; i--) {
		qInfo().noquote() << "ERROR:" << exception.messages()[i];
	}
}

#ifndef Q_OS_WASM
/******************************************************************************
* Returns the application-wide network manager object.
******************************************************************************/
QNetworkAccessManager* Application::networkAccessManager()
{
	if(!_networkAccessManager)
		_networkAccessManager = new QNetworkAccessManager(this);
	return _networkAccessManager;
}
#endif

}	// End of namespace
