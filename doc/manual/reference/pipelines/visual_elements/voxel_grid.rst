.. _visual_elements.voxel_grid:

Voxel grid
----------

.. image:: /images/visual_elements/voxel_grid_panel.png
  :width: 30%
  :align: right

This type of :ref:`visual element <visual_elements>` controls the visual appearance of 
:ref:`voxel grid <scene_objects.voxel_grid>` data objects, which are structured grids made of 
2- or 3-dimensional cells (voxels), each associated with one or more numeric values.

.. image:: /images/visual_elements/voxel_grid_example.png
  :width: 30%
  :align: right

This visual element renders the voxel grid as a solid object showing just the grid cells on the outer 
boundaries of the domain. Note that, by default, the voxel cells will all appear gray, because they do not have a 
color assigned to them yet. You can use the :ref:`particles.modifiers.color_coding` modifier
to given them a color and visualize a numeric field quantity associated with the grid cells as in this example.

.. image:: /images/visual_elements/voxel_grid_example_interpolated.png
  :width: 30%
  :align: right

Parameters
""""""""""

Transparency
  The degree of semi-transparency to use when rendering the grid surfaces.

Highlight edges
  Activates the rendering of wireframe lines along the edges of the grid cells.

Interpolate colors
  Will smoothly interpolate between the discrete colors of adjacent cells.

.. seealso::

  :py:class:`ovito.vis.VoxelGridVis` (Python API)