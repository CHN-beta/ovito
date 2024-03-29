.. _appendix.license.qt5.instructions:

Build instructions for Qt5
--------------------------

OVITO Basic and OVITO Pro program package include a binary version of the Qt libraries licensed under the GNU Lesser General Public License (LGPLv3).
In accordance with the requirements of this license, this section gives instructions on how to obtain or rebuild compatible versions of these binaries from source.

Windows
"""""""

OVITO for Windows is built against the original, unmodified binary DLLs of Qt 5.15.2 (MSVC 2019 64-bit) distributed by the Qt Company.

Linux
"""""

OVITO for Linux includes Qt libraries that have been built from the unmodified sources of Qt 5.15.2 distributed by the Qt Company.
The following commands have been used to generate them::

  # Build platform: CentOS 6.9
  # Compiler: g++ 7.1 (CentOS devtoolset-7)
  ./configure \
      -opensource \
      -confirm-license \
      -shared \
      -nomake examples \
      -qt-libpng \
      -qt-libjpeg \
      -qt-pcre \
      -xcb \
      -bundled-xcb-xinput \
      -xkbcommon \
      -no-cups \
      -pch \
      -no-eglfs \
      -no-linuxfb \
      -feature-vulkan \
      -I $VULKAN_SDK/include -L $VULKAN_SDK/lib \
      -no-feature-qdoc \
      -skip qtactiveqt \
      -skip qtconnectivity \
      -skip qt3d \
      -skip qtcanvas3d \
      -skip qtdatavis3d \
      -skip qtcharts \
      -skip qtlocation \
      -skip qtsensors \
      -skip qtdeclarative \
      -skip qtdoc \
      -skip qtgraphicaleffects \
      -skip qtmultimedia \
      -skip qtquickcontrols \
      -skip qtquickcontrols2 \
      -skip qtpurchasing \
      -skip qtremoteobjects \
      -skip qtsensors \
      -skip qtserialport \
      -skip qttranslations \
      -skip qtwebchannel \
      -skip qtgamepad \
      -skip qtscript \
      -skip qtserialbus \
      -skip qtvirtualkeyboard \
      -skip qtwayland \
      -skip qtwebengine \
      -skip qtwebsockets \
      -skip qtwebview \
      -skip qtwebglplugin \
      -skip qtxmlpatterns \
      -no-use-gold-linker \
      -prefix $HOME/progs/qt5
  make
  make install

macOS
"""""

OVITO for macOS ships with a standard installation of the Qt 5.15.2 framework distributed by the Qt Company.
