.. _viewport_layers.text_label:

Text label layer
----------------

.. image:: /images/viewport_layers/text_label_overlay_panel.*
  :width: 35%
  :align: right

Add this :ref:`viewport layer <viewport_layers>` to a viewport to display text or other information
on top of the three-dimensional scene. The label's text may contain placeholders, which get replaced with the current values
of :ref:`global attributes <usage.global_attributes>` dynamically computed in OVITO's data pipeline.
This makes it possible to include dynamic data such as the current simulation time, the number of atoms,
or numeric results in rendered images or movies.

HTML text formatting
""""""""""""""""""""

You can include HTML markup elements and Cascading Style Sheet (CSS) attributes to format parts of the text.
OVITO supports a subset of the HTML standard, which is `documented here <https://doc.qt.io/qt-6/richtext-html-subset.html>`__.
The following table gives a few examples:

.. |br| raw:: html

   <br/>

.. |underlined| raw:: html

   <span style="text-decoration: underline">underlined</span>

.. list-table::
  :widths: 50 50
  :header-rows: 1

  * - Input markup text
    - Formatted output
  * - nm<sup>3</sup>
    - nm\ :sup:`3`  
  * - e<sub>z</sub>
    - e\ :sub:`z`  
  * - First line<br>Second line
    - First line |br| Second line
  * - Some <i>italic</i> text
    - Some *italic* text
  * - Some <span style="text-decoration: underline">\ underlined</span> text
    - Some |underlined| text

Attribute values
""""""""""""""""

The text label gets a list of :ref:`global attributes <usage.global_attributes>` from the 
selected data pipeline. Attributes are named variables containing numeric or other values 
dynamically computed by the pipeline. You can incorporate attribute values in the label text 
by inserting placeholders of the form `[attribute_name]` into the text field.

When a placeholder in the text references an attribute with a numeric value, the value 
gets formatted according to a format specification string. You have the choice between decimal notation (``%f``), exponential notation (``%e``)
and an automatic mode (``%g``). Furthermore, the format string allows you to control the output precision, i.e. the number of decimal places that
appear after the decimal point. Use ``%.2f``, for example, to always show two digits after the decimal point. 
The format string must follow the standard convention of the `printf() <https://en.cppreference.com/w/cpp/io/c/fprintf>`__ C function.

Defining new attributes
"""""""""""""""""""""""

Attributes computed by the data pipeline may not always have the desired 
format, units, or normalization for using them in a text label directly. For instance, 
the ``CommonNeighborAnalysis.counts.BCC`` attribute calculated by the
:ref:`Common neighbor analysis <particles.modifiers.common_neighbor_analysis>` modifier
counts the total number of bcc atoms in the system. But what if you rather would 
like to print the *fraction* of bcc atoms, not the absolute count?
In such situations some kind of conversion and/or transformation of the attribute's value is required,
and you will have to define a new attribute that derives its value from the original attribute. 

.. highlight:: python

This can be accomplished by inserting a :ref:`Python script <particles.modifiers.python_script>` modifier 
into the data pipeline. This modifier executes a simple, user-defined Python function that computes the value of our 
new attribute on the basis of the existing attributes(s)::

  def modify(frame, data):
      bcc_count = data.attributes['CommonNeighborAnalysis.counts.BCC']
      data.attributes['bcc_fraction'] = bcc_count / data.particles.count

In this example we access the existing attribute ``CommonNeighborAnalysis.counts.BCC`` (the atom count) and 
convert it into a fraction by dividing by the total number of atoms. The new value is output
as a new attribute named ``bcc_fraction``, making it available for inclusion in the text label layer.

.. seealso::
  
  :py:class:`ovito.vis.TextLabelOverlay` (Python API)


