# Purpose:  The .spec file for building poEdit RPM

# set this to 1 if you want to build semistatic rpm
%define        semistatic    0

# version and release
%define        VERSION 1.1.6
%define        RELEASE 1

# default installation directory
%define prefix /usr

%if %{semistatic}
  %define NAME        poedit-semistatic
  %define semiconfig  --enable-semistatic
%else
  %define NAME        poedit
  %define semiconfig
%endif


Summary:       Gettext catalogs editor
Name:          %NAME
Version:       %VERSION
Release:       %RELEASE
Copyright:     MIT license
Group:         Applications/Editors
Source:        poedit-%{version}.tar.bz2
URL:           http://poedit.sourceforge.net
Packager:      Vaclav Slavik <vaclav.slavik@matfyz.cz>
Prefix:        %prefix
Requires:      gtk+ >= 1.2.7 gettext

%if %{semistatic}
BuildRequires: wxGTK >= 2.3.2 wxGTK-devel wxGTK-static
Provides:      poedit
%else
Requires:      wxGTK >= 2.3.2
BuildRequires: wxGTK-devel
%endif

BuildRoot: /var/tmp/poedit-%{version}

%description
poEdit is cross-platform gettext catalogs (.po files) editor. It is built with
wxWindows toolkit and can run on Unix or Windows. It aims to provide convenient 
way of editing gettext catalogs. It features UTF-8 support, fuzzy and untranslated 
records highlighting, whitespaces highlighting, references browser, headers editing
and can be used to create new catalogs or update existing catalogs from source
code by single click.

%prep
%setup -n poedit-%{version}

%build
(
export KDEDIR=/usr 
%configure ${POEDIT_CONFIGURE_FLAGS} %{semiconfig}
)

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall GNOME_DATA_DIR=$RPM_BUILD_ROOT/usr/share \
             KDE_DATA_DIR=$RPM_BUILD_ROOT/usr/share

%find_lang poedit
%if %{semistatic}
%find_lang poedit-wxstd
cat poedit-wxstd.lang >>poedit.lang
%endif

%clean
rm -Rf ${RPM_BUILD_ROOT}

%files -f poedit.lang
%defattr(-,root,root)
%doc NEWS LICENSE README AUTHORS
%{_bindir}/poedit
%dir %{_datadir}/poedit
%{_mandir}/*/*
%{_datadir}/poedit/*
%{_datadir}/mimelnk/application/x-po.kdelnk
%{_datadir}/applnk/Development/poedit.kdelnk
%{_datadir}/gnome/apps/Development/poedit.desktop
%{_datadir}/mime-info/poedit.*
%{_datadir}/icons/poedit.xpm
%{_datadir}/pixmaps/poedit.xpm
