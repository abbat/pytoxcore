Name:           python-toxcore
Version:        0.0.19
Release:        1
Summary:        Python binding for ToxCore
License:        GPL-3
Group:          Applications/Internet
URL:            https://github.com/abbat/pytoxcore
BuildRequires:  python-devel
BuildRequires:  libvpx-devel, libsodium-devel >= 0.5.0
BuildRequires:  tox-libtoxcore-devel
Source0:        https://build.opensuse.org/source/home:antonbatenev:tox/%{name}/%{name}_%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%if 0%{?suse_version}
BuildRequires:  libopus-devel
%else
BuildRequires:  opus-devel
%endif

%description
PyToxCore provides a Pythonic binding, i.e Object-oriented instead of C style,
raise exception instead of returning error code.


%prep
%setup -q -n pytoxcore


%build
%if 0%{?suse_version}
CFLAGS="-Wl,-Bsymbolic-functions -fno-strict-aliasing" python setup.py build
%else
CFLAGS="-Wl,-Bsymbolic-functions" python setup.py build
%endif


%install
python setup.py install --prefix=%{buildroot}/usr


%files
%defattr(-,root,root,-)
%doc README.md AUTHORS

%{_libdir}/python*/site-packages/*


%changelog
* Tue Jan 15 2015 Anton Batenev <antonbatenev@yandex.ru> - 0.0.19-1
- Initial
