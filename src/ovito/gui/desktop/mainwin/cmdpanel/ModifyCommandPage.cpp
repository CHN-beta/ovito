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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/pipeline/ModifierTemplates.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/oo/CloneHelper.h>
#include <ovito/gui/base/actions/ActionManager.h>
#include <ovito/gui/base/mainwin/PipelineListModel.h>
#include <ovito/gui/desktop/app/GuiApplication.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/dialogs/ModifierTemplatesPage.h>
#include <ovito/gui/desktop/widgets/selection/SceneNodeSelectionBox.h>
#include "ModifyCommandPage.h"

#include <QtNetwork>

namespace Ovito {

/******************************************************************************
* Initializes the modify page.
******************************************************************************/
ModifyCommandPage::ModifyCommandPage(MainWindow* mainWindow, QWidget* parent) : QWidget(parent),
		_datasetContainer(mainWindow->datasetContainer()), _actionManager(mainWindow->actionManager())
{
	QGridLayout* layout = new QGridLayout(this);
	layout->setContentsMargins(2,2,2,2);
	layout->setSpacing(4);
	layout->setColumnStretch(0,1);

	SceneNodeSelectionBox* nodeSelBox = new SceneNodeSelectionBox(_datasetContainer, this);
	layout->addWidget(nodeSelBox, 0, 0, 1, 1);
	QMenu* pipelineMenu = new QMenu(this);
	pipelineMenu->addAction(_actionManager->getAction(ACTION_EDIT_RENAME_PIPELINE));
	pipelineMenu->addAction(_actionManager->getAction(ACTION_EDIT_CLONE_PIPELINE));
	pipelineMenu->addSeparator();
	pipelineMenu->addAction(_actionManager->getAction(ACTION_EDIT_DELETE));
	pipelineMenu->addSeparator();

	// Set up the 'Precompute all frames' menu action.
	QAction* precomputeFramesAction = pipelineMenu->addAction(QIcon(":/guibase/actions/file/cache_pipeline_output.svg"), tr("Precompute all frames"));
	precomputeFramesAction->setCheckable(true);
	connect(pipelineMenu, &QMenu::aboutToShow, this, [this,precomputeFramesAction]() {
		if(_datasetContainer.currentSet() && _datasetContainer.currentSet()->selection()->nodes().empty() == false) {
			if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(_datasetContainer.currentSet()->selection()->nodes().front())) {
				precomputeFramesAction->setChecked(pipeline->pipelineTrajectoryCachingEnabled());
				precomputeFramesAction->setEnabled(true);
				return;
			}
		}
		precomputeFramesAction->setChecked(false);
		precomputeFramesAction->setEnabled(false);
	});
	connect(precomputeFramesAction, &QAction::triggered, this, [this,precomputeFramesAction]() {
		if(_datasetContainer.currentSet() && _datasetContainer.currentSet()->selection()->nodes().empty() == false) {
			if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(_datasetContainer.currentSet()->selection()->nodes().front())) {
				pipeline->setPipelineTrajectoryCachingEnabled(precomputeFramesAction->isChecked());
			}
		}
	});

	QToolButton* pipelineMenuButton = new QToolButton(this);
	pipelineMenuButton->setStyleSheet(
		"QToolButton { padding: 0px; margin: 0px; border: none; background-color: transparent; } "
		"QToolButton::menu-indicator { image: none; } ");
	pipelineMenuButton->setPopupMode(QToolButton::InstantPopup);
	pipelineMenuButton->setIcon(QIcon(":/guibase/actions/edit/pipeline_menu.svg"));
	pipelineMenuButton->setMenu(pipelineMenu);
	pipelineMenuButton->setEnabled(nodeSelBox->isEnabled());
	pipelineMenuButton->setToolTip(tr("Pipeline menu"));
	layout->addWidget(pipelineMenuButton, 0, 1, 1, 1);
	connect(nodeSelBox, &SceneNodeSelectionBox::enabledChanged, pipelineMenuButton, &QToolButton::setEnabled);

	_pipelineListModel = new PipelineListModel(_datasetContainer, _actionManager, this);
	class ModifierListBox : public QComboBox {
	public:
		using QComboBox::QComboBox;
		virtual void showPopup() override {
			static_cast<ModifierListModel*>(model())->updateActionState();
			QComboBox::showPopup();
		}
	};
	_modifierSelector = new ModifierListBox(this);
    layout->addWidget(_modifierSelector, 1, 0, 1, 1);
	_modifierSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	_modifierSelector->setModel(new ModifierListModel(this, mainWindow, _pipelineListModel));
	_modifierSelector->setMaxVisibleItems(0xFFFF);
    connect(_modifierSelector, qOverload<int>(&QComboBox::activated), this, [this](int index) {
		QComboBox* selector = static_cast<QComboBox*>(sender());
		static_cast<ModifierListModel*>(selector->model())->insertModifierByIndex(index);
		selector->setCurrentIndex(0);
	});
	connect(_pipelineListModel, &PipelineListModel::selectedItemChanged, this, [&]() { 
		_modifierSelector->setEnabled(_pipelineListModel->selectedItem() != nullptr); 
	});

	class PipelineListView : public QListView {
	public:
		PipelineListView(QWidget* parent) : QListView(parent) {}
		virtual QSize sizeHint() const override { return QSize(256, 260); }
	protected:
		virtual bool edit(const QModelIndex& index, QAbstractItemView::EditTrigger trigger, QEvent* event) override {
			if(trigger == QAbstractItemView::SelectedClicked && event->type() == QEvent::MouseButtonRelease) {
				// Avoid triggering edit mode when user clicks the check box next to a list item.
				QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
				int origin = visualRect(index).left();
#ifndef Q_OS_MAC
				if(mouseEvent->pos().x() < origin + 50)
#else
				if(mouseEvent->pos().x() < origin + 60)
#endif
					trigger = QAbstractItemView::NoEditTriggers;
			}
			if((trigger == QAbstractItemView::SelectedClicked || trigger == QAbstractItemView::NoEditTriggers) && event->type() == QEvent::MouseButtonRelease) {
				// Detect when user clicks on the collapsable part of a group item.
				if(index.data(PipelineListModel::ItemTypeRole) == PipelineListItem::ModifierGroup) {
					QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
					int origin = visualRect(index).left();
#ifndef Q_OS_MAC
					if(mouseEvent->pos().x() >= origin + 25 && mouseEvent->pos().x() < origin + 50) {
#else
					if(mouseEvent->pos().x() >= origin + 30 && mouseEvent->pos().x() < origin + 60) {
#endif
						trigger = QAbstractItemView::NoEditTriggers;
						// Toggle the collapsed state of the group.
						bool isCollapsed = index.data(PipelineListModel::IsCollapsedRole).toBool();
						const_cast<QAbstractItemModel*>(index.model())->setData(index, !isCollapsed, PipelineListModel::IsCollapsedRole);
					}
				}
			}
			return QListView::edit(index, trigger, event);
		}
	};

	_splitter = new QSplitter(Qt::Vertical);
	_splitter->setChildrenCollapsible(false);

	QWidget* upperContainer = new QWidget();
	_splitter->addWidget(upperContainer);
	QHBoxLayout* subLayout = new QHBoxLayout(upperContainer);
	subLayout->setContentsMargins(0,0,0,0);
	subLayout->setSpacing(2);

	_pipelineWidget = new PipelineListView(upperContainer);
	_pipelineWidget->setDragDropMode(QAbstractItemView::InternalMove);
	_pipelineWidget->setDragEnabled(true);
	_pipelineWidget->setAcceptDrops(true);
	_pipelineWidget->setDragDropOverwriteMode(false);
	_pipelineWidget->setDropIndicatorShown(true);
	_pipelineWidget->setEditTriggers(QAbstractItemView::SelectedClicked);
	_pipelineWidget->setModel(_pipelineListModel);
	_pipelineWidget->setSelectionModel(_pipelineListModel->selectionModel());
	_pipelineWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	_pipelineWidget->setIconSize(_pipelineListModel->iconSize());
	subLayout->addWidget(_pipelineWidget);

	// Listen to selection changes in the pipeline editor list widget.
	connect(_pipelineListModel, &PipelineListModel::selectedItemChanged, this, &ModifyCommandPage::onSelectedItemChanged);

	// Double-click on a modifier or visual element toggles the enabled state of the element.
	connect(_pipelineWidget, &PipelineListView::doubleClicked, this, &ModifyCommandPage::onModifierStackDoubleClicked);

	QToolBar* editToolbar = new QToolBar(this);
	editToolbar->setOrientation(Qt::Vertical);
#ifndef Q_OS_MACX
	editToolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; }");
