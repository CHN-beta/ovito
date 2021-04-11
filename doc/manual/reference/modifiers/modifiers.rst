.. _particles.modifiers:

Modifiers
=========

Modifiers are the basic building blocks for creating a :ref:`data pipeline <usage.modification_pipeline>` in OVITO.
Like tools in a toolbox, each modifier implements a very specific, well-defined type of operation or computation, and typically you will need to
combine several modifiers to accomplish more complex tasks.

**List of modifiers available in OVITO:**

.. table::
  :width: 100%
  :widths: 25 75

  ========================================================== ==========================================================
  Analysis
  =====================================================================================================================
  :ref:`particles.modifiers.atomic_strain`                   Calculates local strain tensors based on the relative motion of neighboring particles.
  :ref:`particles.modifiers.bond_analysis` |ovito-pro|       Computes bond angle and bond length distributions.
  :ref:`particles.modifiers.cluster_analysis`                Decomposes a particle system into clusters of particles.
  :ref:`particles.modifiers.coordination_analysis`           Determines the number of neighbors of each particle and computes the radial distribution function for the system.
  :ref:`particles.modifiers.dislocation_analysis`            Identifies dislocation defects in a crystal.
  :ref:`particles.modifiers.displacement_vectors`            Calculates the displacements of particles based on an initial and a deformed configuration.
  :ref:`particles.modifiers.elastic_strain`                  Calculates the atomic-level elastic strain tensors in crystalline systems.
  :ref:`particles.modifiers.grain_segmentation`              Determines the grain structure in a polycrystalline microstructure.
  :ref:`particles.modifiers.histogram`                       Computes the histogram of a property.
  :ref:`particles.modifiers.scatter_plot`                    Generates a scatter plot of two properties.
  :ref:`particles.modifiers.bin_and_reduce` |ovito-pro|      Aggregates a particle property over a one-, two- or three-dimensional bin grid.
  :ref:`particles.modifiers.correlation_function`            Calculates the spatial cross-correlation function between two particle properties.
  :ref:`particles.modifiers.time_averaging` |ovito-pro|      Computes the average of some time-dependent input quantity over the entire trajectory.
  :ref:`particles.modifiers.time_series` |ovito-pro|         Plots the value of a global attribute as function of simulation time.
  :ref:`particles.modifiers.voronoi_analysis`                Computes the coordination number, atomic volume, and Voronoi index of particles from their Voronoi polyhedra.
  :ref:`particles.modifiers.wigner_seitz_analysis`           Identifies point defects (vacancies and interstitials) in a crystal lattice.
  ========================================================== ==========================================================

.. table::
  :width: 100%
  :widths: 25 75

  ========================================================== ==========================================================
  Coloring
  =====================================================================================================================
  :ref:`particles.modifiers.ambient_occlusion`               Performs an ambient occlusion calculation to shade particles.
  :ref:`particles.modifiers.assign_color`                    Assigns a color to all selected elements.
  :ref:`particles.modifiers.color_by_type` |ovito-pro|       Colors particles or bonds according to a typed (discrete) property.
  :ref:`particles.modifiers.color_coding`                    Colors particles or bonds based on the value of a scalar (continuous) property.
  ========================================================== ==========================================================

.. table::
  :width: 100%
  :widths: 25 75

  ========================================================== ==========================================================
  Modification
  =====================================================================================================================
  :ref:`particles.modifiers.affine_transformation`           Applies an affine transformation to the system.
  :ref:`particles.modifiers.combine_particle_sets`           Merges the particles and bonds from two separate input files into one dataset.
  :ref:`particles.modifiers.compute_property`                Assigns property values to particles or bonds according to a user-defined formula.
  :ref:`particles.modifiers.delete_selected_particles`       Removes the selected elements from the visualization.
  :ref:`particles.modifiers.freeze_property`                 Freezes the values of a dynamic particle property at a given animation time to make them available at other times.
  :ref:`particles.modifiers.load_trajectory`                 Loads time-dependent atomic positions from a separate trajectory file.
  :ref:`particles.modifiers.python_script` |ovito-pro|       Lets you write your own modifier function in Python.
  :ref:`particles.modifiers.show_periodic_images`            Duplicates particles and other data elements to visualize periodic images of the system.
  :ref:`particles.modifiers.slice`                           Cuts the structure along an infinite plane.
  :ref:`particles.modifiers.smooth_trajectory`               Computes time-averaged particle positions using a sliding window or generates intermediate sub-frames using linear interpolation.
  :ref:`particles.modifiers.unwrap_trajectories`             Computes unwrapped particle coordinates in order to generate continuous trajectories at periodic cell boundaries.
  :ref:`particles.modifiers.wrap_at_periodic_boundaries`     Folds particles located outside of the periodic simulation box back into the box.
  ========================================================== ==========================================================

