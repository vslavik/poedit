
# Poedit: cross-platform translation editor

[![Crowdin](https://badges.crowdin.net/poedit/localized.svg)](https://crowdin.com/project/poedit)

## About

This program is a simple translation editor for PO and XLIFF files. It also
serves as a GUI frontend to more GNU gettext utilities (win32 version
is part of the distribution) and catalogs editor/source code parser. It helps
with translating applications into another language. For details on principles
of the solution used, see GNU gettext documentation.


## Installation

Easily-installable prebuilt binaries for Windows and macOS are available from
https://poedit.net/download

Official binaries for Linux are available as a Snap at https://snapcraft.io/poedit.
Most Linux distributions also include native Poedit packages.


### Installing from sources

Requirements:

  * Boost
  * Unicode build of [wxWidgets](http://www.wxwidgets.org) library, version >= 3.2
  * ICU
  * LucenePlusPlus
  * If on Unix, GtkSpell for spell checking support

Optional dependencies:

  * cld2 (better language autodetection and non-English source languages)
  * C++REST SDK >= 2.5 (Crowdin integration)


### Unix

Do the usual thing:

    ./configure
    make
    make install

You must have the dependencies installed in a location where configure will find
them, e.g. by setting `CPPFLAGS` and `LDFLAGS` appropriately.

### macOS

You need a full git checkout to build on macOS; see below for details.

After checkout, use the `Poedit.xcworkspace` workspace and the latest version of
Xcode to build Poedit.

There are some additional dependencies on tools not included with macOS.
They can be installed with Homebrew and `macos/Brewfile`:

    brew bundle --file=macos/Brewfile


### Windows using Visual Studio

You need a full git checkout to build on Windows; see below for details. Note that the repository uses
symlinks and so you'll need to enabled [Developer Mode](https://msdn.microsoft.com/en-us/windows/uwp/get-started/enable-your-device-for-development) and configure git to allow symlinks:

    git config --global core.symlinks true

After checkout, use the `Poedit.sln` solution to build everything. To build the installer, open `win32/poedit.iss` in Inno Setup and compile the project.


 ### Installing from Git repository

Get the sources from GitHub (https://github.com/vslavik/poedit):

    git clone https://github.com/vslavik/poedit.git

If you are on Windows or OSX, you'll need all the dependencies too. After
cloning the repository, run the following command:

    git submodule update --init --recursive

On Linux and other Unices, only a subset of submodules is necessary, so you can
save some time and disk space by checking out only them:

    git submodule update --init deps/json deps/pugixml

When building for Unix/Linux, if you get the sources directly from the Git
repository, some generated files are not present. You have to run the
`./bootstrap` script to create them. After that, continue according to the
instructions above.

The `./bootstrap` script requires some additional tools to be installed:

 * AsciiDoc, xsltproc and xmlto to generate the manual page
 * gettext tools to create `.mo` files

On macOS and Windows, bootstrapping is not needed.


## License

Poedit is released under [the MIT license](COPYING) and you're free to do
whatever you want with it and its source code (well, almost :-) -- see the
license text).

See the `COPYING` file for details on the program's licensing.

Windows and macOS versions contain GNU gettext binaries. They are distributed
under the GNU General Public License and their source code is available from
http://www.gnu.org/software/gettext. If you have difficulties getting them
from there, email me for a copy of the sources.


## Links

1. [Poedit's website](https://poedit.net/)
2. [GNU gettext](http://www.gnu.org/software/gettext/)
