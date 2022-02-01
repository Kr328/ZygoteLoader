# Enforce riru env

MAGISK_CURRENT_RIRU_MODULE_PATH=$(magisk --path)/.magisk/modules/riru-core

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