#endif
	subLayout->addWidget(editToolbar);

	// Create pipeline editor toolbar.
	editToolbar->addAction(_actionManager->getAction(ACTION_MODIFIER_DELETE));
	editToolbar->addSeparator();
	editToolbar->addAction(_actionManager->getAction(ACTION_MODIFIER_MOVE_UP));
	editToolbar->addAction(_actionManager->getAction(ACTION_MODIFIER_MOVE_DOWN));
	editToolbar->addSeparator();
	editToolbar->addAction(_actionManager->getAction(ACTION_PIPELINE_TOGGLE_MODIFIER_GROUP));
	editToolbar->addAction(_actionManager->getAction(ACTION_PIPELINE_MAKE_INDEPENDENT));

	QAction* manageModifierTemplatesAction = _actionManager->createCommandAction(ACTION_MODIFIER_MANAGE_TEMPLATES, tr("Manage Modifier Templates..."), ":/guibase/actions/modify/modifier_save_preset.bw.svg", tr("Open the dialog that lets you manage the saved modifier templates."));
	connect(manageModifierTemplatesAction, &QAction::triggered, [mainWindow]() {
		ApplicationSettingsDialog dlg(mainWindow, &ModifierTemplatesPage::OOClass());
		dlg.exec();
	});
	editToolbar->addAction(manageModifierTemplatesAction);

	layout->addWidget(_splitter, 2, 0, 1, 2);
	layout->setRowStretch(2, 1);

	// Create the properties panel.
	_propertiesPanel = new PropertiesPanel(nullptr, mainWindow);
	_propertiesPanel->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
	_splitter->addWidget(_propertiesPanel);
	_splitter->setStretchFactor(1,1);

	connect(&_datasetContainer, &DataSetContainer::selectionChangeComplete, this, &ModifyCommandPage::onSelectionChangeComplete);

	// Create About panel.
	createAboutPanel();
}

