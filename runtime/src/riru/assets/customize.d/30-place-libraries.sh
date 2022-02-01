# Place Riru libraries

if [ "$ARCH" = "arm" ] || [ "$ARCH" = "arm64" ]; then
  ui_print "- Placing arm(64) libraries"

  mv "$MODPATH/riru/armeabi-v7a" "$MODPATH/riru/lib"
  mv "$MODPATH/riru/arm64-v8a" "$MODPATH/riru/lib64"
  rm -rf "$MODPATH/riru/x86" "$MODPATH/riru/x86_64"
fi

if [ "$ARCH" = "x86" ] || [ "$ARCH" = "x64" ]; then
  ui_print "- Placing x86(_64) libraries"

  mv "$MODPATH/riru/x86" "$MODPATH/riru/lib"
  mv "$MODPATH/riru/x86_64" "$MODPATH/riru/lib64"
  rm -rf "$MODPATH/riru/armeabi-v7a" "$MODPATH/riru/arm64-v8a"
fi
