Importing data
==============

.. figure:: /images/scene_objects/file_wildcard_pattern.*
   :figwidth: 30%
   :align: right
   
   File source panel

To load a simulation file from your local computer, select :guilabel:`Load File` from the menu or use the corresponding button in the toolbar.
OVITO detects the format of the file automatically (see :ref:`list of supported formats <file_formats.input>`).
Compressed text-based files with a :file:`.gz` suffix can be read directly by OVITO.

The imported dataset will appear in the viewports as a new three-dimensional object
and also as an entry in the "Data source" section of the :ref:`pipeline editor <usage.modification_pipeline.pipeline_listbox>`, as indicated in the screenshot on the right.

If you select this item in the list, the :ref:`External File <scene_objects.file_source>` panel appears below the pipeline editor.
The tool buttons at the top of that panel let you reload
the imported input file in case it has been changed or rewritten outside of OVITO, or you can pick a different file as data source of the :ref:`data pipeline <usage.modification_pipeline>`.
Switching the data source file can be useful if you have set up a data analysis pipeline and now would like to apply it to a different simulation dataset.

.. _usage.import.command_line:

Command line
------------

When launching OVITO from a terminal, you can directly specify a file to load. This works for local and remote files::

  ovito /path/filename
  ovito sftp://hostname/path/filename
  ovito https://www.website.org/path/filename

You can import several files at once by specifying multiple filenames on the command line. 
If they all have the same file format, they will be concatenated into a sequence forming an animatable trajectory.
If they have different formats, OVITO will detect whether they represent a pair of topology/trajectory files (see next section).
If not, they will be inserted as several independent objects into the scene. 

.. _usage.import.sequence:

Simulation trajectories
-----------------------

OVITO can load trajectories consisting of a series of simulation snapshots.
Various scenarios are supported by the software:

A series of snapshot files:
  By default, whenever you import a new simulation file, OVITO tries to detect if the file is part of a numbered sequence of files
  with similar names in the same directory. To this end, the last number (if any) in the filename you've picked is replaced with the wildcard
  character ``*`` to generate a search pattern, which will subsequently be used to look in the directory for more files belonging to the sequence.
  For instance, if you imported a file named :file:`anim1c_5000.dump`, OVITO will generate the search pattern
  :file:`anim1c_*.dump` to find more snapshots (e.g. :file:`anim1c_0.dump`, :file:`anim1c_1000.dump`, :file:`anim1c_2000.dump`, etc). It is possible to
  manually override the generated file pattern in the input field highlighted in the screenshot or to turn off the 
  automatic search by deactivating the :guilabel:`auto-generate` option.

One file containing all trajectory frames:
  OVITO automatically detects whether the imported file contains more than one simulation frame and loads all of them as an animation sequence.
  For some file types, e.g. XYZ and LAMMPS dump, this is indicated by the :guilabel:`Contains multiple timesteps`
  checkbox highlighted in the screenshot. Note that OVITO typically keeps only the data of a single frame in memory at a time.
  Subsequent frames are loaded into memory only when needed, for example if you play back the animation or move the time slider.

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

  The topology file will be loaded first (e.g. a LAMMPS *data* file) and a :ref:`Load trajectory <particles.modifiers.load_trajectory>` modifier 
  will be inserted into the data pipeline to load the time-dependent atomic positions
  from the trajectory file (e.g. a LAMMPS *dump* file). This modifier merges both pieces of information -the static topology and the dynamic trajectory data- into a single animated dataset.

OVITO will display a timeline and a time slider at the bottom of main window if a simulation sequence with more than one frame
was loaded. Learn more about OVITO's animation functions in :ref:`this section <usage.animation>` of the manual.

.. _usage.import.remote:

Remote data access
==================

OVITO comes with built-in SSH and HTTP(S) clients for accessing files on remote machines. This feature can save you from having to transfer
files stored in remote locations, for example on HPC clusters, to your local desktop computer first.
To open a file located on a remote host, select :menuselection:`File --> Load Remote File` from the menu.

The current version of OVITO does not provide a way to browse directories on remote machines. You have to directly specify
the full path to the remote file as an URL of the form::

  sftp://user@hostname/path/filename

In this URL, replace :command:`user` with the SSH login name for your remote machine,
:command:`hostname` with the host name of the remote machine,
and :command:`/path/filename` with the full path to the simulation file that should be loaded.
Similarly, you can let OVITO retrieve a data file from a web server by specifying an URL of the form::

  https://www.website.org/path/filename

When connecting to the remote machine, OVITO will ask for the login password or the passphrase for the private key to be used for authentication.
Once established, the SSH connection is kept alive until the program session ends. OVITO creates a temporary copy of the remote file on the local computer before
loading the data into memory to speed up subsequent accesses to all simulation frames. The local data copies are cached until you close OVITO or
until you hit the :guilabel:`Reload` button in the :ref:`External File <scene_objects.file_source>` panel.

.. note::

  If it exists, OVITO will parse the :file:`~/.ssh/config` `configuration file <https://www.ssh.com/ssh/config>`_ in your home directory to 
  configure the SSH connection.  

  When running OVITO from the terminal, you can set the environment variable ``OVITO_SSH_LOG=1`` to activate log output
  for the built-in SSH client and diagnose possible connection problems.

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


.. _usage.animation:
.. _file_formats.input:
.. _scene_objects.file_source:
.. _clone_pipeline:
