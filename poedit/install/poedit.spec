# Purpose:  The .spec file for building poEdit RPM

# version and release
%define VERSION 1.0.2
%define RELEASE 1


# default installation directory
%define prefix /usr


Summary:   Gettext catalogs editor
Name:      poedit
Version:   %VERSION
Release:   %RELEASE
Copyright: BSD license
Group:     Applications/Editors
Source:    poedit-%{VERSION}.tar.gz
URL:       http://www.volny.cz/v.slavik/poedit/
Packager:  Vaclav Slavik <v.slavik@volny.cz>
Prefix:    %prefix
Requires:  gtk+ >= 1.2.6
BuildRoot: /tmp/poedit-%{version}

%description
poEdit is cross-platform gettext catalogs (.po files) editor. It is built with
wxWindows toolkit and can run on Unix or Windows. It aims to provide convenient 
way of editing gettext catalogs. It features fuzzy and untranslated records
highlighting, whitespaces highlighting, references browser, headers editing
and can be used to create new catalogs or update existing catalogs from source
code by single click.

%prep
%setup

%build
./configure --prefix=%prefix

%install
make prefix=$RPM_BUILD_ROOT%{prefix} GNOME_DATA_DIR=$RPM_BUILD_ROOT/usr/share KDE_DATA_DIR=$RPM_BUILD_ROOT/usr/share install-strip


%clean
rm -Rf ${RPM_BUILD_ROOT}

%postun
rm -Rf %prefix/share/poedit
rmdir %prefix/share/mimelnk/application
rmdir %prefix/share/applnk/Development
rmdir %prefix/share/gnome/apps/Development
rmdir %prefix/share/mime-info
rmdir %prefix/share/icons
rmdir %prefix/share/pixmaps

%files
%defattr(-, root, root)
%doc ChangeLog LICENSE README
%prefix/bin/poedit
%prefix/share/poedit/poedit_help.htb
%prefix/share/mimelnk/application/x-po.kdelnk
%prefix/share/applnk/Development/poedit.kdelnk
%prefix/share/gnome/apps/Development/poedit.desktop
%prefix/share/mime-info/poedit.*
%prefix/share/icons/poedit.xpm
%prefix/share/pixmaps/poedit.xpm
