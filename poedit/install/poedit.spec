# set this to 1 if you want to build semistatic rpm
%define        semistatic      0

%{?_with_semistatic: %{expand: %%define semistatic 1}}
%{?_without_semistatic: %{expand: %%define semistatic 0}}

# version and release
%define   VERSION     1.2.5
%define   RELEASE     1

%if %{semistatic}
  %define NAME        poedit-semistatic
%else
  %define NAME        poedit
%endif


Summary:       Gettext catalogs editor
Name:          %NAME
Version:       %VERSION
Release:       %RELEASE
License:       MIT
Group:         Applications/Editors
Source:        poedit-%{version}.tar.bz2
URL:           http://poedit.sourceforge.net
Packager:      Vaclav Slavik <vaclav.slavik@matfyz.cz>
Requires:      gtk+ >= 1.2.7 gettext

%if %{semistatic}
BuildRequires: wxGTK >= 2.4.1 wxGTK-devel wxGTK-static
Provides:      poedit
%else
Requires:      wxGTK >= 2.4.1
BuildRequires: wxGTK-devel
%endif
BuildRequires: zip

BuildRoot:     %{_tmppath}/%{name}-%{version}

%description
poEdit is cross-platform gettext catalogs (.po files) editor. It is built with
wxWindows toolkit and can run on Unix or Windows. It aims to provide convenient
way of editing gettext catalogs. It features UTF-8 support, fuzzy and
untranslated records highlighting, whitespaces highlighting, references
browser, headers editing and can be used to create new catalogs or update
existing catalogs from source code by single click.

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
rm -rf %{buildroot}
%makeinstall GNOME_DATA_DIR=/usr/share KDE_DATA_DIR=/usr/share

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


%find_lang poedit
%if %{semistatic}
%find_lang poedit-wxstd
cat poedit-wxstd.lang >>poedit.lang
%endif

%clean
rm -Rf %{buildroot}

%post
# This is done on Mandrake to update its menus:
if [ -x /usr/bin/update-menus ]; then /usr/bin/update-menus || true ; fi

%postun
# This is done on Mandrake to update its menus:
if [ "$1" = "0" -a -x /usr/bin/update-menus ]; then /usr/bin/update-menus || true ; fi


%files -f poedit.lang
%defattr(-,root,root)
%doc NEWS LICENSE README AUTHORS

%dir %{_datadir}/poedit
%{_datadir}/poedit/resources.zip
%{_datadir}/poedit/help.zip
%{_datadir}/poedit/help-gettext.zip
%lang(hr) %{_datadir}/poedit/help-hr.zip
%{_bindir}/poedit
%{_mandir}/*/*
%{_datadir}/mimelnk/application/*
%{_datadir}/applnk/Development/*
%{_datadir}/gnome/apps/Development/*
%{_datadir}/mime-info/poedit.*
%{_datadir}/icons/*
%{_datadir}/pixmaps/*
%{_libdir}/menu/*
