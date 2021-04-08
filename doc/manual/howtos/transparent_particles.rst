.. _howto.transparent_particles:

Semi-transparent particles
==========================

.. image:: /images/howtos/semitransparent_particles.*
   :width: 40%
   :align: right

Particles can be made semi-transparent by setting their ``Transparency`` :ref:`particle property <usage.particle_properties.special>`.
A ``Transparency`` value of 0.0 lets a particle appear fully opaque, while any value in the range 0.0-1.0 renders the particle translucent.

The easiest way to set the ``Transparency`` property is the :ref:`Compute property <particles.modifiers.compute_property>` modifier.
Simply enter the desired transparency value into the expression field and the :ref:`Compute property <particles.modifiers.compute_property>` 
modifier will assign that value to all particles.

Sometimes you want to make only a subset of the particles semi-transparent. For this, first select the particles, 
then apply the :ref:`Compute property <particles.modifiers.compute_property>` modifier. 
Activate its :guilabel:`Compute only for selected particles` option to restrict the assignment of the 
``Transparency`` value to the currently selected particles only. Unselected particles will keep their transparency values (0 by default).

For demonstration purposes, in the example shown on the right, the :ref:`Compute property <particles.modifiers.compute_property>` 
modifier was used to set the ``Transparency`` property according to a simple math formula: ``ReducedPosition.X``.
Thus, instead of assigning a uniform transparency value to all particles, each particle's value is computed
individually from its X position divided by the simulation cell size, 
which varies between 0 to 1 from one side of the simulation box to the other.

.. _particles.modifiers.compute_property: