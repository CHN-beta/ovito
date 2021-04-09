.. _viewport_layers.text_label:

Text label viewport layer
-------------------------

.. image:: /images/viewport_layers/text_label_overlay_panel.*
  :width: 35%
  :align: right

This :ref:`viewport layer <viewport_layers>` renders some text 
into the output image. The text string may contain placeholders that get replaced with the current values
of :ref:`global attributes <usage.global_attributes>` dynamically computed in OVITO's data pipeline.
This makes it possible to include dynamic information such as the current simulation time, the number of atoms,
or numeric computation results in rendered images or movies.

When a placeholder in the text string is replaced with a floating-point value, that value gets formatted using the 
format specification string. You have the choice between decimal notation (``%f``), exponential notation (``%e``)
and an automatic mode (``%g``). Furthermore, the format string allows you to control the output precision, i.e. the number of digits that
appear after the decimal point. Use ``%.2f``, for example, to always show two digits after the decimal point. 
The format string must follow the standard convention of the `printf() <https://en.cppreference.com/w/cpp/io/c/fprintf>`__ C function.

The :guilabel:`Variables` panel displays the list of available global attributes computed in the current pipeline, 
which may be included in the text as placeholders, and which will get replaced with their dynamically computes values when 
rendering an animation.

Defining derived attributes
"""""""""""""""""""""""""""

Sometimes a global attribute computed by the data pipeline does not yet have the right 
format, unit or normalization that is required for the text label display. For instance, 
the ``CommonNeighborAnalysis.counts.BCC`` attribute computed by the
:ref:`Common neighbor analysis <particles.modifiers.common_neighbor_analysis>` modifier
reports the number of BCC atoms in the system. But what if you would like to print the *fraction*
of BCC atoms instead of the absolute count?

.. highlight:: python

In order to divide the absolute atom count by the total number of atoms in the system we need to define a new derived attribute.
This can be accomplished by inserting a :ref:`Python script <particles.modifiers.python_script>` modifier 
into the data pipeline. This modifier executes a simple Python function computing the value of our new attribute
on the basis of the existing attributes(s)::

  def modify(frame, data):
      bcc_count = data.attributes['CommonNeighborAnalysis.counts.BCC']
      data.attributes['bcc_fraction'] = bcc_count / data.particles.count

The new attribute ``bcc_fraction`` may now be included as a placeholder in the text label string to render it into the output image.

.. seealso::
  
  :py:class:`~ovito.vis.TextLabelOverlay` (Python API)



.. _usage.global_attributes:
.. _particles.modifiers.common_neighbor_analysis: