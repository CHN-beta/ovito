.. _usage.particle_properties:

Particle properties
===================

:ref:`Particle properties <scene_objects.particles>` are attributes (typically numeric) associated with each individual particle.
They play a central role in the data model employed by OVITO to represent molecular and other structures. 
Common examples for particle properties are position, chemical type or velocity. In OVITO, it is possible for the user
to add any number of new properties to particles, either explicitly or as a result of computations performed within the program.
Note, however, that it is not possible to define a property only for some of the particles. At any time, all particles in a dataset always have the 
same uniform set of properties (but their individual *values* may differ, of course).

Note that this concept of uniform properties is very general and applies to other kinds of entities as well. For instance, in OVITO the interatomic bonds may be
associated with *bond properties*, e.g. the bond type or the bond color. So keep in mind that, even though the following introduction primarily focuses on particle properties,
it applies to other classes of data elements in the same manner.

OVITO allows you to associate particles with an arbitrary number of properties. Each property is identified by its unique name, for example ``Position`` or
``Potential Energy``. OVITO works with a built-in list of commonly-used property names such as the mentioned two, but
you are free to define new properties and give them names as needed. The ``Position`` property plays a special role though, because it is always present.
That's because particles cannot exist without a spatial position. Other predefined standard properties such as ``Color``, ``Radius`` or ``Selection``
have also a special meaning to the program, but they are option. Their values control how OVITO will render the particles. By assigning new values to these 
properties, you get control over the visual appearance of the particles.

In OVITO, per-particle property values can have different data types (real or integer) and dimensionality (e.g. scalar, vector, tensor). 
The ``Position`` property, for instance, is a vector property with three components per particle, referred to as 
``Position.X``, ``Position.Y`` and ``Position.Z`` within OVITO's user interface. 

How are properties assigned?
----------------------------

An initial set of properties is automatically created by OVITO whenever you open a simulation data file.
The values of standard properties such as ``Position``, ``Particle Type`` and ``Velocity``
are typically initialized from the data loaded from the imported simulation file. Some file formats such as *LAMMPS dump* and the `extended XYZ format <http://libatoms.github.io/QUIP/io.html#module-ase.io.extxyz>`_
can store an arbitrary number of extra data columns. 
These extra per-atom attributes are automatically mapped to corresponding particle properties in OVITO during file import.

.. figure:: /images/usage/properties/particle_inspection_example.*
   :figwidth: 50%
   :align: right

   Data inspector displaying the table of particle properties

To find out which properties are currently associated with the particles, you can open OVITO's :ref:`Data inspector <data_inspector>` panel, 
which is shown in the screenshot on the right. Alternatively, you can simply point the mouse cursor at a particle in the viewports to let OVITO display 
the values of its properties in the status bar.

OVITO provides a rich set of functions for modifying the properties of particles. These so-called *modifiers*
will be introduced in more detail in a following section of this manual. But to already give you a first idea of the principle:
The :ref:`Assign color <particles.modifiers.assign_color>` modifier function lets you assign a uniform color of your choice
to all currently selected particles. It does that by setting the ``Color`` property of the
particles to the given RGB value (if the ``Color`` property doesn't exist yet, it is automatically created). 
The subset of currently selected particles is determined by the ``Selection`` particle property: Particles whose ``Selection``
property has a non-zero value are part of the current selection set, while particles for which ``Selection=0`` are not selected.

Fittingly, OVITO provides a number of selection modifiers, which let you define a particle selection set by appropriately setting the values of the ``Selection`` property.
For example, the :ref:`Select type <particles.modifiers.select_particle_type>` modifier takes the ``Particle Type``
property of each particle to decide whether or not to select that particle. It allows you to select all atoms of a particular chemical type, for example,
and then perform some operation only on that subset of particles.

Another typical modifier is the :ref:`Coordination Analysis <particles.modifiers.coordination_analysis>` modifier.
It computes the number of neighbors of each particle within a given cutoff range and stores the computation results in a new particle property named ``Coordination``. 
Subsequently, you can refer to the values of this property, for example to select particles having a coordination number in a certain range
or to color particles based on their coordination number (see :ref:`Color Coding <particles.modifiers.color_coding>` modifier).

Of course it is possible to export the particle property values to an output file. OVITO supports a variety of output formats for that (see the 
:ref:`data export <usage.export>` section of this manual). For instance, the *XYZ* format is a simple table
format supporting an arbitrary set of output columns.

.. _usage.particle_properties.special:

Special particle properties
---------------------------

As mentioned above, certain particle properties play a special role in OVITO, because their values control the visual
appearance of the particles as well as other aspects. The following table lists these properties and describes their function:

========================= ========================== =======================================================================================
Particle property         Data type (components)     Description
========================= ========================== =======================================================================================
``Position``              Real (X, Y, Z)             The particle coordinates. For 2-dim. systems, the Z-component will be 0.
``Color``                 Real (R, G, B)             If present, this property controls the display color of particles. 
                                                     Red, green and blue components are in the range [0,1].
``Radius``                Real                       If present, this property controls the display size of particles.
``Particle Type``         Integer                    Stores the type identifier of each particle. This also determines the 
                                                     display size and color  if the ``Radius`` or ``Color`` property are not present.
``Particle Identifier``   Integer                    Stores the unique ID of each particle. This information will be used by some 
                                                     modifiers to track particles over time even if the storage order changes.
``Transparency``          Real                       A value in the range [0,1] controlling the particle's transparency. 
                                                     If not present, particles are rendered fully opaque.
``Selection``             Integer                    Stores the current selection state of particles (1 for selected particles; 0 otherwise).
========================= ========================== =======================================================================================

.. _usage.bond_properties.special:

Special bond properties
---------------------------

========================= ========================== =======================================================================================
Bond property             Data type (components)     Description
========================= ========================== =======================================================================================
``Topology``              Integer (A, B)             This bond property is always present and holds the indices of the two particles 
                                                     connected by a bond.
``Bond Type``             Integer                    Stores the type identifier of each bond. The bond type determines the display color 
                                                     if the ``Color`` property is not present.
``Color``                 Real (R, G, B)             If present, this property controls the display color of individual bonds. 
                                                     Red, green and blue components are in the range [0,1].
``Transparency``          Real                       A value in the range [0,1] controlling the bonds's transparency. 
                                                     If not present, bonds are rendered fully opaque.
``Selection``             Integer                    Stores the current selection state of bonds (1 for selected bonds; 0 otherwise).
========================= ========================== =======================================================================================

.. _scene_objects.particles:
.. _particles.modifiers.assign_color:
.. _particles.modifiers.coordination_analysis:
.. _particles.modifiers.color_coding:
.. _particles.modifiers.select_particle_type:
.. _usage.export: