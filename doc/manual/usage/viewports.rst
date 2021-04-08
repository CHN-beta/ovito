.. _usage.viewports:

Viewport windows
================

The viewport windows show the three-dimensional scene containing the loaded dataset.
OVITO's main window has four viewports, laid out in a two by two grid,
all showing the same scene from different perspectives.
The label in the upper left corner of each viewport window and the axis tripod in the lower left corner
indicate the orientation of the viewport's virtual camera.

.. _usage.viewports.navigation:

Navigation functions

.. image:: /images/viewport_control_toolbar/viewport_screenshot.*
  :width: 30%
  :align: right

Use the mouse to rotate or move the virtual camera of a viewport:

* Left-click and drag to rotate the camera around the current orbit center, which is located in the center of the simulation box by default.
* Right-click and drag the mouse in order to move the camera parallel to the projection plane. You can also use the middle mouse button 
  or the shift key + left mouse button for this.
* Use the mouse wheel to zoom in or out.
* Double-click an object to reposition the orbit center to the point under the mouse cursor. 
  From now on the camera will rotate around that new location in 3d space marked with a three-dimensional cross.
* Double-click in an empty region of a viewport to reset the orbit center to the center of the active dataset.

Note that the z-axis is considered the "up" (vertical) direction, and OVITO
constrains the camera orientation such that this axis always points upward in the viewports.
You turn this default behavior off in the :ref:`viewport context menu <usage.viewports.menu>` or
choose a different constraint axis in the :ref:`application settings <application_settings.viewports>` 
dialog of OVITO.

.. _usage.viewports.toolbar:

Viewport toolbar
----------------

.. image:: /images/viewport_control_toolbar/viewport_toolbar.*
   :width: 16%
   :align: left

The viewport toolbar is located below the viewports and contains buttons to activate various navigation input modes.
In addition, you will find two other useful functions here:

.. image:: /images/viewport_control_toolbar/zoom_scene_extents.bw.*
   :width: 32
   :align: right

The :guilabel:`Zoom Scene Extents` function automatically adjusts the virtual camera of the active viewport
such that all objects in the scene become fully visible. The active viewport is marked by a yellow border.
Use the :kbd:`Ctrl` key (:kbd:`Command` key on macOS) to zoom all viewports at once.

.. image:: /images/viewport_control_toolbar/maximize_viewport.bw.*
   :width: 32
   :align: right

The :guilabel:`Maximize Active Viewport` button enlarges the active viewport to fill the entire main window.
Clicking the button a second time restores the original 2x2 viewport layout.

.. _usage.viewports.menu:

Viewport menu
-------------

.. image:: /images/viewport_control_toolbar/viewport_menu_screenshot.*
   :width: 30%
   :align: right

Click the caption label in the upper left corner of a viewport (e.g. *Perspective*, *Top*, etc.)
to open the *viewport menu* as shown in the screenshot.

The :guilabel:`View Type` menu lets you switch to one of the standard
viewing orientations and between parallel (orthogonal) and perspective projection types. The
:ref:`Adjust View <viewports.adjust_view_dialog>` function gives you precise
control over the position and orientation of the viewport's camera using numeric input fields.

The :guilabel:`Preview Mode` option activates a virtual frame that is displayed in the viewport to
indicate the region that will be visible in :ref:`rendered images <usage.rendering>`. The aspect ratio of the frame reflects the
image size currently set in the :ref:`Render settings <core.render_settings>` panel.
With preview mode active, scene objects and any :ref:`viewport layers <viewport_layers>` will
be rendered in the interactive viewport window just like they are in the final output image.

Use the :guilabel:`Create Camera` function to insert a camera object into the three-dimensional
scene. This object will be linked to the viewport, and moving the camera object around automatically updates the viewport
accordingly. This gives you the possibility to :ref:`animate the camera <usage.animation.camera>`.

.. _viewports.adjust_view_dialog:
.. _core.render_settings:
.. _application_settings.viewports:
.. _usage.rendering:
.. _usage.animation.camera: