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
#include <ovito/core/oo/RefMaker.h>
#include <ovito/core/dataset/DataSet.h>
#include "UndoStack.h"
#include "UndoStackOperations.h"

namespace Ovito {

/******************************************************************************
* Increments the suspend count of the undo stack associated with the given
* object.
******************************************************************************/
UndoSuspender::UndoSuspender(const RefMaker* object) noexcept
{
	OVITO_CHECK_OBJECT_POINTER(object);
	if(object->dataset() && QThread::currentThread() == object->thread()) {
		_suspendCount = &object->dataset()->undoStack()._suspendCount;
		++(*_suspendCount);
	}
	else {
		_suspendCount = nullptr;
	}
}

/******************************************************************************
* Initializes the undo manager.
******************************************************************************/
UndoStack::UndoStack()
{
}

/******************************************************************************
* Registers a single undoable operation.
* This object will be put onto the undo stack.
******************************************************************************/
void UndoStack::push(std::unique_ptr<UndoableOperation> operation)
{
	OVITO_CHECK_POINTER(operation.get());
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "UndoStack::push()", "This function may only be called from the main thread.");
	OVITO_ASSERT_MSG(isUndoingOrRedoing() == false, "UndoStack::push()", "Cannot record an operation while undoing or redoing another operation.");
	OVITO_ASSERT_MSG(!isSuspended(), "UndoStack::push()", "Not in recording state.");

	UndoSuspender noUndo(*this);

	// Discard previously undone operations.
	_operations.resize(index() + 1);
	if(cleanIndex() > index()) _cleanIndex = -1;

	if(_compoundStack.empty()) {
		_operations.push_back(std::move(operation));
		_index++;
		OVITO_ASSERT(index() == count() - 1);
		limitUndoStack();
		Q_EMIT indexChanged(index());
		Q_EMIT cleanChanged(false);
		Q_EMIT canUndoChanged(true);
		Q_EMIT undoTextChanged(undoText());
		Q_EMIT canRedoChanged(false);
		Q_EMIT redoTextChanged(QString());
	}
	else {
		_compoundStack.back()->addOperation(std::move(operation));
	}
}

/******************************************************************************
* Opens a new compound operation and assigns it the given display name.
******************************************************************************/
void UndoStack::beginCompoundOperation(const QString& displayName)
{
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "UndoStack::beginCompoundOperation()", "This function may only be called from the main thread.");
	OVITO_ASSERT_MSG(isUndoingOrRedoing() == false, "UndoStack::beginCompoundOperation()", "Cannot record an operation while undoing or redoing another operation.");
	_compoundStack.push_back(std::make_unique<CompoundOperation>(displayName));
}

/******************************************************************************
* Closes the current compound operation.
******************************************************************************/
void UndoStack::endCompoundOperation(bool commit)
{
	OVITO_ASSERT_MSG(isUndoingOrRedoing() == false, "UndoStack::endCompoundOperation()", "Cannot record an operation while undoing or redoing another operation.");
	OVITO_ASSERT_MSG(!_compoundStack.empty(), "UndoStack::endCompoundOperation()", "Missing call to beginCompoundOperation().");

	if(!commit) {
		// Undo operations in current compound operation first.
		resetCurrentCompoundOperation();
		// Then discard compound operation.
		_compoundStack.pop_back();
	}
	else {

		// Take current compound operation from the macro stack.
		std::unique_ptr<CompoundOperation> cop = std::move(_compoundStack.back());
		_compoundStack.pop_back();

		// Check if the operation should be kept.
		if(_suspendCount > 0 || !cop->isSignificant()) {
			// Discard operation.
			UndoSuspender noUndo(*this);
			cop.reset();
			return;
		}

		// Put new operation on the stack.
		push(std::move(cop));
	}
}

