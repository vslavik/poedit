# Purpose:  The .spec file for building poEdit RPM

# set this to 1 if you want to build semistatic rpm
%define        semistatic      0

# set this to 1 if you want mandrakized RPM (menu entry)
%{expand:%%define _with_mdk %(if [ -f /etc/mandrake-release ]; then echo 1; else echo 0; fi)}

%{?_with_semistatic: %{expand: %%define semistatic 1}}
%{?_without_semistatic: %{expand: %%define semistatic 0}}

%{?_with_mdk: %{expand: %%define enable_mdk 1}}
%{?_without_mdk: %{expand: %%define enable_mdk 0}}



# version and release
%define        VERSION 1.1.9
%define        RELEASE 1

# default installation directory
%define prefix /usr

%if %{semistatic}
  %define NAME        poedit-semistatic
%else
  %define NAME        poedit
%endif


Summary:       Gettext catalogs editor
Name:          %NAME
Version:       %VERSION
%if !%{enable_mdk}
Release:       %RELEASE
%else
Release:       %{RELEASE}mdk
%endif
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

%if %{semistatic}
%configure --enable-semistatic
%else
%configure
%endif
)

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall GNOME_DATA_DIR=$RPM_BUILD_ROOT/usr/share \
             KDE_DATA_DIR=$RPM_BUILD_ROOT/usr/share

%if %{enable_mdk}
(cd $RPM_BUILD_ROOT
mkdir -p ./%{_libdir}/menu
cat > ./%{_libdir}/menu/poedit <<EOF 
?package(%{name}): \
	command="%{_bindir}/poedit"\\
	needs="X11"\\
	section="Applications/Development/Tools"\\
	icon="poedit.xpm"\\
	mimetypes="application/x-po;application/x-gettext"\\
	title="poEdit"\\
	longtitle="poEdit Gettext Catalogs Editor"
EOF
)
%endif


%find_lang poedit
%if %{semistatic}
%find_lang poedit-wxstd
cat poedit-wxstd.lang >>poedit.lang
%endif

%clean
rm -Rf ${RPM_BUILD_ROOT}

%if %{enable_mdk}
%post
%update_menus

%postun
%clean_menus
%endif

%files -f poedit.lang
%defattr(-,root,root)
%doc NEWS LICENSE README AUTHORS

%dir %{_datadir}/poedit
%{_datadir}/poedit/*
%{_bindir}/poedit
%{_mandir}/*/*
%{_datadir}/mimelnk/application/*
%{_datadir}/applnk/Development/*
%{_datadir}/gnome/apps/Development/*
%{_datadir}/mime-info/poedit.*
%{_datadir}/icons/*
%{_datadir}/pixmaps/*

%if %{enable_mdk}
%{_libdir}/menu/*
%endif
