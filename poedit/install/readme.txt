


                                 ------------
                                    poEdit
                                 ------------
                                  
              a cross-platform Gettext catalogs editing tool


 About
-------

This program is GUI frontend to GNU Gettext utilities (win32 version
is part of the distribution) and catalogs editor/source code parser. It helps
with translating application into another language. For details on principles
of the solution used, see GNU Gettext documentation or wxWindows' wxLocale 
class reference.


 Installing
------------

poEdit is built on top of wxWindows toolkit, so it requires it at 
compilation time (version 2.2.1 or newer, see http://www.wxwindows.org).

Under Unix, do "./configure && make && make install" as usual.

Win32 version is distributed as precompiled binary with Gettext utilities
included. You don't need wxWindows for it. If you want to compile poEdit
yourself, you're on your own.


 License
---------

poEdit is released under BSD style license and you're free to do 
whatever you want with it and its source code (well, almost :-).
See LICENSE file for details.

Win32 version contains GNU Gettext binaries as compiled by Franco Bez.
They are distributed under the GNU General Public License.


 Author
--------
 
Vaclav Slavik <v.slavik@volny.cz>


 Links
-------

http://www.volny.cz/v.slavik/poEdit
        - poEdit homepage
http://www.wxwindows.org
        - wxWindows toolkit homepage
http://www.gnu.org
        - GNU project homepage, contains Gettext and documentation
http://www.jordanr.dhs.org/isinfo.htm
http://www.wintax.nl/isx
        - Inno Setup and Inno Setup Extensions, installation
          program used by poEdit under Windows
