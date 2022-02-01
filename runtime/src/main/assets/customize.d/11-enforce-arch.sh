# Enforce device arch
# Supported abis: arm64-v8a armeabi-v7a x86_64 x86

[ "$ARCH" != "arm" ] && [ "$ARCH" != "arm64" ] && [ "$ARCH" != "x86" ] && [ "$ARCH" != "x64" ] && abort "! Unsupported platform: $ARCH"

ui_print "- Device arch: $ARCH"