/******************************************************************************
* Undoes all actions of the current compound operation.
******************************************************************************/
void UndoStack::resetCurrentCompoundOperation()
{
	OVITO_ASSERT_MSG(isUndoingOrRedoing() == false, "UndoStack::resetCurrentCompoundOperation()", "Cannot reset operation while undoing or redoing another operation.");
	OVITO_ASSERT_MSG(!_compoundStack.empty(), "UndoStack::resetCurrentCompoundOperation()", "Missing call to beginCompoundOperation().");

	CompoundOperation* cop = _compoundStack.back().get();
	// Undo operations.
	UndoSuspender noUndo(*this);
	_isUndoing = true;
	try {
		cop->undo();
		cop->clear();
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
	_isUndoing = false;
}

/******************************************************************************
* Shrinks the undo stack to maximum number of undo steps.
******************************************************************************/
void UndoStack::limitUndoStack()
{
	if(_undoLimit < 0) return;
	int n = count() - _undoLimit;
	if(n > 0) {
		if(index() >= n) {
			UndoSuspender noUndo(*this);
			_operations.erase(_operations.begin(), _operations.begin() + n);
			_index -= n;
			Q_EMIT indexChanged(index());
		}
	}
}

/******************************************************************************
* Resets the undo system. The undo stack will be cleared.
******************************************************************************/
void UndoStack::clear()
{
	_operations.clear();
	_compoundStack.clear();
	_index = -1;
	_cleanIndex = -1;
	Q_EMIT indexChanged(index());
	Q_EMIT cleanChanged(isClean());
	Q_EMIT canUndoChanged(false);
	Q_EMIT canRedoChanged(false);
	Q_EMIT undoTextChanged(QString());
	Q_EMIT redoTextChanged(QString());
}

/******************************************************************************
* Marks the stack as clean and emits cleanChanged() if the stack was not already clean.
******************************************************************************/
void UndoStack::setClean()
{
	if(!isClean()) {
		_cleanIndex = index();
		Q_EMIT cleanChanged(true);
	}
}

/******************************************************************************
* Marks the stack as dirty and emits cleanChanged() if the stack was not already dirty.
******************************************************************************/
void UndoStack::setDirty()
{
	bool signal = isClean();
	_cleanIndex = -2;
	if(signal)
		Q_EMIT cleanChanged(false);
}

/******************************************************************************
* Undoes the last operation in the undo stack.
******************************************************************************/
void UndoStack::undo()
{
	OVITO_ASSERT(isRecording() == false);
	OVITO_ASSERT(isUndoingOrRedoing() == false);
	OVITO_ASSERT_MSG(_compoundStack.empty(), "UndoStack::undo()", "Cannot undo last operation while a compound operation is open.");
	if(!canUndo()) return;

	UndoableOperation* curOp = _operations[index()].get();
	OVITO_CHECK_POINTER(curOp);
	_isUndoing = true;
	suspend();
	try {
		curOp->undo();
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
	_isUndoing = false;
	resume();
	_index--;
	Q_EMIT indexChanged(index());
	Q_EMIT cleanChanged(isClean());
	Q_EMIT canUndoChanged(canUndo());
	Q_EMIT undoTextChanged(undoText());
	Q_EMIT canRedoChanged(canRedo());
	Q_EMIT redoTextChanged(redoText());
}

/******************************************************************************
* Redoes the last undone operation in the undo stack.
******************************************************************************/
void UndoStack::redo()
{
	OVITO_ASSERT(isRecording() == false);
	OVITO_ASSERT(isUndoingOrRedoing() == false);
	OVITO_ASSERT_MSG(_compoundStack.empty(), "UndoStack::redo()", "Cannot redo operation while a compound operation is open.");
	if(!canRedo()) return;

	UndoableOperation* nextOp = _operations[index() + 1].get();
	OVITO_CHECK_POINTER(nextOp);
	_isRedoing = true;
	suspend();
	try {
		nextOp->redo();
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
	_isRedoing = false;
	resume();
	_index++;
	Q_EMIT indexChanged(index());
	Q_EMIT cleanChanged(isClean());
	Q_EMIT canUndoChanged(canUndo());
	Q_EMIT undoTextChanged(undoText());
	Q_EMIT canRedoChanged(canRedo());
	Q_EMIT redoTextChanged(redoText());
}

/******************************************************************************
* Undo the compound edit operation that was made.
******************************************************************************/
void UndoStack::CompoundOperation::undo()
{
	for(int i = (int)_subOperations.size() - 1; i >= 0; --i) {
		OVITO_CHECK_POINTER(_subOperations[i]);
		_subOperations[i]->undo();
	}
}

/******************************************************************************
* Re-apply the compound change, assuming that it has been undone.
******************************************************************************/
void UndoStack::CompoundOperation::redo()
{
	for(const auto& op : _subOperations) {
		OVITO_CHECK_POINTER(op);
		op->redo();
	}
}

/******************************************************************************
* Prints a text representation of the undo stack to the console.
* This is for debugging purposes only.
******************************************************************************/
void UndoStack::debugPrint()
{
	qDebug() << "Undo stack (suspend=" << _suspendCount << "index=" << _index << "clean index=" << _cleanIndex << "):";
	int index = 0;
	for(const auto& op : _operations) {
		qDebug() << "  " << index << ":" << qPrintable(op->displayName());
		if(CompoundOperation* compOp = dynamic_cast<CompoundOperation*>(op.get())) {
			compOp->debugPrint(2);
		}
		index++;
	}
}


/******************************************************************************
* Prints a text representation of the compound operation to the console.
* This is for debugging purposes only.
******************************************************************************/
void UndoStack::CompoundOperation::debugPrint(int level)
{
	int index = 0;
	for(const auto& op : _subOperations) {
		qDebug() << QByteArray(level*2, ' ').constData() << index << ":" << qPrintable(op->displayName());
		if(CompoundOperation* compOp = dynamic_cast<CompoundOperation*>(op.get())) {
			compOp->debugPrint(level+1);
		}
		index++;
	}
}

/******************************************************************************
* Is called to undo an operation.
******************************************************************************/
void TargetChangedUndoOperation::undo()
{
	_target->notifyTargetChanged();
}

/******************************************************************************
* Is called to redo an operation.
******************************************************************************/
void TargetChangedRedoOperation::redo()
{
	_target->notifyTargetChanged();
}

}	// End of namespace
