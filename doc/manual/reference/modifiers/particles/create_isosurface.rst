.. _particles.modifiers.create_isosurface:

Create isosurface
-----------------

.. image:: /images/modifiers/create_isosurface_panel.png
  :width: 30%
  :align: right

.. figure:: /images/modifiers/create_isosurface_example.png
  :figwidth: 30%
  :align: right
  
  Two isosurfaces of the charge density field

This modifier generates an `isosurface <https://en.wikipedia.org/wiki/Isosurface>`__ for a field quantity defined on a structured 
:ref:`voxel grid <scene_objects.voxel_grid>`.
The computed isosurface is a :ref:`surface mesh <scene_objects.surface_mesh>` data object and 
its visual appearance is controlled by the accompanying :ref:`surface mesh <visual_elements.surface_mesh>` visual element.

See the :ref:`list of input file formats <file_formats.input>` supported by OVITO to find out how to import
voxel grids into the program. You can also apply the isosurface modifier to a dynamically generated voxel grid produced by the
:ref:`particles.modifiers.bin_and_reduce` modifier.

Note that you can apply this modifier several times in a pipeline to create multiple surfaces at different iso-levels.

The option :guilabel:`Transfer field values to surface` lets the modifier copy all field quantities defined on the voxel grid over to the isosurface's vertices.
This includes any secondary field quantities in addition to the selected primary field quantity for which the isosurface is being constructed, and which is constant and equal to the iso-level value on the
entire surface. Subsequently, OVITO's :ref:`particles.modifiers.color_coding` modifier may be used to color the isosurface based on a secondary 
field quantity. The locally varying values for a secondary field quantity on the surface are computed at each mesh vertex using trilinear interpolation from the voxel grid values. 

.. seealso::
  
  :py:class:`ovito.modifiers.CreateIsosurfaceModifier` (Python API)