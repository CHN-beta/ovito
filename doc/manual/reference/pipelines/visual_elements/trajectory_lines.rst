.. _visual_elements.trajectory_lines:

Trajectory lines
----------------

.. image:: /images/visual_elements/trajectory_lines_panel.jpg
   :width: 30%
   :align: right

.. image:: /images/scene_objects/trajectory_lines_example.gif
   :width: 30%
   :align: right

This :ref:`visual element <visual_elements>` renders continuous lines to visualize the
trajectories of motion of particles. The visual element is automatically created by the
:ref:`particles.modifiers.generate_trajectory_lines` modifier, which samples the particle positions
to generate the lines.

The option :guilabel:`Wrap trajectory lines around` folds trajectory lines leaving the periodic simulation
cell back into the cell. Note that this option can only yield correct results if the simulation cell size
does not change with time (fixed box dimensions).

The option :guilabel:`Show up to current time only` restricts rendering of trajectories lines to
those parts which have already been traversed by the particles up to the current animation time. Thus, the trajectory lines will
be gradually drawn while playing the animation, as in the examples shown on the right.

Line coloring
"""""""""""""

By default, the trajectory lines are all rendered with the same uniform color. Alternatively, you 
can activate the :guilabel:`Color mapping` option, which tells the visual element to visualize a local property 
along the trajectory lines using a pseudo-color map. When this option is selected,
OVITO shows a separate color mapping panel, which lets you pick the source property for the pseudo-coloring
and configure the color transfer function.

The source property for the line pseudo-coloring can be adopted from the particles the trajectory lines are based on. For this, make sure
to activate the :guilabel:`Sample a particle property` option in the :ref:`particles.modifiers.generate_trajectory_lines` modifier.
This option copies the property values at each timestep from the particles to the corresponding vertices of the trajectory lines.

.. seealso::

   :py:class:`ovito.vis.TrajectoryVis` (Python API)