# Maintainer: Noah Burchell <nburchell5@gmail.com>

pkgname=cube
pkgver=1.1.1
pkgrel=1
pkgdesc="spinning cube (and the other platonic solids) for your terminal"
arch=('x86_64' 'aarch64')
url="https://github.com/noahburchell/cube"
license=('GPL-3.0-only')
depends=('glibc')
source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('f139e5142a0b1bd477638c5c2756c2bba86a260b89389c199206300c13c70f16')

build() {
	cd "$pkgname-$pkgver"
	make
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir" PREFIX=/usr install
}
