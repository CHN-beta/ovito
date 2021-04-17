.. _visual_elements.bonds:

Bonds
-----

.. image:: /images/visual_elements/bonds_panel.png
  :width: 30%
  :align: right

This :ref:`visual element <visual_elements>` renders bonds between pairs of particles.
:ref:`Bonds <scene_objects.bonds>` can either be loaded from the imported simulation file, or they can be created within OVITO, e.g. using the
:ref:`particles.modifiers.create_bonds` modifier.

Parameters
""""""""""

Shading mode
  Switches between a three-dimensional display mode, where bonds are rendered as cylinders, and a flat
  representation, where bonds are rendered as lines.  

Bond width
  Controls the display width of bonds (both cylinders and lines).  

Bond color  
  The rendering color of bonds. This color parameter is only used if the option :guilabel:`Use particle colors` is turned off.  

Use particle colors  
  If active, bonds are rendered in the same colors as the particles they connect.  

.. seealso::

  :py:class:`ovito.vis.BondsVis` (Python API)