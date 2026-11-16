# The version comes from packaging/version.sh: rpmbuild --define "pkgver $(packaging/version.sh)"
Name:           glyphpop
Version:        %{pkgver}
Release:        1%{?dist}
Summary:        Emoji, kaomoji and symbol picker for Wayland
License:        GPL-3.0-or-later AND Unicode-3.0 AND MIT
URL:            https://github.com/ItzELECTR0/GlyphPop
Source:         %{name}.tar.gz
ExclusiveArch:  x86_64 aarch64

# Zig 0.17 is newer than Fedora's, so it is not listed and has to be on PATH.
BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  ninja-build
BuildRequires:  cmake(LayerShellQt) >= 6.6
BuildRequires:  cmake(Qt6DBus)
BuildRequires:  cmake(Qt6GuiPrivate)
BuildRequires:  cmake(Qt6Qml)
BuildRequires:  cmake(Qt6Quick)
BuildRequires:  cmake(Qt6WaylandClient)
BuildRequires:  cmake(Qt6WaylandClientPrivate)
BuildRequires:  pkgconfig(xkbcommon)

# Qt private API, so only the Qt this was built against will do.
%{?_qt6:Requires: %{_qt6}%{?_isa} = %{_qt6_version}}
Requires:       layer-shell-qt
Requires:       qt6qml(QtQuick)
Requires:       qt6qml(QtQuick.Effects)
Recommends:     google-noto-color-emoji-fonts
Recommends:     xdg-desktop-portal

%description
GlyphPop opens at the text cursor and types the emoji, kaomoji or symbol you
pick straight into the focused field.

%prep
%autosetup -n %{name}
cp data/unicode/LICENSE unicode-LICENSE

%build
%cmake -G Ninja
%cmake_build

%install
%cmake_install

%check
ZIG_GLOBAL_CACHE_DIR=$PWD/zig-cache zig build test --cache-dir $PWD/zig-test-cache

%files
%license LICENSE unicode-LICENSE
%{_bindir}/glyphpop
%{_datadir}/applications/net.electris.GlyphPop.desktop
%{_datadir}/icons/hicolor/scalable/apps/net.electris.GlyphPop.svg
