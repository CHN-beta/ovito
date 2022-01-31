.. _file_formats.input.lammps_dump:
  
LAMMPS dump file reader
-----------------------

.. image:: /images/io/lammps_dump_reader.*
  :width: 30%
  :align: right

Reads trajectory files written by the `LAMMPS dump command <https://docs.lammps.org/dump.html>`__.

.. _file_formats.input.lammps_dump.variants:

Supported format variants
"""""""""""""""""""""""""

Handles files written by the following LAMMPS dump styles:

  - ``custom``, ``custom/gz``, ``custom/mpiio``
  - ``atom``, ``atom/gz``, ``atom/mpiio``

.. note::

  Dump styles ``cfg``, ``xyz``, ``local``, ``xtc`` and ``netcdf`` are handled by :ref:`different file format readers <file_formats.input>` of OVITO.

In addition to text dump files, OVITO supports reading *binary* dump files (".bin" suffix) and *gzipped* dump files (".gz" suffix).

OVITO can read trajectories from a single large dump file or from a sequence of smaller dump files 
(written by LAMMPS when the output filename contains a "*" wildcard character). It can even concatenate a long trajectory from 
several dump files, each containing multiple frames.

The current program version does *not* support loading sets of parallel dump files, written by LAMMPS when the output filename contains a "%" wildcard character.

.. _file_formats.input.lammps_dump.dump_modify:

Support for ``dump_modify`` options
"""""""""""""""""""""""""""""""""""

LAMMPS lets you configure the dump output through its `dump_modify command <https://docs.lammps.org/dump_modify.html>`__. 
OVITO provides support for the following dump_modify keywords:

  - The `time` keyword makes LAMMPS write the current elapsed simulation time to the dump file. 
    In OVITO, the time value of each trajectory frame is made available as a :ref:`global attribute <usage.global_attributes>` named "Time". 
    The timestep number, which is always present, is made available as global attribute "Timestep".

  - The `units` keyword makes LAMMPS write two extra lines "ITEM: UNITS" to the dump file header. 
    OVITO currently ignores this information, since the program has no internal unit system (all quantities are treated as being without units).

  - The `sort` keyword requests LAMMPS to output the list of atoms to the dump file in sorted order (by unique atom ID). 
    This option is normally not needed, because OVITO accepts an unsorted list of atoms and can handle trajectories in which the storage
    order of atoms varies throughout a simulation (which is common in LAMMPS). OVITO maintains the original order in which atoms appear in the dump file. 
    Unique atom IDs (if present as a separate column in the dump file) are used to identify individual atoms in different trajectory frames. 
    Nevertheless, if desired, the dump file reader provides a user option requesting OVITO to sort the atoms by ID during data import 
    (``sort_particles=True`` keyword parameter of the :py:func:`~ovito.io.import_file` Python function). This option makes the ordering of
    atoms stable within OVITO even if the `sort` keyword was not used at the time LAMMPS wrote the dump file. 

.. _file_formats.input.lammps_dump.property_mapping:

Column to property mapping
""""""""""""""""""""""""""

The data columns of a dump file get mapped to corresponding :ref:`particle properties <usage.particle_properties>` within OVITO during file import.
This happens automatically according to the following rules, but you can may manually override the mapping if necessary.

========================== ========================== =========================
LAMMPS column name         OVITO particle property    Comments
========================== ========================== =========================
x, y, z                    ``Position``
xu, yu, zu                 ``Position``  
xs, ys, zs                 ``Position``               Automatic conversion from reduced to Cartesian coordinates
xsu, ysu, zsu              ``Position``               Automatic conversion from reduced to Cartesian coordinates
id                         ``Particle Identifier``
vx, vy, vz                 ``Velocity``               Automatic computation of ``Velocity Magnitude`` property
type                       ``Particle Type``          
element                    ``Particle Type``          Named types (may be combined with numeric IDs from `type` column)
mass                       ``Mass``
radius                     ``Radius``
diameter                   ``Radius``                 Automatic division by 2
mol                        ``Molecule Identifier``    
q                          ``Charge``
ix, iy, iz                 ``Periodic Image`` 
fx, fy, fz                 ``Force``
mux, muy, muz              ``Dipole Orientation``
mu                         ``Dipole Magnitude``
omegax, omegay, omegaz     ``Angular Velocity``
angmomx, angmomy, angmomz  ``Angular Momentum``
tqx, tqy, tqz              ``Torque``
spin                       ``Spin``
c_epot                     ``Potential Energy``
c_kpot                     ``Kinetic Energy``
c_stress[1..6]             ``Stress Tensor``          Symmetric tensor components XX, YY, ZZ, XY, XZ, YZ
c_orient[1..4]             ``Orientation``            Quaternion components X, Y, Z, W (see :ref:`here <howto.aspherical_particles.orientation>`)
c_shape[1..3]              ``Aspherical Shape``       Principal semi-axes (see :ref:`here <howto.aspherical_particles.ellipsoids>`)
c_diameter[1..3]           ``Aspherical Shape``       Same as above but with automatic division by 2 (see :ref:`example <howto.aspherical_particles.orientation>`)
c_cna                      ``Structure Type``
pattern                    ``Structure Type``
selection                  ``Selection``
========================== ========================== =========================

Columns having any other name are mapped to a user-defined particle property with the same name.

.. _file_formats.input.lammps_dump.further_notes:

Further notes
"""""""""""""

- LAMMPS can perform 2d and 3d simulations (see `dimension <https://docs.lammps.org/dimension.html>`__ command) and OVITO can also treat a system 
  as either two- or three-dimensional (see :ref:`scene_objects.simulation_cell`). However, the dimensionality of a simulation is not encoded in the 
  dump file. OVITO assumes that the simulation is two-dimensional if the dump file contains no z-coordinates. You can override this after import if necessary.
