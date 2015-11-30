Name:           python-toxcore
Version:        0.0.14
Release:        1
Summary:        Python binding for ToxCore
License:        GPL-3
Group:          Applications/Internet
URL:            https://github.com/abbat/pytoxcore
BuildRequires:  python-devel
BuildRequires:  tox-libvpx-devel, tox-libsodium-devel, tox-libtoxcore-devel
Source0:        https://build.opensuse.org/source/home:antonbatenev:tox/%{name}/%{name}_%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%if 0%{?suse_version}
BuildRequires: libopus-devel >= 0.9.14
%else
BuildRequires: opus-devel >= 0.9.14
%endif

%description
PyToxCore provides a Pythonic binding, i.e Object-oriented instead of C style,
raise exception instead of returning error code.


%prep
%setup -q -n pytoxcore


%build
CFLAGS="-Wl,-Bsymbolic-functions" python setup.py build


%install
python setup.py install --prefix=%{buildroot}/usr


%files
%defattr(-,root,root,-)
%doc README.md AUTHORS

%{_libdir}/python*/site-packages/*


%changelog
* Thu Nov 30 2015 Anton Batenev <antonbatenev@yandex.ru> - 0.0.14-1
- Initial
