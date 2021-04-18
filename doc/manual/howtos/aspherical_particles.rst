.. _howto.aspherical_particles:

Non-spherical particle shapes
=============================

.. figure:: /images/howtos/ellipsoid_particles_example1.*
   :figwidth: 25%
   :align: right
   
   Ellipsoid particles

OVITO can visualize particles with ellipsoid and box shapes as shown in the pictures on the right.
In addition, particles with cylindrical and spherocylindrical (capsule) shape can be visualized.

While the size of standard spherical and cubic particles is controlled by the Radius particle property,
the size and shape of non-spherical and non-cubic particles is determined by the ``Aspherical Shape`` property.
For OVITO to display non-spherical particles, this property must be defined and the
desired shape must be selected in the settings of the :ref:`Particles <visual_elements.particles>` visual element.

The ``Aspherical Shape`` particle property consists of three components ``X``, ``Y``, and ``Z``
which specify the size (radius) of ellipsoidal or box particles along the three principal axes.
Particles for which all three components of the `Aspherical Shape` property are zero, are rendered
as standard spheres (or cubes). In this case their size is controlled by the ``Radius`` property, as if the ``Aspherical Shape`` property was not defined.

.. figure:: /images/howtos/spherocylinder_particles_example1.*
   :figwidth: 25%
   :align: right
   
   Spherocylindrical particles

The size of cylindrical and spherocylindrical particles is also determined by the `Aspherical Shape` property.
Here, the ``X`` component specifies the radius of the cylinder and the ``Z`` component specifies its length
(along the local Z axis). The ``Y`` component is ignored.

The orientation of aspherical particles is controlled by the ``Orientation`` particle property,
which has four components ``X``, ``Y``, ``Z``, ``W`` that form
a quaternion. If the ``Orientation`` property is not defined, or if the four components do not form a unit quaternion,
no rotation is applied to an aspherical particle and it remains aligned with the coordinate axes.

.. figure:: /images/howtos/box_particles_example1.*
   :figwidth: 25%
   :align: right
   
   Box-shaped particles

Both the ``Aspherical Shape`` and the ``Orientation`` properties are typically
read from simulation files. The LAMMPS and LIGGGHTS simulation codes can output this information to dump files using the following LAMMPS script commands:

:: 

  compute orient all property/atom quati quatj quatk quatw
  compute shape all property/atom shapex shapey shapez
  dump 1 all custom 100 ellipsoid.dump id type x y z &
      c_orient[1] c_orient[2] c_orient[3] c_orient[4] &
      c_shape[1] c_shape[2] c_shape[3]

The ``quati``, ``quatj``, ``quatj``, and ``quatw``
atom properties of LAMMPS need to be mapped to the ``Orientation.X``, ``Orientation.Y``,
``Orientation.Z``, and ``Orientation.W`` properties of OVITO when importing
the dump file. Similarly, the ``shapex``, ``shapey``, and ``shapez`` columns
need to be mapped to ``Aspherical Shape.X``, ``Aspherical Shape.Y``, and ``Aspherical Shape.Z``
in OVITO. Since these properties are output as ``c_orient*`` and ``c_shape*`` by the
dump command above, OVITO cannot set up this mapping automatically. You have to do it manually by using the
:guilabel:`Edit column mapping` button in the file import panel after loading the dump file.
