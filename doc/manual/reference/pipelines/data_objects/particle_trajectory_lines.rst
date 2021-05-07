.. _scene_objects.particle_trajectory_lines:

Trajectory lines
----------------

.. image:: /images/scene_objects/trajectory_lines_example.gif
  :width: 35%
  :align: right

A trajectory lines :ref:`data object <scene_objects>` stores the continuous motion paths of a set of particles,
which have been precomputed by a :ref:`particles.modifiers.generate_trajectory_lines` modifier
for the purpose of visualization. 

The visual appearance of the trajectory lines in rendered images is controlled through the associated 
:ref:`Trajectory lines <visual_elements.trajectory_lines>` visual element.

.. seealso::
  
  :py:class:`ovito.data.TrajectoryLines` (Python API)