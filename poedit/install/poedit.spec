# Purpose:  The .spec file for building poEdit RPM

# version and release
%define VERSION 1.1.0
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
URL:       http://poedit.sourceforge.net
Packager:  Vaclav Slavik <v.slavik@volny.cz>
Prefix:    %prefix
Requires:  gtk+ >= 1.2.6
BuildRoot: /var/tmp/poedit-%{version}

%description
poEdit is cross-platform gettext catalogs (.po files) editor. It is built with
wxWindows toolkit and can run on Unix or Windows. It aims to provide convenient 
way of editing gettext catalogs. It features UTF-8 support, fuzzy and untranslated 
records highlighting, whitespaces highlighting, references browser, headers editing
and can be used to create new catalogs or update existing catalogs from source
code by single click.

%prep
%setup

%build
./configure --prefix=%prefix ${POEDIT_CONFIGURE_FLAGS}

%install
rm -rf ${RPM_BUILD_ROOT}
make prefix=${RPM_BUILD_ROOT}%{prefix} \
     GNOME_DATA_DIR=$RPM_BUILD_ROOT/usr/share \
     KDE_DATA_DIR=$RPM_BUILD_ROOT/usr/share \
     install

%clean
rm -Rf ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root)
%doc NEWS LICENSE README
%prefix/bin/poedit
%dir %prefix/share/poedit
%prefix/share/poedit/*
%prefix/share/mimelnk/application/x-po.kdelnk
%prefix/share/applnk/Development/poedit.kdelnk
%prefix/share/gnome/apps/Development/poedit.desktop
%prefix/share/mime-info/poedit.*
%prefix/share/icons/poedit.xpm
%prefix/share/pixmaps/poedit.xpm
