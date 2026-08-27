# Maintainer: Noah Burchell <nburchell5@gmail.com>

pkgname=cube
pkgver=1.2
pkgrel=1
pkgdesc="spinning cube (and the other platonic solids) for your terminal"
arch=('x86_64' 'aarch64')
url="https://github.com/noahburchell/cube"
license=('GPL-3.0-only')
depends=('glibc')
source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('2f734cfa572f5a630a3781541d36b7a44f005840eaf7675a44cc660e2fb9999d')

build() {
	cd "$pkgname-$pkgver"
	make
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir" PREFIX=/usr install
}
