#!/bin/sh
# Build "Seven Kingdoms.app" from src/7kaa + data/ and install to /Applications.
# Usage: packaging/mkbundle-mac.sh   (run after `make`)
set -e
cd "$(dirname "$0")/.."
OUT=build/bundle; APP="$OUT/Seven Kingdoms.app"; C="$APP/Contents"
rm -rf "$APP"; mkdir -p "$C/MacOS" "$C/Frameworks" "$C/Resources"
cp -R data/. "$C/Resources/"; rm -rf "$C/Resources"/Makefile* "$C/Resources/__pycache__"
cp src/7kaa "$C/MacOS/7kaa"; chmod 755 "$C/MacOS/7kaa"
# Bundle non-system dylibs (SDL2-compat, gettext) and rewrite install names
for lib in $(otool -L "$C/MacOS/7kaa" | awk 'NR>1{print $1}' | grep -v '^/usr/lib\|^/System'); do
  bn=$(basename "$lib"); cp "$lib" "$C/Frameworks/$bn"; chmod 644 "$C/Frameworks/$bn"
  install_name_tool -change "$lib" "@executable_path/../Frameworks/$bn" "$C/MacOS/7kaa"
  install_name_tool -id "@executable_path/../Frameworks/$bn" "$C/Frameworks/$bn"
done
# sdl2-compat dlopens libSDL3.dylib via @loader_path — put it alongside
cp /usr/local/opt/sdl3/lib/libSDL3.0.dylib "$C/Frameworks/"; chmod 644 "$C/Frameworks/libSDL3.0.dylib"
ln -sf libSDL3.0.dylib "$C/Frameworks/libSDL3.dylib"
install_name_tool -id "@executable_path/../Frameworks/libSDL3.0.dylib" "$C/Frameworks/libSDL3.0.dylib"
# Icon from src/7k.ico
mkdir -p "$OUT/icon.iconset"; sips -s format png src/7k.ico --out "$OUT/icon.iconset/base.png" >/dev/null
for s in 16 32 128 256 512; do sips -z $s $s "$OUT/icon.iconset/base.png" --out "$OUT/icon.iconset/icon_${s}x${s}.png" >/dev/null; done
rm "$OUT/icon.iconset/base.png"; iconutil -c icns "$OUT/icon.iconset" -o "$C/Resources/7kaa.icns"; rm -rf "$OUT/icon.iconset"
cat > "$C/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>Seven Kingdoms</string>
  <key>CFBundleDisplayName</key><string>Seven Kingdoms</string>
  <key>CFBundleIdentifier</key><string>io.github.orchardroot.7kaa</string>
  <key>CFBundleExecutable</key><string>7kaa</string>
  <key>CFBundleIconFile</key><string>7kaa.icns</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleVersion</key><string>2.15.6-mod</string>
  <key>CFBundleShortVersionString</key><string>2.15.6-mod</string>
  <key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
codesign --force --deep --sign - "$APP"
rm -rf "/Applications/Seven Kingdoms.app"; mv "$APP" /Applications/
echo "Installed /Applications/Seven Kingdoms.app"
