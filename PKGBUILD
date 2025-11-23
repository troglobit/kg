pkgname=kg-git
pkgver=r38.3ae5e73
pkgrel=1
pkgdesc="Antirez kilo with Emacs keybindings"
arch=('x86_64')
url="https://github.com/troglobit/kg"
license=('BSD-2')
makedepends=('git')
source=("$srcdir/$pkgname::git+https://github.com/troglobit/kg.git")
sha256sums=('SKIP')

pkgver() {
        cd "$srcdir/$pkgname"
        printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
        cd "$srcdir/$pkgname"
        make
}

package() {
        cd "$srcdir/$pkgname"
        install -sD --target-directory=$pkgdir/usr/bin kg
        install -Dm644 --target-directory=$pkgdir/usr/share/licenses/$pkgname LICENSE
        install -Dm644 --target-directory=$pkgdir/usr/share/doc/$pkgname README.md
}