.. table::
  :width: 100%
  :widths: 25 75

  ========================================================== ==========================================================
  Selection
  =====================================================================================================================
  :ref:`particles.modifiers.clear_selection`                 Resets the selection state of all elements.
  :ref:`particles.modifiers.expand_selection`                Selects particles that are neighbors of already selected particles.
  :ref:`particles.modifiers.expression_select`               Selects particles and other elements based on a user-defined criterion.
  :ref:`particles.modifiers.manual_selection`                Lets you select individual particles or bonds with the mouse.
  :ref:`particles.modifiers.invert_selection`                Inverts the selection state of each element.
  :ref:`particles.modifiers.select_particle_type`            Selects all elements of a particular type, e.g. all atoms of a chemical species.
  ========================================================== ==========================================================

.. table::
  :width: 100%
  :widths: 25 75

  ========================================================== ==========================================================
  Structure identification
  =====================================================================================================================
  :ref:`particles.modifiers.bond_angle_analysis`             Identifies common crystal structures by an analysis of the bond-angle distribution.
  :ref:`particles.modifiers.centrosymmetry`                  Calculates the centrosymmetry parameter for every particle.
  :ref:`particles.modifiers.chill_plus`                      Identifies hexagonal ice, cubic ice, hydrate and other arrangements of water molecules.
  :ref:`particles.modifiers.common_neighbor_analysis`        Performs the common neighbor analysis (CNA) to determine local crystal structures.
  :ref:`particles.modifiers.identify_diamond_structure`      Identifies atoms that are arranged in a cubic or hexagonal diamond lattice.
  :ref:`particles.modifiers.polyhedral_template_matching`    Identifies common crystal structures using the PTM method and computes local crystal orientations.
  :ref:`particles.modifiers.vorotop_analysis`                Identifies local structure of particles using the topology of their Voronoi polyhedra.
  ========================================================== ==========================================================

.. table::
  :width: 100%
  :widths: 25 75

  ========================================================== ==========================================================
  Visualization
  =====================================================================================================================
  :ref:`particles.modifiers.construct_surface_mesh`          Constructs a triangle mesh representing the surface of a solid.
  :ref:`particles.modifiers.create_bonds`                    Creates bonds between particles.
  :ref:`particles.modifiers.create_isosurface`               Generates an isosurface from a scalar field.
  :ref:`particles.modifiers.coordination_polyhedra`          Shows coordination polyhedra.
  :ref:`particles.modifiers.generate_trajectory_lines`       Generates trajectory lines from the time-dependent particle positions.
  :ref:`particles.modifiers.interactive_molecular_dynamics`  Visualize live atomic trajectories from a running MD simulation as they are being calculated.
  ========================================================== ==========================================================

.. toctree::
  :maxdepth: 1
  :hidden:

  particles/atomic_strain
  Bond analysis <particles/bond_analysis>

.. particles/bond_angle_analysis
.. particles/affine_transformation
.. particles/ambient_occlusion
.. particles/assign_color
.. particles/centrosymmetry
.. particles/chill_plus
.. particles/clear_selection
.. particles/cluster_analysis
.. particles/color_by_type
.. particles/color_coding
.. particles/combine_particle_sets
.. particles/common_neighbor_analysis
.. particles/compute_property
.. particles/construct_surface_mesh
.. particles/coordination_analysis
.. particles/coordination_polyhedra
.. particles/correlation_function
.. particles/create_bonds
.. particles/create_isosurface
.. particles/delete_selected_particles
.. particles/dislocation_analysis
.. particles/displacement_vectors
.. particles/elastic_strain
.. particles/grain_segmentation
.. particles/expand_selection
.. particles/expression_select
.. particles/freeze_property
.. particles/generate_trajectory_lines
.. particles/histogram
.. particles/identify_diamond
.. particles/interactive_molecular_dynamics
.. particles/invert_selection
.. particles/load_trajectory
.. particles/manual_selection
.. particles/polyhedral_template_matching
.. particles/python_script
.. particles/show_periodic_images
.. particles/scatter_plot
.. particles/select_particle_type
.. particles/bin_and_reduce
.. particles/slice
.. particles/smooth_trajectory
.. particles/time_averaging
.. particles/time_series
.. particles/unwrap_trajectories
.. particles/voronoi_analysis
.. particles/vorotop_analysis
.. particles/wigner_seitz_analysis
.. particles/wrap_at_periodic_boundaries


.. _particles.modifiers.cluster_analysis:

