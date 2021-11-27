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
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include "Qt3DViewportWindow.h"

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QAspectEngine>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraLens>
#include <Qt3DRender/QRenderAspect>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QPointLight>

#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QTorusMesh>

namespace Ovito {

OVITO_REGISTER_VIEWPORT_WINDOW_IMPLEMENTATION(Qt3DViewportWindow);

/******************************************************************************
* Constructor.
******************************************************************************/
Qt3DViewportWindow::Qt3DViewportWindow(Viewport* viewport, ViewportInputManager* inputManager, UserInterface* gui, QWidget* parentWidget) : 
		BaseViewportWindow(gui, inputManager, viewport)
{
	// Embed the QWindow in a QWidget container.
	_widget = QWidget::createWindowContainer(this, parentWidget);

	_widget->setMouseTracking(true);
    _widget->setFocusPolicy(Qt::StrongFocus);

    // Configure Qt3D render settings.
    renderSettings()->setRenderPolicy(Qt3DRender::QRenderSettings::OnDemand);
    defaultFrameGraph()->setClearColor(QColor(QRgb(0x000000)));

    // Root entity
    Qt3DCore::QEntity* rootEntity = new Qt3DCore::QEntity;

    // Material
    Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial(rootEntity);
    material->setDiffuse(QColor(QRgb(0xbeb32b)));

    // Camera
    Qt3DRender::QCamera* camera = this->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 0, 40.0f));
    camera->setViewCenter(QVector3D(0, 0, 0));

    // Light
    Qt3DCore::QEntity *lightEntity = new Qt3DCore::QEntity(rootEntity);
    Qt3DRender::QPointLight *light = new Qt3DRender::QPointLight(lightEntity);
    light->setColor("white");
    light->setIntensity(1);
    lightEntity->addComponent(light);
    Qt3DCore::QTransform* lightTransform = new Qt3DCore::QTransform(lightEntity);
    lightTransform->setTranslation(camera->position());
    lightEntity->addComponent(lightTransform); 

    // Torus
    Qt3DCore::QEntity *torusEntity = new Qt3DCore::QEntity(rootEntity);
    Qt3DExtras::QTorusMesh *torusMesh = new Qt3DExtras::QTorusMesh;
    torusMesh->setRadius(5);
    torusMesh->setMinorRadius(1);
    torusMesh->setRings(100);
    torusMesh->setSlices(20);
    Qt3DCore::QTransform *torusTransform = new Qt3DCore::QTransform;
    torusTransform->setScale3D(QVector3D(1.5, 1, 0.5));
    torusTransform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 45.0f));
    torusEntity->addComponent(torusMesh);
    torusEntity->addComponent(torusTransform);
    torusEntity->addComponent(material);

    // Sphere
    Qt3DCore::QEntity *sphereEntity = new Qt3DCore::QEntity(rootEntity);
    Qt3DExtras::QSphereMesh *sphereMesh = new Qt3DExtras::QSphereMesh;
    sphereMesh->setRadius(3);
    sphereMesh->setGenerateTangents(true);
    Qt3DCore::QTransform *sphereTransform = new Qt3DCore::QTransform;
    sphereEntity->addComponent(sphereMesh);
    sphereEntity->addComponent(sphereTransform);
    sphereEntity->addComponent(material);

    setRootEntity(rootEntity);
}

/******************************************************************************
* Puts an update request event for this viewport on the event loop.
******************************************************************************/
void Qt3DViewportWindow::renderLater()
{
    // Request a deferred refresh of the QWindow.
	_updateRequested = true;
    if(viewport() && !viewport()->dataset()->viewportConfig()->isSuspended()) {
    	Qt3DExtras::Qt3DWindow::requestUpdate();
    }
}

/******************************************************************************
* If an update request is pending for this viewport window, immediately
* processes it and redraw the window contents.
******************************************************************************/
void Qt3DViewportWindow::processViewportUpdate()
{
	if(_updateRequested) {
        OVITO_ASSERT_MSG(!viewport()->isRendering(), "Qt3DViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
        OVITO_ASSERT_MSG(!viewport()->dataset()->viewportConfig()->isRendering(), "Qt3DViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");

        // Note: All we can do is request a deferred window update. 
        // A QWindow has no way of forcing an immediate repaint.
        Qt3DExtras::Qt3DWindow::requestUpdate();
    }
}

/******************************************************************************
* Determines the object that is visible under the given mouse cursor position.
******************************************************************************/
ViewportPickResult Qt3DViewportWindow::pick(const QPointF& pos)
{
	ViewportPickResult result;
	return result;
}

}	// End of namespace
