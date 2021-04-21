.. _particles.modifiers.python_script:

Python script |ovito-pro|
-------------------------

.. image:: /images/modifiers/python_script_panel.png
  :width: 30%
  :align: right

This modifier type lets you write your own user-defined modifier function in the Python language. Your Python function runs within the graphical user interface 
just like a normal OVITO modifier and can manipulate, analyze or amend the simulation data in exactly the way you need it.
Such user-defined modifier functions are useful whenever the standard tool set of :ref:`built-in modifiers <particles.modifiers>` of OVITO is insufficient 
to solve your specific problem.

What is a Python script modifier?
"""""""""""""""""""""""""""""""""

.. highlight:: python

A script modifier consists of a single Python function definition, which you enter into the integrated code editor window 
of OVITO. For example::

  def modify(frame, data):
      print("Number of particles:", data.particles.count)
      data.attributes['Time'] = 0.002 * frame

This function prints the current number of particles to the log window and generates a new :ref:`global attribute <usage.global_attributes>` for the simulation time based on the current
animation frame number. OVITO takes care of executing this Python function at the right time(s), e.g.
whenever you move the animation time slider and the results of the :ref:`data pipeline <usage.modification_pipeline>` need to be recomputed.
Note that you only define a Python function, not an entire Python program. OVITO is responsible for calling your user-defined function whenever it needs to.

The function receives the current dataset, produced by the upstream data pipeline, as an input argument, and you can implement your own data processing algorithms 
within the Python function, operating on the provided input data. Use OVITO's comprehensive :ref:`Python API <scripting_manual>`
to access different aspects of the datasets or dynamically generate new data objects, which will then be rendered in OVITO's interactive viewports. 

Usage
"""""

.. image:: /images/modifiers/python_script_code_editor.png
  :width: 50%
  :align: right

After inserting a new Python script modifier into the pipeline you can open the integrated code editor using the
:guilabel:`Edit script` button. The editor window lets you change the source code for the user-defined modifier function,
which initially consists of a few example statements. You should replace these statements with your own code, performing the 
specific computations or actions that are needed to solve your problem. 
The :ref:`scripting manual <writing_custom_modifiers>` gives further instructions on how to write the user-defined modifier function. 
Note that the Python function must be named ``modify()``,  
however, you are free to define additional functions within the same source file and call them as sub-routines from the main modifier function.

Once you are done writing the user-defined modifier function, press the *Commit and run script* button (the "play" icon)
in the code editor's toolbar. This will compile the code by executing any top-level statements 
including the function definition and then request an update of the data pipeline. As part of this pipeline update 
your ``modify()`` function will get invoked by the system.

Note that the pipeline system may run your ``modify()`` function repeatedly, 
for example when stepping through a simulation trajectory. That means you should always write your ``modify()`` function
in such a way that it doesn't have any side effects on the global state of OVITO. The function should be "pure" and stateless in the sense 
that it only operates on the current dataset received from the system as a function parameter.
The pipeline is free to decide whether it caches the results of your modifier function, and it may call your function 
repeatedly if necessary. Thus, the function must be designed such that it can process 
each trajectory frame in isolation. 

Making the modifier function permanently available in OVITO
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

First, you should give your modifier a meaningful name, making it easier for you to identify the modifier 
in the data pipeline. The :ref:`pipeline editor <usage.modification_pipeline.pipeline_listbox>` of
OVITO lets you to change the title of the modifier from the default "Python script" to a more descriptive label 
that better summarizes the specific purpose of your Python function. In order to rename a modifier in the pipeline editor, make sure 
it is selected and then click the pipeline item a second time to edit its name. 

Next, you have two alternative ways of making your Python modifier permanently available in future program
sessions. One is to save the current modifier, including the source code of the
``modify()`` function, as a :ref:`modifier template <modifier_templates>`. The modifier template will appear as a new
entry in the list of available modifiers, allowing you to easily access the user-defined modifier in the future and insert it into a new data 
pipeline.

The second option is to save the source code as a :file:`.py` file in 
one of the following directories on your computer. OVITO Pro will automatically scan these directories and 
display all :file:`.py` scripts in the available modifiers list, from where you can inserted them 
into a data pipeline with a single click.

  * All platforms: :file:`<HOME>/.config/Ovito/scripts/modifiers/*.py`
  * Linux: :file:`<INSTALLDIR>/share/ovito/scripts/modifiers/*.py`
  * Windows: :file:`<INSTALLDIR>/scripts/modifiers/*.py`
  * macOS: :file:`<INSTALLDIR>/Ovito.app/Contents/Resources/scripts/modifiers/*.py`
  * Anaconda: :file:`<INSTALLDIR>/share/ovito/scripts/modifiers/*.py`

Here, :file:`<HOME>` and :file:`<INSTALLDIR>` refer to your home directory and the installation location of OVITO Pro on your computer, 
respectively. The latter location already contains a bunch of predefined Python modifier files shipping with the program.

Examples
""""""""

The scripting manual contains several :ref:`code examples <modifier_script_examples>`  
demonstrating how to write a ``modify()`` function:

  * :ref:`example_msd_calculation`
  * :ref:`example_order_parameter_calculation`
  * :ref:`example_visualize_local_lattice_orientation`
  * :ref:`example_select_overlapping_particles`

.. seealso::

  :py:class:`ovito.modifiers.PythonScriptModifier` (Python API)
