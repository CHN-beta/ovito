.. _scene_objects.particles:

Particles
---------

A system of *N* particles is represented in OVITO as a group of :ref:`particle properties <usage.particle_properties>`,
each being a uniform data array of length *N*:

.. image:: /images/scene_objects/particles_data_model.png
  :width: 85%

The property array with the name ``Position`` is always part of the particle dataset and contains the Cartesian coordinates of the particles.
All other properties are optional. Whether they are present or not depends on the kind of simulation file you import
and the :ref:`modifiers <usage.modification_pipeline>` you apply to the dataset within OVITO. That's because modifiers may add 
new particle properties to the set, e.g., to store the results of a computation they perform.
 
You can open the :ref:`data inspector <data_inspector>` panel of OVITO to view all particle properties that currently 
exist in the output of the data pipeline. 

.. _usage.particle_properties.special:

Special particle properties
"""""""""""""""""""""""""""

Certain particle properties play a special role in OVITO, because their values control the visual
appearance of the particles as well as other aspects. The following table lists these properties and describes their respective functions:

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

.. _scene_objects.particle_types:

Typed properties
""""""""""""""""

.. image:: /images/scene_objects/particle_types_panel.png
  :width: 30%
  :align: right

A *typed* particle property is a property array containing discrete numeric 
values and a supplementary mapping of these numeric values to corresponding type definitions. 
The ``Particle Type`` property is a typical example for such a typed property. 
It stores each particle's chemical type encoded as a unique integer value (1, 2, 3, ...), the so-called numeric type identifier.
Additionally, the ``Particle Type`` property stores a list of records defining the types, 
which establishes a mapping between the numeric type ID of each particle and the auxiliary information 
associated with that type, e.g. its name and display color:
      
.. image:: /images/scene_objects/typed_property.png
  :width: 55%
      
Note that a dataset may not just contain a single typed property like the ``Particle Type`` property. 
In fact, several typed properties can exist simultaneously, establishing several orthogonal classifications.
Examples are the particle properties ``Residue Type``, ``Structure Type``, and ``Molecule Type``.

All typed properties read from an imported simulation file are accessible in the pipeline editor
as shown in the screenshot on the right. Here you can edit each type's attributes. In case of the ``Particle Type``
property, these settings directly :ref:`affect how OVITO renders the particles <visual_elements.particles>` belonging to the type.

Particle types named after one of the standard chemical elements get automatically initialized with appropriate default values for
the display color, display radius, van der Waals radius, and mass. If necessary, you can change the default values permanently 
for each type using the corresponding presets menus indicated in the screenshot. You can even specify default parameters for particle
types having generic names such as "Type 1", "Type 2", etc., which may be necessary if the imported simulation file contains numeric type
information but no type names.

.. seealso::

  * :py:class:`ovito.data.Particles` (Python API)
  * :py:class:`ovito.data.Property` (Python API)
  * :py:attr:`ovito.data.Property.types` (Python API)