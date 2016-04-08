///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#ifndef __OVITO_PYTHON_VIEWPORT_OVERLAY_H
#define __OVITO_PYTHON_VIEWPORT_OVERLAY_H

#include <plugins/pyscript/PyScript.h>
#include <gui/properties/PropertiesEditor.h>
#include <core/viewport/overlay/ViewportOverlay.h>
#include <plugins/pyscript/engine/ScriptEngine.h>

class QsciScintilla;

namespace PyScript {

using namespace Ovito;

/**
 * \brief A viewport overlay that runs a Python script which paints into the viewport.
 */
class OVITO_PYSCRIPT_EXPORT PythonViewportOverlay : public ViewportOverlay
{
public:

	/// \brief Constructor.
	Q_INVOKABLE PythonViewportOverlay(DataSet* dataset);

	/// \brief This method asks the overlay to paint its contents over the given viewport.
	virtual void render(Viewport* viewport, QPainter& painter, const ViewProjectionParameters& projParams, RenderSettings* renderSettings) override;

	/// Returns the Python script that renders the overlay.
	const QString& script() const { return _script; }

	/// Sets the Python script that renders the overlay.
	void setScript(const QString& script) { _script = script; }

	/// Returns the output generated by the script.
	const QString& scriptOutput() const { return _scriptOutput; }

	/// Returns the Python script function executed by the overlay.
	boost::python::object scriptFunction() {
		return _overlayScriptFunction;
	}

	/// Sets the Python script function to be executed by the overlay.
	void setScriptFunction(const boost::python::object& func) {
		_overlayScriptFunction = func;
		notifyDependents(ReferenceEvent::TargetChanged);
	}

	/// Returns whether the script was successfully compiled.
	bool compilationSucessful() const {
		return !_overlayScriptFunction.is_none();
	}

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// This method is called once for this object after they have been completely loaded from a stream.
	virtual void loadFromStreamComplete() override {
		ViewportOverlay::loadFromStreamComplete();
		compileScript();
	}

private Q_SLOTS:

	/// Is called when the script generates some output.
	void onScriptOutput(const QString& text);

private:

	/// Compiles the script entered by the user.
	void compileScript();

	/// The Python script.
	PropertyField<QString> _script;

	/// The Python engine.
	ScriptEngine _scriptEngine;

	/// The output generated by the script.
	QString _scriptOutput;

	/// The compiled script function.
	boost::python::object _overlayScriptFunction;

	Q_CLASSINFO("DisplayName", "Python script");

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_PROPERTY_FIELD(_script);
};

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A properties editor for the PythonViewportOverlay class.
 */
class PythonViewportOverlayEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE PythonViewportOverlayEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

protected Q_SLOTS:

	/// Is called when the current edit object has generated a change
	/// event or if a new object has been loaded into editor.
	void onContentsChanged(RefTarget* editObject);

	/// Is called when the user presses the 'Apply' button to commit the Python script.
	void onApplyChanges();

private:

	QsciScintilla* _codeEditor;
	QsciScintilla* _errorDisplay;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE

}	// End of namespace

#endif // __OVITO_PYTHON_VIEWPORT_OVERLAY_H
