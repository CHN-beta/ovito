.. _particles.modifiers.slice:

Slice
-----

.. image:: /images/modifiers/slice_panel.png
  :width: 30%
  :align: right

This modifier either deletes or selects all elements on one side of an infinite cutting plane.
Alternatively, the modifier can cut out a slab of a given thickness from the structure.

.. figure:: /images/modifiers/slice_example_input.png
  :figwidth: 20%

  Input

.. figure:: /images/modifiers/slice_example_output1.png
  :figwidth: 20%

  Output (slab width = 0)

.. figure:: /images/modifiers/slice_example_output2.png
  :figwidth: 20%

  Output (slab width > 0)


Parameters
""""""""""

Cartesian / reduced cell coordinates
  Selects whether the `Distance` and `Normal` parameters are specified in 
  Cartesian space (i.e. global simulation coordinate system) or in reduced simulation cell space. The latter allows you to specify
  the cutting plane's orientation in terms of the reciprocal cell vectors (Miller indices) and relative to the cell's origin. 

Distance
  The (signed) distance of the cutting plane from the coordinate origin
  measured parallel to the plane normal. If `Cartesian` mode is active,
  the origin is the point (0,0,0) of the gloabl simulation coordinate system.
  In `Reduced cell coordinates`, the origin is the corner of the simulation cell. 

Normal (X,Y,Z)
  The three components of the plane's normal vector, which defines the orientation of the plane. 
  This vector does not have to be a unit vector. Note that you can click on the blue labels
  next to each input field to reset the vector to point along the corresponding coordinate axis.

Slab width
  Specifies the width of the slab to cut out from the input structure.
  If this value is zero (the default), everything on one side of the
  cutting plane is deleted. If `slab width` is set to a positive value, 
  a slice of the given thickness is cut out.

Reverse orientation
  Effectively flips the cutting plane's orientation. If the `slab width`
  parameter is zero, activating this option will remove all elements on the opposite side
  of the plane. Otherwise this option will let the modifier cut away a slab of
  the given thickness from the input structure.

Create selection (do not delete)
  This option lets the modifier select elements instead of deleting them.

Apply to selection only
  Restricts the effect of the modifier to the subset of elements that are currently selected.

Visualize plane
  Lets the modifier generate polygonal geometry to visualize the plane in rendered images. 
  Otherwise the plane is only indicated in the interactive viewports of OVITO.

Alignment functions
"""""""""""""""""""

These functions reposition or align the cutting plane:

  * :guilabel:`Center in simulation cell` moves the plane to the center of the simulation cell by shifting it parallel to the plane's normal vector.

  * :guilabel:`Align view to plane` rotates the viewport camera to make it look perpendicular onto the cutting plane.  

  * :guilabel:`Align plane to view` rotates the plane such that its normal vector becomes parallel with the camera viewing direction of the active viewport.

  * :guilabel:`Pick three points` lets you pick three spatial points with the mouse in the viewports. The cutting plane will be repositioned such that it goes through all three points.

Animating the plane
"""""""""""""""""""

The position of the cutting plane can be animated. Use the :guilabel:`A` button
next to each numerical parameter field to open the corresponding key-frame animation dialog.
See the :ref:`animation section <usage.animation>` of this manual for more information on this topic.

.. seealso::

  :py:class:`ovito.modifiers.SliceModifier` (Python API)