/******************************************************************************
* Loads the layout of the widgets from the settings store.
******************************************************************************/
void ModifyCommandPage::restoreLayout() 
{
	QSettings settings;
	settings.beginGroup("app/mainwindow/modify");
	QVariant state = settings.value("splitter");
	if(state.canConvert<QByteArray>())
		_splitter->restoreState(state.toByteArray());
}

/******************************************************************************
* Saves the layout of the widgets to the settings store.
******************************************************************************/
void ModifyCommandPage::saveLayout() 
{
	QSettings settings;
	settings.beginGroup("app/mainwindow/modify");
	settings.setValue("splitter", _splitter->saveState());
}

/******************************************************************************
* This is called after all changes to the selection set have been completed.
******************************************************************************/
void ModifyCommandPage::onSelectionChangeComplete(SelectionSet* newSelection)
{
	_pipelineListModel->refreshList();
}

/******************************************************************************
* Is called when a new modification list item has been selected, or if the currently
* selected item has changed.
******************************************************************************/
void ModifyCommandPage::onSelectedItemChanged()
{
	PipelineListItem* currentItem = pipelineListModel()->selectedItem();
	RefTarget* editObject = nullptr;

	if(currentItem != nullptr) {
		editObject = currentItem->object();
		if(currentItem->isSubObject())
			pipelineListModel()->setNextSubObjectToSelectByTitle(currentItem->title());
	}

	if(editObject != _propertiesPanel->editObject()) {
		_propertiesPanel->setEditObject(editObject);

		// Request a viewport update whenever a new item in the pipeline editor is selected, 
		// because the currently selected modifier may be rendering gizmos in the viewports. 
		if(_datasetContainer.currentSet())
			_datasetContainer.currentSet()->viewportConfig()->updateViewports();
	}

	// Whenever no object is selected, show the About Panel containing information about the program.
	if(currentItem == nullptr && pipelineListModel()->selectedIndex() == -1)
		_aboutRollout->show();
	else
		_aboutRollout->hide();
}

/******************************************************************************
* This called when the user double clicks on an item in the modifier stack.
******************************************************************************/
void ModifyCommandPage::onModifierStackDoubleClicked(const QModelIndex& index)
{
	PipelineListItem* item = pipelineListModel()->item(index.row());
	OVITO_CHECK_OBJECT_POINTER(item);

	if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
		// Toggle enabled state of modifier.
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Toggle modifier state"), [modApp]() {
			modApp->modifier()->setEnabled(!modApp->modifier()->isEnabled());
		});
	}

	if(DataVis* vis = dynamic_object_cast<DataVis>(item->object())) {
		// Toggle enabled state of vis element.
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Toggle visual element"), [vis]() {
			vis->setEnabled(!vis->isEnabled());
		});
	}
}

/******************************************************************************
* Creates the rollout panel that shows information about the application
* whenever no object is selected.
******************************************************************************/
void ModifyCommandPage::createAboutPanel()
{
	QWidget* rollout = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(8,8,8,8);

	QTextBrowser* aboutLabel = new QTextBrowser(rollout);
	aboutLabel->setObjectName("AboutLabel");
	aboutLabel->setOpenExternalLinks(true);
	aboutLabel->setMinimumHeight(600);
	aboutLabel->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
	aboutLabel->viewport()->setAutoFillBackground(false);
	layout->addWidget(aboutLabel);

	QByteArray newsPage;
#if !defined(OVITO_BUILD_APPSTORE_VERSION)
	QSettings settings;
	if(settings.value("updates/check_for_updates", true).toBool()) {
		// Retrieve cached news page from settings store.
		newsPage = settings.value("news/cached_webpage").toByteArray();
	}
	if(newsPage.isEmpty()) {
		QResource res("/gui/mainwin/command_panel/about_panel.html");
		newsPage = QByteArray((const char *)res.data(), (int)res.size());
	}
#else
	QResource res("/gui/mainwin/command_panel/about_panel_no_updates.html");
	newsPage = QByteArray((const char *)res.data(), (int)res.size());
#endif

	_aboutRollout = _propertiesPanel->addRollout(rollout, Application::applicationName());
	showProgramNotice(QString::fromUtf8(newsPage.constData()));

#if !defined(OVITO_BUILD_APPSTORE_VERSION)
	if(settings.value("updates/check_for_updates", true).toBool()) {
		constexpr int INSTALLATION_ID_LENGTH = 18;

		// Retrieve/generate unique installation id.
		QByteArray id;
		if(settings.value("updates/transmit_id", true).toBool()) {
			if(settings.contains("installation/id")) {
				id = QByteArray::fromHex(settings.value("installation/id").toString().toLatin1());
				if(id == QByteArray(INSTALLATION_ID_LENGTH, '\0') || id.size() != INSTALLATION_ID_LENGTH)
					id.clear();
			}
			if(id.isEmpty()) {
				// Generate a new unique ID.
				id.fill('0', INSTALLATION_ID_LENGTH);
				std::random_device rdev;
				std::uniform_int_distribution<> rdist(0, 0xFF);
				for(auto& c : id)
					c = (char)rdist(rdev);
				settings.setValue("installation/id", QVariant::fromValue(QString::fromLatin1(id.toHex())));
			}
		}
		else {
			id.fill(0, INSTALLATION_ID_LENGTH);
		}

		QString operatingSystemString;
#if defined(Q_OS_MACOS)
		operatingSystemString = QStringLiteral("macosx");
#elif defined(Q_OS_WIN) || defined(Q_OS_CYGWIN)
		operatingSystemString = QStringLiteral("win");
#elif defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
		operatingSystemString = QStringLiteral("linux");
#else
		operatingSystemString = QStringLiteral("other");
#endif

		QString programEdition;
#if defined(OVITO_BUILD_BASIC)
		programEdition = QStringLiteral("basic/");
#elif defined(OVITO_BUILD_PRO)
		programEdition = QStringLiteral("pro/");
#endif

		// Fetch newest web page from web server.
		QString urlString = QString("https://www.ovito.org/appnews/v%1.%2.%3/%4?ovito=%5&OS=%6%7")
				.arg(Application::applicationVersionMajor())
				.arg(Application::applicationVersionMinor())
				.arg(Application::applicationVersionRevision())
				.arg(programEdition)
				.arg(QString(id.toHex()))
				.arg(operatingSystemString)
				.arg(QT_POINTER_SIZE*8);
		QNetworkAccessManager* networkAccessManager = Application::instance()->networkAccessManager();
		QNetworkReply* networkReply = networkAccessManager->get(QNetworkRequest(QUrl(urlString)));
		connect(networkReply, &QNetworkReply::finished, this, &ModifyCommandPage::onWebRequestFinished);
	}
#endif
}

