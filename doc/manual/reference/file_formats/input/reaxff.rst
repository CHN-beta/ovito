.. _file_formats.input.reaxff:
  
ReaxFF file reader
------------------

Loads interatomic bonds and their properties from text files written by the LAMMPS `fix reaxff/bonds <https://docs.lammps.org/fix_reaxff_bonds.html>`__ command.

This file reader is typically used in conjunction with a :ref:`particles.modifiers.load_trajectory` modifier to amend an already loaded 
particle model with a varying list of bonds from a reactive MD simulation.

In addition to the time-dependent :ref:`bond connectivity <scene_objects.bonds>`, the ReaxFF reader loads the particle properties
``Charge``, ``Atom Bond Order``, and ``Lone Pairs`` from the selected file and attaches them to the existing atomic model.

Furthermore, the property ``Bond Order`` is assigned to the :ref:`bonds <scene_objects.bonds>` as a new attribute.

This reader is able to load gzipped ReaxFF files (".gz" suffix). 