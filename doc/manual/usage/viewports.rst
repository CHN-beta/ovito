.. _usage.viewports:

Viewport windows
================

The viewport windows of OVITO show different views of the three-dimensional scene.
You can switch between the default two-by-two grid layout and a single viewport that fills the entire main window.
The caption in the upper left corner of each viewport and the axis tripod in the lower left corner
indicate the orientation of the virtual camera.

.. _usage.viewports.navigation:

Camera navigation
-----------------

.. image:: /images/viewport_control_toolbar/viewport_screenshot.*
  :width: 30%
  :align: right

Use the mouse to rotate or move the virtual camera of a viewport:

* Left-click and drag to rotate the camera around the current orbit center, which is located in the center of the simulation model by default.
* Right-click and drag the mouse in order to move the camera parallel to the projection plane. You can also use the middle mouse button 
  or :kbd:`Shift` + left mouse button for this.
* Use the mouse wheel to zoom in or out.
* Double-click an object to reposition the orbit center to the point under the mouse cursor. 
  From now on the camera will rotate around that new location marked with a three-dimensional cross.
* Double-click in an empty region of a viewport to reset the orbit center to the center of the simulation model.

Note that the z-axis is considered the "up" (vertical) direction, and OVITO constrains the camera orientation 
such that this axis always points upward in the viewports. You turn this behavior off in the :ref:`viewport context menu <usage.viewports.menu>` 
or change the constraint axis in the :ref:`application settings <application_settings.viewports>` of OVITO.

.. _usage.viewports.toolbar:

Viewport toolbar
----------------

.. image:: /images/viewport_control_toolbar/viewport_toolbar.*
   :width: 16%
   :align: left

The viewport toolbar is located below the viewports and provides buttons for explicitly activating various navigation input modes.
In addition, you can find two other useful functions here:

.. image:: /images/viewport_control_toolbar/zoom_scene_extents.bw.*
   :width: 32
   :align: right

The :guilabel:`Zoom Scene Extents` button automatically adjusts the virtual camera of the active viewport
such that all objects in the scene become fully visible. Use the :kbd:`Ctrl` key (:kbd:`Command` key on macOS) to 
do it for all viewports at once.

.. image:: /images/viewport_control_toolbar/maximize_viewport.bw.*
   :width: 32
   :align: right

The :guilabel:`Maximize Active Viewport` button enlarges the active viewport to fill the entire main window.
Click the button a second time to restore the original 2-by-2 viewport layout.

.. _usage.viewports.menu:

Viewport menu
-------------

.. image:: /images/viewport_control_toolbar/viewport_menu_screenshot.*
   :width: 40%
   :align: right

Click on the caption text in the upper left corner of a viewport window (e.g. *Perspective*, *Top*, etc.)
to open the *viewport menu*.

:guilabel:`Preview Mode` activates a virtual frame in the viewport to
precisely indicate the rectangular region that will be visible in :ref:`rendered output images <usage.rendering>`. 
The aspect ratio of the frame reflects the image size currently set in the :ref:`Render settings <core.render_settings>` panel.

The :guilabel:`Constrain Rotation` option controls whether the virtual camera's orientation is constrained at all times such 
that the z-axis of the simulation coordinate system is pointing upward. If needed, you can set *x* or *y* to be the
vertical axis in the :ref:`application settings dialog <application_settings.viewports>`. 

The :guilabel:`View Type` submenu lets you switch between different standard
viewing directions and parallel (orthogonal) and perspective projection types. The
:ref:`Adjust View <viewports.adjust_view_dialog>` function gives you precise numeric
control over the positioning and orientation of the viewport's camera.

Use the :guilabel:`Create Camera` function to insert a camera object into the three-dimensional
scene. This object will be linked to the viewport, and moving the camera object updates the view
accordingly. This gives you the possibility to create animations with a :ref:'camera motion path <usage.animation.camera>`.

The :guilabel:`Layout` submenu contains several functions that modify the current layout of viewport windows.
OVITO creates 4 standard viewports by default, which are arranged in a 2-by-2 grid. You can add 
and remove viewport windows as needed, and adjust their relative positioning by dragging the separator 
lines between them with the mouse. OVITO Pro provides the option to render images and animations that shoow 
multiple views side by side, see the :ref:`Render all viewports <core.render_settings>` option.