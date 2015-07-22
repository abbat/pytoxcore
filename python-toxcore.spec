Name:           python-toxcore
Version:        0.0.1
Release:        1
Summary:        Python binding for ToxCore
License:        GPL-3
Group:          Applications/Internet
URL:            https://github.com/abbat/pytoxcore
BuildRequires:  python-devel, tox-libtoxcore-devel, tox-libsodium-devel
Source0:        https://build.opensuse.org/source/home:antonbatenev:tox/%{name}/%{name}_%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

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
* Thu Jul 23 2015 Anton Batenev <antonbatenev@yandex.ru> - 0.0.1-1
- Initial
