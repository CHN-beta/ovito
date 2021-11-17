.. _usage.import.remote:

Remote file access
==================

OVITO comes with built-in SSH and HTTP clients for accessing files located on remote machines. This feature can save you from having to transfer
files stored in remote locations, for example on a HPC cluster, to your local desktop computer first.
To open a file stored on a remote host, select :menuselection:`File --> Load Remote File` from the menu.

The current version of OVITO does not provide a way to browse directories on remote machines. You have to directly specify
the full path to the remote file as an URL of the form::

  sftp://user@hostname/path/filename

Replace :command:`user` with your SSH login name for your remote machine, :command:`hostname` with the host name of the remote machine,
and :command:`/path/filename` with the full path to the simulation data file to load. Note that the use of the "~" shortcut to a user's home directory is *not* supported; you have to specify the absolute directory path.

Furthermore, you can let OVITO download data from a web server location by specifying an URL of the form::

  https://www.website.org/path/filename

When connecting to the remote machine via SSH, OVITO will ask for the login password or the passphrase for the private key to be used for authentication.
Once established, the SSH connection is kept alive until the program session ends. OVITO creates a temporary copy of the remote file on the local computer before
loading the data into memory to speed up subsequent accesses to all simulation frames. The local data copies are cached until you close OVITO or
until you hit the :guilabel:`Reload` button in the :ref:`External File <scene_objects.file_source>` panel.

.. note::

  If it exists, OVITO will parse the :file:`~/.ssh/config` `configuration file <https://www.ssh.com/ssh/config>`_ in your home directory to 
  configure the SSH connection.  

  When starting OVITO from a terminal window, you can set the environment variable ``OVITO_SSH_LOG=1`` to enable logging output
  for the built-in SSH client and diagnose possible connection problems.
