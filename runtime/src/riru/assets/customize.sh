# Sync with src/main/cpp/loader_riru.cpp
export RIRU_MODULE_API_VERSION=26
export RIRU_MODULE_MIN_API_VERSION=26
export RIRU_MODULE_MIN_RIRU_VERSION_NAME="26.0.0"

if [ "$MAGISK_VER_CODE" -ge 21000 ]; then
  MAGISK_CURRENT_RIRU_MODULE_PATH=$(magisk --path)/.magisk/modules/riru-core
else
  MAGISK_CURRENT_RIRU_MODULE_PATH=/sbin/.magisk/modules/riru-core
fi

if [ ! -d $MAGISK_CURRENT_RIRU_MODULE_PATH ]; then
  ui_print "*********************************************************"
  ui_print "! Riru is not installed"
  ui_print "! Please install Riru from Magisk Manager or https://github.com/RikkaApps/Riru/releases"
  abort    "*********************************************************"
fi

if [ -f "$MAGISK_CURRENT_RIRU_MODULE_PATH/disable" ] || [ -f "$MAGISK_CURRENT_RIRU_MODULE_PATH/remove" ]; then
  ui_print "*********************************************************"
  ui_print "! Riru is not enabled or will be removed"
  ui_print "! Please enable Riru in Magisk first"
  abort    "*********************************************************"
fi

if [ -f $MAGISK_CURRENT_RIRU_MODULE_PATH/util_functions.sh ]; then
  ui_print "- Load $MAGISK_CURRENT_RIRU_MODULE_PATH/util_functions.sh"
  # shellcheck disable=SC1090
  . $MAGISK_CURRENT_RIRU_MODULE_PATH/util_functions.sh
else
  ui_print "*********************************************************"
  ui_print "! Riru $RIRU_MODULE_MIN_RIRU_VERSION_NAME or above is required"
  ui_print "! Please upgrade Riru from Magisk Manager or https://github.com/RikkaApps/Riru/releases"
  abort    "*********************************************************"
fi

check_riru_version
enforce_install_from_magisk_app

MODULE_MIN_SDK=$(grep_prop minSdkVersion "$MODPATH/module.prop")
MODULE_MAX_SDK=$(grep_prop maxSdkVersion "$MODPATH/module.prop")

[ -n "$MODULE_MIN_SDK" ] && [ "$API" -lt "$MODULE_MIN_SDK" ] && abort "! Unsupported API: $API min $MODULE_MIN_SDK"
[ -n "$MODULE_MAX_SDK" ] && [ "$API" -gt "$MODULE_MAX_SDK" ] && abort "! Unsupported API: $API max $MODULE_MAX_SDK"

if [ "$ARCH" != "arm" ] && [ "$ARCH" != "arm64" ] && [ "$ARCH" != "x86" ] && [ "$ARCH" != "x64" ]; then
  abort "! Unsupported platform: $ARCH"
fi

ui_print "- Device platform: $ARCH($API)"

if [ -f "$MODPATH/setup.sh" ]; then
  . "$MODPATH/setup.sh"
fi

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

ui_print "- Restore permissions"
set_perm_recursive "$MODPATH" 0 0 0755 0644