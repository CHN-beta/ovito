.. _particles.modifiers.wrap_at_periodic_boundaries:

Wrap at periodic boundaries
---------------------------

This modifier remaps particles that are located outside of the simulation box
back into the box by "wrapping" their coordinates around at
the boundaries of the simulation box.

.. figure:: /images/modifiers/wrap_at_periodic_boundaries_example_before.*
  :figwidth: 20%

  Input

.. figure:: /images/modifiers/wrap_at_periodic_boundaries_example_after.*
  :figwidth: 20%

  Output

The wrapping is only performed along those directions for which periodic
boundary conditions (PBC) are enabled for the simulation cell. The PBC flags are read from the
input simulation file if available and can be manually set in the :ref:`Simulation cell <scene_objects.simulation_cell>` panel.

.. seealso::
  
  :py:class:`ovito.modifiers.WrapPeriodicImagesModifier` (Python API)