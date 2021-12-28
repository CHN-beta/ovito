.. _appendix.license.pyside6.instructions:

Build instructions for PySide6
------------------------------

The OVITO package includes a distribution of the PySide6 module and Shiboken6 module licensed under the GNU Lesser General Public License (LGPLv3).
In accordance with the requirements of this license, this page provides instructions on how to obtain or rebuild compatible versions of these binary modules from source.

Windows
"""""""

OVITO Pro for Windows ships with a copy of the official PySide6 module (version 6.2.2) from 
the `PyPI repository <https://pypi.org/project/PySide6/>`__.

Linux
"""""

OVITO Pro for Linux ships with a copy of the PySide6 module that has been built from the original sources provided by
the Qt Company, following the standard procedure described `here <https://doc.qt.io/qtforpython/gettingstarted-linux.html>`__.
PySide6 v6.2 has been compiled against Qt 6.2.2 (see :ref:`here <appendix.license.qt.instructions>`) and a build of the standard `CPython <https://www.python.org>`__ 3.9 interpreter::

  # Build platform: CentOS 6.10
  # Compiler: g++ 9.1 (CentOS devtoolset-9)
  git clone --recursive https://code.qt.io/pyside/pyside-setup
  cd pyside-setup
  git checkout 6.2
  python3 setup.py install \
    --qmake=$HOME/progs/qt6/bin/qmake \
    --ignore-git \
    --no-examples \
    --module-subset=Core,Gui,Widgets,Xml,Network,Svg \
    --skip-docs

macOS
"""""

OVITO Pro for macOS ships with a copy of the PySide6 module that has been built from the original sources provided by
the Qt Company, following the standard procedure described `here <https://doc.qt.io/qtforpython/gettingstarted-macOS.html>`__.
PySide6 v6.2 has been compiled against Qt 6.2.2 (macOS) and a standard installation of the `CPython <https://www.python.org>`__ 3.9 interpreter for macOS (universal binary)::

  git clone --recursive https://code.qt.io/pyside/pyside-setup
  cd pyside-setup
  git checkout 6.2

  sudo CLANG_INSTALL_DIR=$HOME/progs/libclang SETUPTOOLS_USE_DISTUTILS=stdlib \
    python3.9 setup.py install \
    --qmake=`echo $HOME/Qt/6.*.*/macos/bin/qmake` \
    --ignore-git \
    --module-subset=Core,Gui,Widgets,Xml,Network,Svg \
    --skip-docs

  cd /Library/Frameworks/Python.framework/Versions/3.9/lib/python3.9/site-packages/PySide6/
  sudo rm -r Assistant.app Designer.app Linguist.app
  sudo rm -r examples Qt
  sudo rm lupdate lrelease 
