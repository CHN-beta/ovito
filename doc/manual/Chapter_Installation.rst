.. highlight:: console

============
Installation
============

Binary program packages of OVITO for Linux, Windows, and macOS can be downloaded from `www.ovito.org <https://www.ovito.org/>`_.

.. _installation.requirements:

Requirements
============

OVITO requires a 64-bit operating system and runs on processors with x86-64 architecture.
The graphical user interface of OVITO requires 3D graphics hardware with support for the `OpenGL <https://en.wikipedia.org/wiki/OpenGL>`_ programming interface (OpenGL 2.1 or newer). 
In general it is recommended that you install the latest graphics driver provided by your hardware vendor before running OVITO as some older drivers may not fully support modern OpenGL specifications, which can lead to compatibility problems.


Installation steps
===================

*Linux*:
    Extract the downloaded :file:`.tar.xz` archive file using the `tar <https://www.computerhope.com/unix/utar.htm>`_ command, e.g.: :command:`tar xJfv ovito-3.0.0-x86_64.tar.xz`.
    This will create a new subdirectory containing the program files.
    Change into that directory and start OVITO by running the executable :command:`./bin/ovito`.

*Windows*:
    Run the installer program :file:`ovito-x.y.z-win64.exe` to install OVITO in a directory of your choice.
    Note that Windows might ask whether you really want to launch the installer, because it was downloaded from the web and it is not digitally signed.

*macOS*:
    Double-click the downloaded :file:`.dmg` disk image file to open it, agree to the program license, and drag the :program:`Ovito` application bundle into your :file:`Applications` folder.
    Then start OVITO by double-clicking the application bundle.

Running OVITO on a remote machine
===================================
    
Note that the OVITO desktop application cannot be run through an SSH connection using X11 forwarding mode, because it requires direct 
access to the graphics hardware (so-called OpenGL direct rendering mode). 
Thus, if you try to run :command:`ovito` in an SSH terminal, you will likely get failure messages during program startup 
or just a black application window. 
  
It is possible, however, to run OVITO through an SSH connection using a VirtualGL + VNC setup.
For further information, please see the `www.virtualgl.org <https://www.virtualgl.org/>`_ website.
In this mode, OVITO will make use of the graphics hardware of the remote machine, which must be set up to allow running
applications in a desktop environment. Please contact your local computing center staff to find out whether 
this kind of remote visualization mode is supported by the HPC cluster you work on. 

Installing the Python module
============================

The **OVITO Pro** program packages ship with an integrated Python interpreter (":command:`ovitos`") that gets installed alongside with the graphical desktop application,
allowing you to execute Python scripts written for OVITO. 
Optionally, it is possible to install OVITO as a Python module into an existing Python interpreter on your system in case you would like to make use of 
OVITO's functionality in scripts written for other interpreters (e.g. the system Python interpreter or **Anaconda**).
Please refer to the `scripting manual <href="introduction/running" anchor="installing-the-ovito-module-in-your-python-interpreter" no-pro-tag="1">`_ for more 
information on the setup procedure.
    
Troubleshooting
================

If you run into any problems during the installation of OVITO, you can contact us 
via the `online support forum <https://www.ovito.org/forum/>`_. The OVITO team will be happy to provide help.

Linux
-----------------

Starting :command:`ovito` or the script interpreter :command:`ovitos` may fail with the following error::

  ./ovito: error while loading shared libraries: libQt5DBus.so.5: 
           cannot open shared object file: No such file or directory

This error is typically caused by broken symbolic links inside the :file:`lib/ovito/` subdirectory after 
extracting the OVITO installation archive on a computer other than the target machine. 
**Solution:** Reinstall OVITO by extracting the installation archive on the target machine. 
Do *not* transfer the program directory tree between different computers after it has been extracted.

Furthermore, you may see the the following error when running :command:`ovito` on a Linux machine::

  qt.qpa.plugin: Could not load the Qt platform plugin "xcb" in "" even though it was found.
  This application failed to start because no Qt platform plugin could be initialized. 
  Reinstalling the application may fix this problem.
  Available platform plugins are: minimal, offscreen, vnc, xcb.

In this case OVITO cannot find the required :file:`libxcb-*.so` set of system libraries, which might not be 
preinstalled on fresh Linux systems. **Solution:** Install the required libraries using the system's package manager::

  # On Ubuntu/Debian systems:
  sudo apt-get install libxcb1 libx11-xcb1 libxcb-glx0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 \
                       libxcb-randr0 libxcb-render-util0 libxcb-render0 libxcb-shape0 libxcb-shm0 \
                       libxcb-sync1 libxcb-xfixes0 libxcb-xinerama0 libxcb-xinput0 libxcb-xkb1
                   
  # On CentOS/RHEL systems:
  sudo yum install libxcb xcb-util-image xcb-util-keysyms xcb-util-renderutil xcb-util-wm


Building OVITO from source
===========================

**OVITO Basic** is available under an Open Source license and you can download the source code to build the program yourself, for example to develop new plugin functions for OVITO. 
See the `developers section <development>`_ for further instructions.
