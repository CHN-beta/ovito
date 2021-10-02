.. _python_code_generation:

Python code generator |ovito-pro|
---------------------------------

.. image:: /images/scene_objects/code_generation.jpg
  :width: 60%
  :align: right

OVITO Pro includes a Python code generation function, which can turn any :ref:`data pipeline <usage.modification_pipeline>`
created with the graphical user interface into a corresponding sequence of Python script statements, 
which may subsequently be executed outside of OVITO to automate data post-processing or visualization 
workflows. While you interactively adjust the parameters of modifiers, the code generation function  
produces corresponding script statements for OVITO's Python programming interface.
The generated source code may be saved to disk, further customized if desired,
and then run using the :ref:`embedded Python script interpreter <ovitos_interpreter>` :command:`ovitos` or any regular Python interpreter after installing the 
``ovito`` Python module (see `downloads <https://www.ovito.org/python-downloads/>`__).

The Python code generator is invoked by selecting :menuselection:`File --> Generate Python script` from the
menu. The code generator window displays the dynamically generated 
source code, which gets updated in realtime while you make changes to the data pipeline or a modifier's settings.

You can activate the option :guilabel:`Include visualization code` to let OVITO additionally generate code statements that
set up the virtual camera, render settings, and the visual appearance of the dataset exactly as you prescribed it
in the graphical user interface. 
This option is useful if you are going to use the generated Python script for automating image and animation rendering tasks.

Supported features
""""""""""""""""""

The code generator can generate Python statements for the following aspects of the visualization scene:
  
  * Data file import (all arguments of the :py:func:`ovito.io.import_file` function)
  * Visual elements
  * Modifiers
  * Viewport layers
  * Viewport camera setup
  * Rendering engine configuration
  * Image/animation render settings

The following aspects are *not* covered by the code generator yet:

  * Direct modifications you make to imported data objects in the GUI, e.g. the colors and radii of particle types
  * Data export to an output file
  * Key-frame based parameter animations

Thus, you have may have to amend the generated script with corresponding code statements yourself to 
take care of these things. See the sections :ref:`file_output_overview` and :py:attr:`ParticleType.radius <ovito.data.ParticleType.radius>` in the OVITO Python API documentation on how to do that.
