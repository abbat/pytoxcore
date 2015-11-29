# Maintainer: Anton Batenev <antonbatenev@yandex.ru>

_pkgname=pytoxcore
pkgname=python-toxcore
pkgver=0.0.13
pkgrel=1
pkgdesc="Python binding for ToxCore"
arch=('i686' 'x86_64')
url="https://github.com/abbat/pytoxcore"
license=('GPL-3')
depends=('python2')
makedepends=('tox-libtoxcore' 'libvpx' 'opus' 'libsodium')
source=("https://build.opensuse.org/source/home:antonbatenev:tox/${pkgname}/${pkgname}_${pkgver}.tar.bz2")
sha512sums=('SKIP')

build() {
    cd "${_pkgname}"
    python2 setup.py build
}

package() {
    cd "${_pkgname}"

    python2 setup.py install --root=${pkgdir}/ --optimize=1

    install -d -m 755 "${pkgdir}/usr/share/doc/${pkgname}"

    install -m 644 README.md "${pkgdir}/usr/share/doc/${pkgname}/README.rst"
}
