.. _visual_elements.surface_mesh:

Surface mesh
------------

.. image:: /images/visual_elements/surface_mesh_panel.png
  :width: 30%
  :align: right

This :ref:`visual element <visual_elements>` controls the appearance of triangulated surface meshes,
which are produced by e.g the :ref:`particles.modifiers.construct_surface_mesh`,
:ref:`particles.modifiers.create_isosurface` or :ref:`particles.modifiers.dislocation_analysis` modifiers.

.. image:: /images/visual_elements/surface_mesh_example.png
  :width: 30%
  :align: right

:ref:`Surfaces meshes <scene_objects.surface_mesh>` are closed two-dimensional manifolds which are embedded in a spatial domain delimited by the
simulation box. If periodic boundary conditions are used, the surface mesh itself may be periodic. For visualization purposes, a periodic mesh is
first converted to a non-periodic representation by truncating it at the simulation cell boundaries. OVITO can generate so called
*cap polygons* to fill the holes that would otherwise appear at the intersection of the surface with a periodic boundary of the simulation cell.

Parameters
""""""""""

Surface color
  The color for the surface.  

Transparency
  The degree of transparency of the surface.  

Smooth shading  
  Turns on interpolation of face normals to make the polygonal mesh appear smooth instead of faceted.  

Highlight edges  
  Activates the rendering of wireframe lines along the edges of the mesh facets.  

Cap polygons  
  This option enables the display of the cap polygons at the intersection of the surface with periodic cell boundaries.  

Inside out  
  Reverses the orientation sense of the surface and generates cap polygons on the outside, not inside of the
  enclosed volume.
    
.. seealso::

  :py:class:`ovito.vis.SurfaceMeshVis` (Python API)