Cluster analysis
--------------------------------------------

.. _particles.modifiers.coordination_analysis:

Coordination analysis
--------------------------------------------

.. _particles.modifiers.dislocation_analysis.fileformat:
.. _particles.modifiers.dislocation_analysis:

Dislocation analysis (DXA)
--------------------------------------------

.. _particles.modifiers.displacement_vectors:

Displacement vectors
--------------------------------------------

.. _particles.modifiers.elastic_strain:

Elastic strain calculation
--------------------------------------------

.. _particles.modifiers.grain_segmentation:

Grain segmentation
--------------------------------------------

.. _particles.modifiers.histogram:

Histogram
--------------------------------------------

.. _particles.modifiers.scatter_plot:

Scatter plot
--------------------------------------------

.. _particles.modifiers.bin_and_reduce:

Spatial binning |ovito-pro|
--------------------------------------------

.. _particles.modifiers.correlation_function:

Correlation function
--------------------------------------------

.. _particles.modifiers.time_averaging:

Time averaging |ovito-pro|
--------------------------------------------

.. _particles.modifiers.time_series:

Time series |ovito-pro|
--------------------------------------------

.. _particles.modifiers.voronoi_analysis:

Voronoi analysis
--------------------------------------------

.. _particles.modifiers.wigner_seitz_analysis:

Wigner-Seitz analysis
--------------------------------------------

.. _particles.modifiers.ambient_occlusion:

Ambient occlusion
--------------------------------------------

.. _particles.modifiers.assign_color:

Assign color
--------------------------------------------

.. _particles.modifiers.color_by_type:

Color by type |ovito-pro|
--------------------------------------------

.. _particles.modifiers.color_coding:

Color coding
--------------------------------------------

.. _particles.modifiers.affine_transformation:

Affine transformation
--------------------------------------------

.. _particles.modifiers.combine_particle_sets:

Combine datasets
--------------------------------------------

.. _particles.modifiers.compute_property:

Compute property
--------------------------------------------

.. _particles.modifiers.delete_selected_particles:

Delete selected
--------------------------------------------

.. _particles.modifiers.freeze_property:

Freeze property
--------------------------------------------

.. _particles.modifiers.load_trajectory:

Load trajectory
--------------------------------------------

.. _particles.modifiers.python_script:

Python script |ovito-pro|
--------------------------------------------

.. _particles.modifiers.show_periodic_images:

Replicate
--------------------------------------------

.. _particles.modifiers.slice:

Slice
--------------------------------------------

.. _particles.modifiers.smooth_trajectory:

Smooth trajectory
--------------------------------------------

.. _particles.modifiers.unwrap_trajectories:

Unwrap trajectories
--------------------------------------------

.. _particles.modifiers.wrap_at_periodic_boundaries:

Wrap at periodic boundaries
--------------------------------------------

.. _particles.modifiers.clear_selection:

Clear selection
--------------------------------------------

.. _particles.modifiers.expand_selection:               

Expand selection
--------------------------------------------

.. _particles.modifiers.expression_select:   

Expression selection
--------------------------------------------

.. _particles.modifiers.manual_selection:              

Slice
--------------------------------------------

.. _particles.modifiers.invert_selection:               

Invert selection
--------------------------------------------

.. _particles.modifiers.select_particle_type:          

Select type
--------------------------------------------

.. _particles.modifiers.bond_angle_analysis:             

Ackland-Jones analysis
--------------------------------------------

.. _particles.modifiers.centrosymmetry:                  

Slice
--------------------------------------------

.. _particles.modifiers.chill_plus:                      

Chill+
--------------------------------------------

.. _particles.modifiers.common_neighbor_analysis:        

Common neighbor analysis
--------------------------------------------

.. _particles.modifiers.identify_diamond_structure:      

Identify diamond structure
--------------------------------------------

.. _particles.modifiers.polyhedral_template_matching:    

Polyhedral template matching
--------------------------------------------

.. _particles.modifiers.vorotop_analysis:                

VoroTop analysis
--------------------------------------------

.. _particles.modifiers.construct_surface_mesh:          

Construct surface mesh
--------------------------------------------

.. _particles.modifiers.create_bonds:                    

Create bonds
--------------------------------------------

.. _particles.modifiers.create_isosurface:              

Create isosurface
--------------------------------------------

.. _particles.modifiers.coordination_polyhedra:          

Coordination polyhedra
--------------------------------------------

.. _particles.modifiers.generate_trajectory_lines:      

Generate trajectory lines
--------------------------------------------

.. _particles.modifiers.interactive_molecular_dynamics:  

Interactive molecular dynamics
--------------------------------------------

