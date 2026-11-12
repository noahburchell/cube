# Maintainer: Noah Burchell <cube@nburch.org>

pkgname=cube
pkgver=1.2.2
pkgrel=1
pkgdesc="spinning cube (and the other platonic solids) for your terminal"
arch=('x86_64' 'aarch64')
url="https://github.com/noahburchell/cube"
license=('GPL-3.0-only')
depends=('glibc')
source=("$pkgname-$pkgver.tar.gz::$url/releases/download/v$pkgver/$pkgname-$pkgver.tar.gz")
sha256sums=('aa130ea3b8620b6d0382ee887cffc8507504bdb34842d199d805f0d763eef522')

build() {
	cd "$pkgname-$pkgver"
	make
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir" PREFIX=/usr install
}
