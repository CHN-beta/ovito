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

#pragma once


#include <ovito/gui_wasm/GUI.h>
#include <ovito/core/viewport/ViewportWindowInterface.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/TextPrimitive.h>
#include <ovito/core/rendering/ImagePrimitive.h>
#include <ovito/core/rendering/LinePrimitive.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief The internal render window assciated with the Viewport class.
 */
class OVITO_GUIWASM_EXPORT ViewportWindow : public QObject, public ViewportWindowInterface
{
	Q_OBJECT

public:

	/// Constructor.
	ViewportWindow(Viewport* owner, ViewportInputManager* inputManager, QQuickWindow* quickWindow);

	/// Destructor.
	virtual ~ViewportWindow();

	/// Returns the underlying Qt Quick window.
	QQuickWindow* quickWindow() const { return _quickWindow; }

	/// Returns the owning viewport of this window.
	Viewport* viewport() const { return _viewport; }

	/// Returns the input manager handling mouse events of the viewport (if any).
	ViewportInputManager* inputManager() const;

    /// \brief Puts an update request event for this window on the event loop.
	virtual void renderLater() override;

	/// \brief Immediately redraws the contents of this window.
	virtual void renderNow() override;

	/// If an update request is pending for this viewport window, immediately
	/// processes it and redraw the window contents.
	virtual void processViewportUpdate() override;

	/// Returns the current size of the viewport window (in device pixels).
	virtual QSize viewportWindowDeviceSize() override {
		return quickWindow()->size() * quickWindow()->effectiveDevicePixelRatio();
	}

	/// Returns the current size of the viewport window (in device-independent pixels).
	virtual QSize viewportWindowDeviceIndependentSize() override {
		return quickWindow()->size();
	}

	/// Lets the viewport window delete itself.
	/// This is called by the Viewport class destructor.
	virtual void destroyViewportWindow() override {
		// Detach from viewport.
		_viewport = nullptr;
		deleteLater();
	}

	/// Renders custom GUI elements in the viewport on top of the scene.
	virtual void renderGui() override;

	/// Provides access to the OpenGL context used by the viewport window for rendering.
	virtual QOpenGLContext* glcontext() override { return quickWindow()->openglContext(); }

	/// \brief Determines the object that is visible under the given mouse cursor position.
	ViewportPickResult pick(const QPointF& pos);

	/// \brief Displays the context menu for the viewport.
	/// \param pos The position in where the context menu should be displayed.
	void showViewportMenu(const QPoint& pos = QPoint(0,0));

private:

	/// Render the axis tripod symbol in the corner of the viewport that indicates
	/// the coordinate system orientation.
	void renderOrientationIndicator();

	/// Renders the frame on top of the scene that indicates the visible rendering area.
	void renderRenderFrame();

private Q_SLOTS:

	/// Renders the contents of the viewport window.
	void renderViewport();

private:

	/// The owning viewport of this window.
	Viewport* _viewport;

	/// The underlying Qt Quick window.
	QQuickWindow* _quickWindow;

	/// A flag that indicates that a viewport update has been requested.
	bool _updateRequested = false;

	/// The zone in the upper left corner of the viewport where
	/// the context menu can be activated by the user.
	QRect _contextMenuArea;

	/// Indicates that the mouse cursor is currently positioned inside the
	/// viewport area that activates the viewport context menu.
	bool _cursorInContextMenuArea = false;

	/// The input manager handling mouse events of the viewport.
	QPointer<ViewportInputManager> _inputManager;

#ifdef OVITO_DEBUG
	/// Counts how often this viewport has been rendered.
	int _renderDebugCounter = 0;
#endif

	/// The rendering buffer maintained to render the viewport's caption text.
	std::shared_ptr<TextPrimitive> _captionBuffer;

	/// The geometry buffer used to render the viewport's orientation indicator.
	std::shared_ptr<LinePrimitive> _orientationTripodGeometry;

	/// The rendering buffer used to render the viewport's orientation indicator labels.
	std::shared_ptr<TextPrimitive> _orientationTripodLabels[3];

	/// This is used to render the render frame around the viewport.
	std::shared_ptr<ImagePrimitive> _renderFrameOverlay;

	/// This is the renderer of the interactive viewport.
	OORef<ViewportSceneRenderer> _viewportRenderer;

	/// This renderer generates an offscreen rendering of the scene that allows picking of objects.
	OORef<PickingSceneRenderer> _pickingRenderer;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
