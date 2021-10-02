.. _visual_elements.vectors:

Vectors
-------

.. image:: /images/visual_elements/vectors_panel.jpg
  :width: 35%
  :align: right

This :ref:`visual element <visual_elements>` visualize a vectorial property associated with the particles using arrow glyphs.
For example, the particle property ``Displacement``, which is computed by the :ref:`particles.modifiers.displacement_vectors` modifier, 
has an attached visual element, which can be found under the "Visual elements" section of the :ref:`pipeline editor <usage.modification_pipeline.pipeline_listbox>` (see screenshot).
This visual element provides controls for the appearance of the arrow glyphs. 

Parameters
""""""""""

Scaling factor
  All arrow lengths get scaled by this factor. The default scaling factor is 1.0, which means the lengths of the rendered glyphs reflect the original three-dimensional vectors.

Arrow width
  The width of the arrow glyphs (in simulation units of length).

Alignment
  Controls how the arrow glyphs are positioned relative to the particles they belong to.

Reverse direction
  Flips the direction of all arrows, swapping their heads and tails.

Flat shading
  This option switches from three-dimensional (solid) arrow glyphs to two-dimensional (flat) glyphs.

Coloring
  You can choose between uniform coloring of the all arrows and pseudo-coloring of the arrow glyphs
  based on some scalar property of the particles. When you activate the :guilabel:`Color mapping` option,
  OVITO shows a separate panel letting you pick the source property for the pseudo-coloring
  and configure the color transfer function.

  .. hint:: 
  
    A third method of coloring the arrows is to assign explicit RGB color values to the ``Vector Color`` property of the particles.
    This approach gives you full control over the color of each individual arrow. You can set the ``Vector Color`` property
    using the :ref:`particles.modifiers.color_coding` or :ref:`particles.modifiers.compute_property` modifiers, for example.

Transparency
  The degree of semi-transparency of the arrows. 
  
Offset (XYZ)
  An optional three-dimensional offset vector by which all arrow glyphs are displaced. The offset can be used to move the 
  arrows fully in front of the particles and avoid undesirable occlusions.

  .. image:: /images/visual_elements/vector_offset_example.png
    :width: 75%


.. seealso::

  :py:class:`ovito.vis.VectorVis` (Python API)