/******************************************************************************
* Is called by the system when fetching the news web page from the server is
* completed.
******************************************************************************/
void ModifyCommandPage::onWebRequestFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
	if(reply->error() == QNetworkReply::NoError) {
		QByteArray page = reply->readAll();
		reply->close();
		if(page.startsWith("<html><!--OVITO-->")) {

			showProgramNotice(QString::fromUtf8(page.constData()));

			QSettings settings;
			settings.setValue("news/cached_webpage", page);
		}
	}
	reply->deleteLater();
}

/******************************************************************************
* Displays the given HTML page content in the About pane.
******************************************************************************/
void ModifyCommandPage::showProgramNotice(const QString& htmlPage)
{
	QString finalText = htmlPage;

#ifdef OVITO_EXPIRATION_DATE
	QDate expirationDate = QDate::fromString(QStringLiteral(OVITO_EXPIRATION_DATE), Qt::ISODate);
	QDate currentDate = QDate::currentDate();
	QString expirationNotice;
	if(currentDate < expirationDate.addMonths(-1)) {
		expirationNotice = tr("<h4>Preview build</h4><p>You are using a preview version of %1, which will expire on %2. "
				"The final release of %1 will be made available on our website <a href=\"https://www.ovito.org/\">www.ovito.org</a>.</p>")
			.arg(Application::applicationName())
			.arg(expirationDate.toString(Qt::SystemLocaleShortDate));
	}
	else if(currentDate <= expirationDate) {
		expirationNotice = tr("<h4>Preview build: Expiration date approaching</h4><p style=\"background-color: rgb(230,180,180);\">You are using a preview version of %1, which will expire on %2. "
				"The final program release is now available. Please visit our website <a href=\"https://www.ovito.org/\">www.ovito.org</a>. "
				"<br>This preview release will stop working in %3 days!</p>")
			.arg(Application::applicationName())
			.arg(expirationDate.toString(Qt::SystemLocaleShortDate))
			.arg(currentDate.daysTo(expirationDate));
	}
	else {
		expirationNotice = tr("<h4>Preview build</h4><p style=\"background-color: rgb(230,180,180);\">This preview version of %1 has expired on %2 and will no longer work. "
				"The final program release is now available, please visit our website <a href=\"https://www.ovito.org/\">www.ovito.org</a>. ")
			.arg(Application::applicationName())
			.arg(expirationDate.toString(Qt::SystemLocaleShortDate));
	}
	finalText.replace(QStringLiteral("<p>&nbsp;</p>"), expirationNotice);
#endif

	QTextBrowser* aboutLabel = _aboutRollout->findChild<QTextBrowser*>("AboutLabel");
	OVITO_CHECK_POINTER(aboutLabel);
	aboutLabel->setHtml(finalText);
}


}	// End of namespace
