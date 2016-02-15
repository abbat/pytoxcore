# Maintainer: Anton Batenev <antonbatenev@yandex.ru>

_pkgname=pytoxcore
pkgname=python-toxcore
pkgver=0.1.1
pkgrel=2
pkgdesc="Python binding for ToxCore"
arch=('i686' 'x86_64')
url="https://github.com/abbat/pytoxcore"
license=('GPL-3')
depends=('python' 'toxcore')
makedepends=('git' 'python' 'toxcore')
source=("https://build.opensuse.org/source/home:antonbatenev:tox/${pkgname}/${pkgname}_${pkgver}.tar.bz2")
sha512sums=('SKIP')

build() {
    cd "${_pkgname}"

    python setup.py build
}

package() {
    cd "${_pkgname}"

    python setup.py install --root=${pkgdir}/ --optimize=1

    install -d -m 755 "${pkgdir}/usr/share/doc/${pkgname}"

    install -m 644 README.md "${pkgdir}/usr/share/doc/${pkgname}/README.rst"
}
