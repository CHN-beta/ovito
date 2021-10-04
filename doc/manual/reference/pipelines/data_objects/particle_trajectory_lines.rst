.. _scene_objects.particle_trajectory_lines:

Trajectory lines
----------------

.. image:: /images/scene_objects/trajectory_lines_example.gif
  :width: 35%
  :align: right

A trajectory lines :ref:`data object <scene_objects>` stores the continuous paths of motion of a set of particles,
which have been recorded by the :ref:`particles.modifiers.generate_trajectory_lines` modifier
by sampling the current positions of the particles over time. 

The visual appearance of the trajectory lines in rendered images is controlled by the associated 
:ref:`Trajectory lines <visual_elements.trajectory_lines>` visual element.

.. seealso::
  
  :py:class:`ovito.data.TrajectoryLines` (Python API)