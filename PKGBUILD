# Maintainer: Noah Burchell <cube@nburch.org>

pkgname=cube
pkgver=1.2.1
pkgrel=1
pkgdesc="spinning cube (and the other platonic solids) for your terminal"
arch=('x86_64' 'aarch64')
url="https://github.com/noahburchell/cube"
license=('GPL-3.0-only')
depends=('glibc')
source=("$pkgname-$pkgver.tar.gz::$url/releases/download/v$pkgver/$pkgname-$pkgver.tar.gz")
sha256sums=('15a73225cb371d30a7587d597bfab689a3cdbe130e587c034219a32cc00384b3')

build() {
	cd "$pkgname-$pkgver"
	make
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir" PREFIX=/usr install
}
