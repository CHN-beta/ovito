.. _appendix.license.pyside6.instructions:

Build instructions for PySide6
------------------------------

The OVITO package includes a distribution of the PySide6 module and Shiboken6 module licensed under the GNU Lesser General Public License (LGPLv3).
In accordance with the requirements of this license, this page provides instructions on how to obtain or rebuild compatible versions of these binary modules from source.

Windows
"""""""

OVITO Pro for Windows ships with a copy of the official PySide6-Essentials module (version 6.3.1) from 
the `PyPI repository <https://pypi.org/project/PySide6/>`__.

Linux
"""""

OVITO Pro for Linux ships with a copy of the PySide6 module that has been built from the original sources provided by
the Qt Company, following the standard procedure described `here <https://doc.qt.io/qtforpython/gettingstarted-linux.html>`__.
PySide6 v6.3 has been compiled against Qt 6.3.1 (see :ref:`here <appendix.license.qt6.instructions>`) and a custom build of the `CPython <https://www.python.org>`__ 3.10 interpreter::

  # Build platform: CentOS 6.10
  # Compiler: g++ 9.1 (CentOS devtoolset-9)
  git clone --recursive https://code.qt.io/pyside/pyside-setup
  cd pyside-setup
  git checkout 6.3.1
  python3 setup.py install \
    --qmake=$HOME/progs/qt6/bin/qmake \
    --ignore-git \
    --no-examples \
    --module-subset=Core,Gui,Widgets,Xml,Network,Svg \
    --skip-docs \
    --no-qt-tools

macOS
"""""

OVITO Pro for macOS ships with a copy of the PySide6 module that has been built from the original sources provided by
the Qt Company, following the standard procedure described `here <https://doc.qt.io/qtforpython/gettingstarted-macOS.html>`__.
PySide6 v6.3 has been compiled against Qt 6.3.1 (macOS) and a standard installation of the `CPython <https://www.python.org>`__ 3.10 interpreter for macOS (universal binary)::

  git clone --recursive https://code.qt.io/pyside/pyside-setup
  cd pyside-setup
  git checkout 6.3.1

  sudo CLANG_INSTALL_DIR=$HOME/progs/libclang SETUPTOOLS_USE_DISTUTILS=stdlib \
    python3.10 setup.py install \
    --qmake=`echo $HOME/Qt/6.*.*/macos/bin/qmake` \
    --ignore-git \
    --module-subset=Core,Gui,Widgets,Xml,Network,Svg,OpenGL,OpenGLWidgets \
    --skip-docs

  cd /Library/Frameworks/Python.framework/Versions/3.10/lib/python3.10/site-packages/PySide6/
  sudo rm -r Assistant.app Designer.app Linguist.app
  sudo rm lupdate lrelease 
  sudo rm -r examples
  sudo rm -r Qt
