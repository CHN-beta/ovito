.. _installation:

============
Installation
============

Binary program packages of OVITO for Linux, Windows, and macOS can be downloaded from `www.ovito.org <https://www.ovito.org/>`_.

.. _installation.requirements:

System Requirements
===================

OVITO requires a 64-bit operating system and runs on processors with x86-64 architecture.
The graphical user interface of OVITO requires 3D graphics hardware with support for the `OpenGL <https://en.wikipedia.org/wiki/OpenGL>`_ programming interface (OpenGL 2.1 or newer). 
In general it is recommended that you install the latest graphics driver provided by your hardware vendor before running OVITO as some older drivers may not fully support modern OpenGL specifications, which can lead to compatibility problems.

.. _installation.instructions:

Installation instructions
=========================

*Linux*:
    Extract the downloaded :file:`.tar.xz` archive file using the `tar <https://www.computerhope.com/unix/utar.htm>`_ command: :command:`tar xJfv ovito-3.6.0-x86_64.tar.xz`.
    This will create a new sub-directory containing the program files.
    Change into that directory and start OVITO by running the executable :command:`./bin/ovito`.

*Windows*:
    Run the installer program :file:`ovito-x.y.z-win64.exe` to install OVITO in a directory of your choice.
    Note that Windows might ask whether you really want to launch the installer, because it was downloaded from the web and it is not digitally signed.

*macOS*:
    Double-click the downloaded :file:`.dmg` disk image file to open it, agree to the program license, and drag the :program:`Ovito` application bundle into your :file:`Applications` folder.
    Then start OVITO by double-clicking the application bundle.

.. _installation.remote:

Running on remote machines
==========================
    
Note that the OVITO desktop application cannot be run through an SSH connection using X11 forwarding mode, because the software requires direct 
access to the graphics hardware (OpenGL direct rendering mode). If you simply run :command:`ovito` in an SSH terminal, you will likely get failure messages 
during program startup or just a black application window. 
  
It is possible to run OVITO on a remote machine through an SSH connection using a VirtualGL + VNC setup.
For further information, please see the `www.virtualgl.org <https://www.virtualgl.org/>`_ website.
In this mode, OVITO will make use of the graphics hardware of the remote machine, which must be set up to allow running
applications in a desktop environment. Please contact your local computing center staff to find out whether 
this kind of remote visualization mode is supported by the HPC cluster(s) you work on. 

Python module installation
==========================

The **OVITO Pro** program packages ship with an :ref:`integrated Python interpreter <ovitos_interpreter>` (:command:`ovitos`) that gets installed alongside with the desktop application,
allowing you to execute Python scripts written for OVITO. 
Optionally, you can install the ``ovito`` Python module into existing Python interpreters on your system  (e.g. :program:`Anaconda` or the standard :program:`CPython` interpreter) in case you would like to make use of 
OVITO's functionality in script-based workflows. Please refer to :ref:`this section <use_ovito_with_system_interpreter>` for further setup instructions.

.. _installation.troubleshooting:

Troubleshooting
===============

If you run into any problems during the installation of OVITO, you can contact the developers through our `online support forum <https://www.ovito.org/forum/>`_. 
The OVITO team will be happy to help you. The most commonly encountered installation issues are described here: 

Linux
-----

.. error::

  Starting the desktop application :command:`ovito` or the script interpreter :command:`ovitos` may fail with the following error::

    ./ovito: error while loading shared libraries: libQt5DBus.so.5: 
             cannot open shared object file: No such file or directory

  This error is typically caused by broken symbolic links inside the :file:`lib/ovito/` sub-directory after 
  extracting the OVITO installation archive on a computer other than the target machine. 

.. admonition:: Solution
  
  Reinstall OVITO by extracting the installation archive on the target machine. 
  Do *not* transfer the program directory tree between different computers after it has been extracted.

.. error::

  You may see the the following error when running :command:`ovito` on a Linux machine::

    qt.qpa.plugin: Could not load the Qt platform plugin "xcb" in "" even though it was found.
    This application failed to start because no Qt platform plugin could be initialized. 
    Reinstalling the application may fix this problem.
    Available platform plugins are: minimal, offscreen, vnc, xcb.

  In this case OVITO cannot find the required :file:`libxcb-*.so` set of system libraries, which might not be 
  preinstalled on fresh Linux systems. 

.. admonition:: Solution

  Install the required libraries using the system's package manager:

  .. code-block:: shell

    # On Ubuntu/Debian systems:
    sudo apt-get install libxcb1 libx11-xcb1 libxcb-glx0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 \
                         libxcb-randr0 libxcb-render-util0 libxcb-render0 libxcb-shape0 libxcb-shm0 \
                         libxcb-sync1 libxcb-xfixes0 libxcb-xinerama0 libxcb-xinput0 libxcb-xkb1
                   
    # On CentOS/RHEL systems:
    sudo yum install libxcb xcb-util-image xcb-util-keysyms xcb-util-renderutil xcb-util-wm

  Debian users should also pay attention to `this thread in the OVITO support forum <https://www.ovito.org/forum/topic/installation-problem/#postid-2272>`__.

.. error::

  OVITO depends on the OpenSSL libraries (version 1.1.*). If they are not present on your system, starting :command:`ovito` will typically fail with the error::

    error while loading shared libraries: libssl.so.1.1: cannot open shared object file: No such file or directory

.. admonition:: Solution

  Please install the OpenSSL 1.1.x libraries using the package manager of your Linux distribution. OVITO depends on the 
  presence of the shared libraries :file:`libssl.so.1.1` and :file:`libcrypto.so.1.1` in your system directory. On CentOS 7, for example, 
  you should install the package `openssl11-libs <https://pkgs.org/search/?q=openssl11-libs>`__.