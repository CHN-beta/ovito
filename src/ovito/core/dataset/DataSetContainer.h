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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/oo/RefMaker.h>

namespace Ovito {

class MainWindowInterface; // defined in MainWindowInterface.h

/**
 * \brief Manages the DataSet being edited.
 */
class OVITO_CORE_EXPORT DataSetContainer : public RefMaker
{
	Q_OBJECT
	OVITO_CLASS(DataSetContainer)

#ifdef OVITO_QML_GUI
	Q_PROPERTY(Ovito::DataSet* currentSet READ currentSet WRITE setCurrentSet NOTIFY dataSetChanged)
#endif

public:

	/// \brief Constructor.
	DataSetContainer();

	/// \brief Destructor.
	virtual ~DataSetContainer();

	/// \brief Returns the manager of background tasks.
	/// \return Reference to the task manager, which is part of this dataset manager.
	///
	/// Use the task manager to start and control background jobs.
	TaskManager& taskManager() { return _taskManager; }

	/// \brief Returns the graphical user interface this dataset container is associated with.
	virtual UserInterface* guiInterface() { return nullptr; }

	/// \brief Creates an empty dataset and makes it the current dataset.
	/// \return \c true if the operation was completed; \c false if the operation has been canceled by the user.
	/// \throw Exception on error.
	bool newDataset();

	/// \brief Loads the given session state file and makes it the current dataset.
	/// \return \c true if the file has been successfully loaded; \c false if the operation has been canceled by the user.
	/// \throw Exception on error.
	bool loadDataset(const QString& filename);

Q_SIGNALS:

	/// Is emitted when a another dataset has become the active dataset.
	void dataSetChanged(DataSet* newDataSet);

	/// \brief Is emitted when nodes have been added or removed from the current selection set.
	/// \param selection The current selection set.
	/// \note This signal is NOT emitted when a node in the selection set has changed.
	/// \note In contrast to the selectionChangeComplete() signal this signal is emitted
	///       for every node that is added to or removed from the selection set. That is,
	///       a call to SelectionSet::addAll() for example will generate multiple selectionChanged()
	///       events but only a single selectionChangeComplete() event.
    void selectionChanged(SelectionSet* selection);

	/// \brief This signal is emitted after all changes to the selection set have been completed.
	/// \param selection The current selection set.
	/// \note This signal is NOT emitted when a node in the selection set has changed.
	/// \note In contrast to the selectionChange() signal this signal is emitted
	///       only once after the selection set has been changed. That is,
	///       a call to SelectionSet::addAll() for example will generate multiple selectionChanged()
	///       events but only a single selectionChangeComplete() event.
    void selectionChangeComplete(SelectionSet* selection);

	/// \brief This signal is emitted whenever the current selection set has been replaced by another one.
	/// \note This signal is NOT emitted when nodes are added or removed from the current selection set.
    void selectionSetReplaced(SelectionSet* newSelectionSet);

	/// \brief This signal is emitted whenever the current viewport configuration of current dataset has been replaced by a new one.
	/// \note This signal is NOT emitted when the parameters of the current viewport configuration change.
    void viewportConfigReplaced(ViewportConfiguration* newViewportConfiguration);

	/// \brief This signal is emitted whenever the current animation settings of the current dataset have been replaced by new ones.
	/// \note This signal is NOT emitted when the parameters of the current animation settings object change.
    void animationSettingsReplaced(AnimationSettings* newAnimationSettings);

	/// \brief This signal is emitted whenever the current render settings of this dataset
	///        have been replaced by new ones.
	/// \note This signal is NOT emitted when parameters of the current render settings object change.
    void renderSettingsReplaced(RenderSettings* newRenderSettings);

	/// \brief This signal is emitted when the current animation time has changed or if the current animation settings have been replaced.
    void timeChanged(TimePoint newTime);

	/// \brief This signal is emitted when the scene becomes ready after the current animation time has changed.
	void timeChangeComplete();

	/// \brief This signal is emitted whenever the file path of the active dataset changes.
	void filePathChanged(const QString& filePath);

	/// \brief This signal is emitted whenever the modification status (clean state) of the active dataset changes.
	void modificationStatusChanged(bool isClean);

	/// \brief Is emitted whenever the scene of the current dataset has been changed and is being made ready for rendering.
	void scenePreparationBegin();

	/// \brief Is emitted whenever the scene of the current dataset became ready for rendering.
	void scenePreparationEnd();

protected:

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor* field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex) override;

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

protected Q_SLOTS:

	/// This handler is invoked when the current selection set of the current dataset has been replaced.
    void onSelectionSetReplaced(SelectionSet* newSelectionSet);

	/// This handler is invoked when the current animation settings of the current dataset have been replaced.
    void onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings);

private:

	/// The current dataset being edited by the user.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<DataSet>, currentSet, setCurrentSet, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The list of running compute tasks.
	TaskManager _taskManager;

	/// Is called when scene of the current dataset is ready to be displayed.
	void sceneBecameReady();

	/// Indicates whether we are already waiting for the scene to become ready.
	bool _sceneReadyScheduled = false;

	/// The task that makes the scene ready for interactive rendering in the viewports.
	SharedFuture<> _sceneReadyFuture;

	QMetaObject::Connection _selectionSetReplacedConnection;
	QMetaObject::Connection _selectionSetChangedConnection;
	QMetaObject::Connection _selectionSetChangeCompleteConnection;
	QMetaObject::Connection _viewportConfigReplacedConnection;
	QMetaObject::Connection _animationSettingsReplacedConnection;
	QMetaObject::Connection _renderSettingsReplacedConnection;
	QMetaObject::Connection _animationTimeChangedConnection;
	QMetaObject::Connection _animationTimeChangeCompleteConnection;
	QMetaObject::Connection _undoStackCleanChangedConnection;
	QMetaObject::Connection _filePathChangedConnection;
};

}	// End of namespace
