.. _visual_elements.vectors:

Vectors
-------

.. image:: /images/visual_elements/vectors_panel.png
  :width: 35%
  :align: right

This :ref:`visual element <visual_elements>` visualizes vectorial particle properties as arrows.
The :ref:`particles.modifiers.displacement_vectors` modifier, for example,
outputs the calculated displacement vector of each particle as a new particle property named ``Displacement``.
This property is associated with a corresponding `Vectors` visual element,
which appears under the "Visual elements" section of the :ref:`pipeline editor <usage.modification_pipeline.pipeline_listbox>` (see screenshot),
and which controls the visualization of the vector data as arrow glyphs.

Arrow coloring
""""""""""""""

Note that, by default, all arrows are rendered in the same uniform color. If desired, however, you can give each arrow an individual
color by setting the ``Vector Color`` particle property, for example using the :ref:`particles.modifiers.color_coding` or
:ref:`particles.modifiers.compute_property` modifiers.

Parameters
""""""""""

Shading mode
  Selects between a three-dimensional (:guilabel:`Normal`) and a two-dimensional (:guilabel:`Flat`)
  rendering of of arrow glyphs.

Scaling factor
  The arrow length is scaled by this factor. The default scaling factor is 1.0, which means the arrow length exactly matches the original three-dimensional vector.

Arrow width
  The diameter or width of the arrow lines (in simulation units of length).

Alignment
  Controls how arrows are positioned relative to the particles they are associated with.

Reverse direction
  Flips the direction of all arrows, swapping their heads and tails.

Arrow color
  The display color of arrows. This uniform color is used when the ``Vector Color``
  particle property is not present. Otherwise the per-particle colors defined by the ``Vector Color`` property
  are used to render the arrows.

Transparency
  The degree of semi-transparency of the arrows. 
  
Offset
  Three-dimensional offset by which all arrows should be displaced. The offset parameter can be used to move the 
  arrows fully in front of or behind the particles to avoid undesirable occlusions.

.. seealso::

  :py:class:`ovito.vis.VectorVis` (Python API)