.. _usage.import:

Data import
===========

.. figure:: /images/scene_objects/file_wildcard_pattern.*
   :figwidth: 30%
   :align: right
   
   File source panel

To load a simulation file from your local computer, select :guilabel:`Load File` from the menu or use the corresponding button in the toolbar.
OVITO detects the format of the file automatically (see :ref:`list of supported formats <file_formats.input>`).
Compressed text-based files with a :file:`.gz` suffix can be read directly by OVITO.

The imported dataset will be visible in the 3d viewports as a visual object
and also appears under the `Data source` section of the :ref:`pipeline editor <usage.modification_pipeline.pipeline_listbox>` (see screenshot on the right).

The :ref:`External file <scene_objects.file_source>` panel is displayed below the pipeline editor.
It contains tool buttons for reloading the imported data file in case it was changed outside of OVITO, or for picking a different file as 
source of the :ref:`data pipeline <usage.modification_pipeline>`. Switching the imported file is useful when you have set up a 
complex data analysis pipeline and would like to reuse it on a different simulation dataset.

.. _usage.import.command_line:

Command line usage
------------------

If you launch OVITO from the terminal, you can directly specify the file(s) to load. This works both for local and :ref:`remote files <usage.import.remote>`::

  ovito /path/filename
  ovito sftp://hostname/path/filename
  ovito https://www.website.org/path/filename

Importing several files at once by specifying multiple path is possible. 
If they all have the same data format, they will be concatenated into an animatable trajectory.
If they have different formats, OVITO will detect whether they represent a pair of topology/trajectory files (see next section).
Otherwise, they will be inserted as :ref:`separate objects into the scene <usage.import.multiple_datasets>`. 

.. _usage.import.sequence:

Simulation trajectories
-----------------------

OVITO can load simulation trajectories consisting of a series of snapshots. Various cases are supported by the software:

A series of separate files:
  By default, whenever you import a new simulation file, OVITO tries to detect if the file is part of a numbered sequence of files
  with similar names in the same directory. To this end, the last numeric character sequence (if any) in the filename you picked is replaced with the wildcard
  character ``*`` to generate a search pattern, which will subsequently be used to look up more files in the same directory belonging to the trajectory sequence.
  For instance, if you opened a file named :file:`anim1c_5000.dump`, OVITO will automatically generate the search pattern
  :file:`anim1c_*.dump` to find more snapshots (e.g. :file:`anim1c_0.dump`, :file:`anim1c_1000.dump`, :file:`anim1c_2000.dump`, etc). It is possible to
  manually override the generated file pattern in the input field highlighted in the screenshot or to turn off the 
  automatic discovery of file sequences by unchecking the :guilabel:`auto-generate` box.

One file containing all trajectory frames:
  OVITO automatically detects whether the imported file contains more than one simulation frame and loads all of them as an animation sequence.
  For some file types, e.g. XYZ and LAMMPS dump, this is indicated by the :guilabel:`Contains multiple timesteps`
  checkbox highlighted in the screenshot. Note that OVITO typically keeps only the data of a single frame in memory at a time.
  Subsequent frames are loaded into memory only as needed, for example if you play back the animation or move the time slider.

A pair of topology and trajectory files:
  Some MD simulation codes use separate files for the topology and the trajectory of a molecular structure. The topology file contains the static definition of
  atoms, bonds, etc. while the trajectory file contains the computed trajectories and other time-dependent data generated in the MD simulation.
  In such a case you should pick both files in the file selection dialog and import them simultaneously. OVITO recognizes automatically which 
  of the file is the topology file and which one is the trajectory file based on the following table:

  ============================== ======================
  Topology format                Trajectory format
  ============================== ======================
  LAMMPS data                    LAMMPS dump
  Gromacs GRO                    Gromacs XTC
  CASTEP cell                    CASTEP md/geom
  *any other supported format*   XYZ
  ============================== ======================

  The topology file will be loaded first (e.g. a LAMMPS *data* file) and a :ref:`particles.modifiers.load_trajectory` modifier 
  will be inserted into the data pipeline to load the time-dependent atomic positions
  from the trajectory file (e.g. a LAMMPS *dump* file). This modifier merges both pieces of information -the static topology and the dynamic trajectory data- into a single animated dataset.

OVITO will display a timeline and a time slider below the viewports if a simulation sequence with more than one frame
has been loaded. See the section :ref:`usage.animation` to learn more about OVITO's advanced animation capabilities.

.. _usage.import.multiple_datasets:

Visualizing multiple datasets
=============================

.. figure:: /images/usage/importexport/datasets_side_by_side.*
   :figwidth: 40%
   :align: right

   Side-by-side visualization example

OVITO has the capability to manage several objects in the same three-dimensional scene.
This enables you to import and visualize several datasets together in a single picture as shown in the example on the right.
You can also visualize a dataset in several different ways, either side by side or superimposed on each other,
using :ref:`branched data pipelines <clone_pipeline>`, which dynamically duplicate the imported data and process each copy in a 
slightly different way.

The simplest way to visualize multiple datasets in one picture is to invoke the
:menuselection:`File --> Load File` function from the menu several times to import all datasets into the same scene.
When importing the second dataset, OVITO will ask you whether to replace the already loaded dataset or not.
Pick the :guilabel:`Add to scene` option here in order to insert it as an additional object into the existing scene.

.. figure:: /images/usage/importexport/pipeline_selector.*
   :figwidth: 60%
   :align: right

   The pipeline selector widget in OVITO's toolbar

The *pipeline selector* widget, located in the toolbar of the window (see screenshot), lists all datasets and other objects 
that are part of the current scene. Each imported dataset is associated with its own data pipeline. Thus, you can apply different modifiers
to each dataset. The data pipeline of the currently selected dataset is the one being displayed and edited in the
:ref:`pipeline editor <usage.modification_pipeline.pipeline_listbox>` in the panel on the right.

.. _usage.import.positioning_objects:

Positioning datasets in the scene
---------------------------------

OVITO places imported datasets in a default position relative
to the scene's global coordinate system. Thus, when loading the second dataset into the same scene,
it will appear superimposed in the same spatial location as the first dataset, which may not be what you want.

In order to correct this, you can move the individual objects around in the scene and arrange them as needed
for your visualization. In the example picture at the top of this page the second dataset has been translated along the x-axis
to place it next to the first dataset. To do this, use the *Translate* mode, which
is found in the top toolbar above the viewports:

.. image:: /images/usage/importexport/translate_tool.*
   :width: 40%

.. image:: /images/usage/importexport/translate_tool_numeric_fields.*
   :width: 40%

While the *Translate* mode is active, you can move objects around in the viewports
using the mouse. Alternatively, you can enter the desired position of the selected object numerically using the
input fields displayed in the status bar while the *Translate* mode is selected.

Cloning pipelines
-----------------

Instead of importing several data files into OVITO, you can also duplicate a dataset within OVITO in order
to visualize the same data in different ways, for example by applying different sets of modifiers to each replica
of the dataset. See the :ref:`Clone Pipeline <clone_pipeline>` function for more information.

