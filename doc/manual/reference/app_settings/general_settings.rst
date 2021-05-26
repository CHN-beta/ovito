.. _application_settings.general:

General settings
================

.. image:: /images/app_settings/general_settings.*
  :width: 45%
  :align: right

On this tab of the :ref:`application settings dialog <application_settings>` you can adjust
global options affecting the user interface and the graphics system of the program. 

User interface
""""""""""""""

Load file: Use alternative file selection dialog
  Changes the type of file selector shown when you import a simulation file into OVITO. 
  By the default, the operating system's file selection dialog is used. This option
  activates a built-in dialog instead, which may or may not provide certain advantages 
  over the standard file selector (depends on your needs).

Modifiers list: Sort by category
  If this option is set, the :ref:`list of available modifiers <particles.modifiers>` 
  is split into function groups. Otherwise, modifiers are shown in one alphabetically ordered list.

3D graphics
"""""""""""

Graphics hardware interface
  Selects the application programming interface used by OVITO for rendering the contents of the interactive 
  viewports. Currently, OVITO supports the `OpenGL <https://www.opengl.org/>`__ and the `Vulkan <https://www.vulkan.org/>`__ interfaces. The OpenGL interface is more mature
  and should work well on most systems. Vulkan is the more modern programming interface, and some graphics drivers
  may still exhibit compatibility problems. Please inform the OVITO developers about any such problems.
  
  The Vulkan interface provides the advantage of allowing you to directly select the graphics
  device to be used by OVITO in case the system contains several GPUs and/or integrated graphics processors. When using the OpenGL interface,
  you have to make this selection on the `operating system or graphics driver level <https://answers.microsoft.com/en-us/windows/forum/windows_10-hardware/select-gpu-to-use-by-specific-applications/eb671f52-5c24-428d-a7a0-02a36e91ee2f>`__.

  .. note::

    The Vulkan renderer option is *not* available on the macOS platform or in OVITO for Anaconda builds yet. 

  Select :menuselection:`System Information` from the :menuselection:`Help` menu of OVITO to access further information 
  about the graphics hardware available in your system. Submit this information to the OVITO developers when
  reporting graphics compatibility problems.

Program updates
"""""""""""""""

Auto-refresh news page from web server
  If enabled, the program checks for software updates upon each program start by contacting
  the OVITO server `ovito.org`. The availability of a new update is displayed in the 
  command panel of the program.

Send unique installation ID to web server
  If this option is enabled, OVITO transmits an installation ID to the OVITO server as part of the update check to help the software vendor collect anonymous 
  usage statistics. The installation ID is an anonymous, random number generated once during local installation 
  of the program and contains no personal information or user data. Furthermore, the software vendor OVITO GmbH does not collect or store your 
  IP address or other information beyond the anonymous installation ID that could identify you.
  Turning this option off will not affect the operation of OVITO.