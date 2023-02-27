# Place Zygisk libraries

if [ "$ARCH" = "arm" ] || [ "$ARCH" = "arm64" ]; then
  ui_print "- Placing arm(64) libraries"

  rm -rf "$MODPATH/zygisk/x86.so" "$MODPATH/zygisk/x86_64.so"
fi

if [ "$ARCH" = "x86" ] || [ "$ARCH" = "x64" ]; then
  ui_print "- Placing x86(_64) libraries"

  rm -rf "$MODPATH/zygisk/armeabi-v7a.so" "$MODPATH/zygisk/arm64-v8a.so"
fi
