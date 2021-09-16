.. _particles.modifiers.show_periodic_images:

Replicate
---------

.. image:: /images/modifiers/show_periodic_images_panel.png
  :width: 35%
  :align: right

This modifier copies all particles, bonds and other data elements multiple times to visualize periodic images of a system.

The :guilabel:`Operate on` list in the lower panel lets you select the types of data elements that
should be replicated by the modifier. By default, the modifier extends the simulation cell appropriately to
encompass all generated images of the system. If not desired, you can turn off the option :guilabel:`Adjust simulation box size`
to keep the original simulation cell geometry. You should be aware, however, that this produces an inconsistent state, where the 
periodicity length no longer fits to the explicitly replicated contents of the simulation cell. 

Parameters
""""""""""

Number of images - X/Y/Z
  These values control how many times the system is copied along each simulation cell vector.

Adjust simulation box size
  Extends the simulation cell to match the size of the replicated system.

Assign unique IDs
  This option lets the modifier assign new unique IDs to the copied particles or bonds.
  Otherwise the duplicated elements will have the same identifiers as the originals, which
  may cause problems with other modifiers (e.g. the :ref:`particles.modifiers.manual_selection` modifier), which
  rely on the uniqueness of identifiers.

  For replicated particles, this option also adjusts their ``Molecule Identifier`` property if it exists.

.. seealso::

  :py:class:`ovito.modifiers.ReplicateModifier` (Python API)