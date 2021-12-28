.. _appendix.license.pyside2.instructions:

Build instructions for PySide2
------------------------------

The OVITO package includes a distribution of the PySide2 module and Shiboken2 module licensed under the GNU Lesser General Public License (LGPLv3).
In accordance with the requirements of this license, this page provides instructions on how to obtain or rebuild compatible versions of these binary modules from source.

Windows
"""""""

OVITO Pro for Windows ships with a copy of the official PySide2 module (version 5.15.0) from 
the `PyPI repository <https://pypi.org/project/PySide2/>`__.

Linux
"""""

OVITO Pro for Linux ships with a copy of the PySide2 module that has been built from the original sources provided by
the Qt Company, following the standard procedure described `here <https://wiki.qt.io/Qt_for_Python_GettingStarted/X11>`__.
PySide2 v5.15 has been compiled against Qt 5.15.2 (see :ref:`here <appendix.license.qt5.instructions>`) and a build of the standard `CPython <https://www.python.org>`__ 3.9 interpreter::

  # Build platform: CentOS 6.9
  # Compiler: g++ 7.1 (CentOS devtoolset-7)
  git clone --recursive https://code.qt.io/pyside/pyside-setup
  cd pyside-setup
  git checkout 5.15
  python3 setup.py install \
    --qmake=$HOME/progs/qt5/bin/qmake \
    --ignore-git \
    --no-examples \
    --module-subset=Core,Gui,Widgets,Xml,Network,Svg \
    --skip-docs

macOS
"""""

OVITO Pro for macOS ships with a copy of the PySide2 module that has been built from the original sources provided by
the Qt Company, following the standard procedure described `here <https://wiki.qt.io/Qt_for_Python_GettingStarted/MacOS>`__.
PySide2 v5.15 has been compiled against Qt 5.15.2 (macOS) and a standard installation of the `CPython <https://www.python.org>`__ 3.8 interpreter for macOS (64-bit)::

  git clone --recursive https://code.qt.io/pyside/pyside-setup
  cd pyside-setup
  git checkout 5.15
  sudo CLANG_INSTALL_DIR=$HOME/progs/libclang python3.8 setup.py install \
    --qmake=`echo $HOME/Qt/5.*.*/clang_64/bin/qmake` \
    --ignore-git \
    --module-subset=Core,Gui,Widgets,Xml,Network,Svg \
    --skip-docs
