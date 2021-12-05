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

#pragma once


#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/properties/PropertyExpressionEvaluator.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/gui/desktop/mainwin/data_inspector/DataInspectionApplet.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>

namespace Ovito::StdObj {

/**
 * \brief Data inspector page for property-based data.
 */
class OVITO_STDOBJGUI_EXPORT PropertyInspectionApplet : public DataInspectionApplet
{
	OVITO_CLASS(PropertyInspectionApplet)

public:

	/// Lets the applet update the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* pipeline) override;

	/// Returns the data display widget.
	QTableView* tableView() const { return _tableView; }

	/// Returns the input widget for the filter expression.
	AutocompleteLineEdit* filterExpressionEdit() const { return _filterExpressionEdit; }

	/// Returns the UI action that resets the filter expression.
	QAction* resetFilterAction() const { return _resetFilterAction; }

	/// Returns the number of currently displayed elements.
	int visibleElementCount() const { return _filterModel->rowCount(); }

	/// Returns the index of the i-th element currently shown in the table.
	size_t visibleElementAt(int index) const { return _filterModel->mapToSource(_filterModel->index(index, 0)).row(); }

	/// Returns the property container object that is currently selected.
	const PropertyContainer* selectedContainerObject() const { return static_object_cast<PropertyContainer>(selectedDataObject()); }

	/// Selects a specific data object in this applet.
	virtual bool selectDataObject(PipelineObject* dataSource, const QString& objectIdentifierHint, const QVariant& modeHint) override;

protected:

	/// Constructor.
	PropertyInspectionApplet(const PropertyContainerClass& containerClass) : DataInspectionApplet(containerClass), _containerClass(containerClass) {}

	/// Lets the applet create the UI widgets that are to be placed into the data inspector panel.
	void createBaseWidgets();

	/// Creates the evaluator object for filter expressions.
	virtual std::unique_ptr<PropertyExpressionEvaluator> createExpressionEvaluator() {
		return std::make_unique<PropertyExpressionEvaluator>();
	}

	/// Determines the text shown in cells of the vertical header column.
	virtual QVariant headerColumnText(int section) { return section; }

	/// Determines whether the given property represents a color.
	virtual bool isColorProperty(const PropertyObject* property) const {
		return property->type() == PropertyObject::GenericColorProperty;
	}

	/// Creates an optional ad-hoc property that serves as header column for the table.
	virtual ConstPropertyPtr createHeaderColumnProperty(const PropertyContainer* container) { return {}; }

Q_SIGNALS:

	/// This signal is emitted whenever the filter expression has changed.
	void filterChanged();

public Q_SLOTS:

	/// Sets the filter expression.
	void setFilterExpression(const QString& expression);

private Q_SLOTS:

	/// Is called when the user selects a different container object from the list.
	void onCurrentContainerChanged();

	/// Is called when the uer has changed the filter expression.
	void onFilterExpressionEntered();

	/// Is called when an error during filter evaluation ocurred.
	void onFilterStatusChanged(const QString& msgText);

private:

	/// A table model for displaying the property data.
	class PropertyTableModel : public QAbstractTableModel
	{
	public:

		/// Constructor.
		PropertyTableModel(PropertyInspectionApplet* applet, QObject* parent) : QAbstractTableModel(parent), _applet(applet) {}

		/// Returns the number of rows.
		virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override {
			if(parent.isValid() || _properties.empty()) return 0;
			return (int)std::min(_properties.front()->size(), (size_t)std::numeric_limits<int>::max());
		}

		/// Returns the number of columns.
		virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override {
			return parent.isValid() ? 0 : _properties.size();
		}

		/// Returns the data stored under the given 'role' for the item referred to by the 'index'.
		virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

		/// Returns the data for the given role and section in the header with the specified orientation.
		virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
			if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
				OVITO_ASSERT(section >= 0 && section < _properties.size());
				return _properties[section]->name();
			}
			else if(orientation == Qt::Vertical && role == Qt::DisplayRole) {
				return _applet->headerColumnText(section);
			}
			return QAbstractTableModel::headerData(section, orientation, role);
		}

		/// Replaces the contents of this data model.
		void setContents(const PropertyContainer* container);

		/// Returns the list of properties managed by this table model.
		const std::vector<ConstPropertyPtr>& properties() const { return _properties; }

	private:

		/// The owner of the model.
		PropertyInspectionApplet* _applet;

		/// The list of properties.
		std::vector<ConstPropertyPtr> _properties;
	};

	/// A proxy model for filtering the property list.
	class PropertyFilterModel : public QSortFilterProxyModel
	{
	public:

		/// Constructor.
		PropertyFilterModel(PropertyInspectionApplet* applet, QObject* parent) : QSortFilterProxyModel(parent), _applet(applet) {}

		/// Replaces the contents of this data model.
		void setContentsBegin();

		/// Replaces the contents of this data model.
		void setContentsEnd() {
			if(_filterExpression.isEmpty() == false)
				endResetModel();
		}

		/// Sets the filter expression.
		void setFilterExpression(const QString& expression) {
			if(_filterExpression != expression) {
				beginResetModel();
				_filterExpression = expression;
				setupEvaluator();
				endResetModel();
			}
		}

	protected:

		/// Performs the filtering of data rows.
		virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

		/// Initializes the expression evaluator.
		void setupEvaluator();

	private:

		/// The owner of the model.
		PropertyInspectionApplet* _applet;

		/// The filtering expression.
		QString _filterExpression;

		/// The filter expression evaluator.
		mutable std::unique_ptr<PropertyExpressionEvaluator> _evaluator;

		/// The filter expression evaluator worker.
		mutable std::unique_ptr<PropertyExpressionEvaluator::Worker> _evaluatorWorker;

		friend PropertyInspectionApplet;
	};

private:

	/// The type of container objects displayed by this applet.
	const PropertyContainerClass& _containerClass;

	/// The property data display widget.
	QTableView* _tableView = nullptr;

	/// The property table model.
	PropertyTableModel* _tableModel = nullptr;

	/// The filter model.
	PropertyFilterModel* _filterModel;

	/// Input widget for the filter expression.
	AutocompleteLineEdit* _filterExpressionEdit;

	/// The UI action that resets the filter expression.
	QAction* _resetFilterAction;

	/// The current filter status.
	QString _filterStatusString;

	/// For cleaning up widgets.
	QObjectCleanupHandler _cleanupHandler;
};

}	// End of namespace
