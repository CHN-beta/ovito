.. _application_settings:

Application settings
====================

.. image:: /images/app_settings/general_settings.*
  :width: 38%
  :align: right

To open the application settings dialog, select :guilabel:`Edit → Application settings` from the main menu on Windows/Linux.
On macOS, select :guilabel:`Ovito → Preferences` instead.
The dialog consists of several tabs:

.. toctree::
  :maxdepth: 1

  general_settings
  viewport_settings
  modifier_templates
  particle_settings

Where OVITO stores its settings
"""""""""""""""""""""""""""""""

The program stores all user settings in a platform-dependent location to preserve them across sessions. On Windows, the information
is saved in the user section of the system registry. On Linux, it is stored in an INI configuration file in the user's
home directory, and on macOS, it is stored in a property list file. The precise storage
location on your local computer is displayed by the application settings dialog at the bottom.
You can copy the configuration data across computers to transfer your personal settings.

To reset OVITO to its factory default settings, simply delete the configuration file on Linux/macOS or the registry branch
on Windows.
