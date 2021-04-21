.. _visual_elements.trajectory_lines:

Trajectory lines
----------------

.. image:: /images/visual_elements/trajectory_lines_panel.png
   :width: 30%
   :align: right

.. image:: /images/scene_objects/trajectory_lines_example.gif
   :width: 30%
   :align: right

This :ref:`visual element <visual_elements>` renders a set of continuous lines to visualize the
trajectories of motion of particles. Normally, the element is automatically created by the program when you apply the
:ref:`particles.modifiers.generate_trajectory_lines` modifier.

The option :guilabel:`Wrap trajectory lines` folds trajectory lines leaving the periodic simulation
cell back into the cell. Note that this option will yield correct results only if the simulation cell size
is not changing with time.

The option :guilabel:`Show up to current time only` restricts rendering of trajectories lines to
those parts that have been traversed by particles at the current animation time. Thus, the trajectory lines will
get gradually drawn while playing the animation, just as in the examples seen here and on :ref:`this page <particles.modifiers.generate_trajectory_lines>`.

.. seealso::

   :py:class:`ovito.vis.TrajectoryVis` (Python API)