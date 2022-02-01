# Restore module directory permissions

ui_print "- Restore permissions"
set_perm_recursive "$MODPATH"          0    0    0755 0644
set_perm_recursive "$MODULE_DATA_PATH" 1000 1000 0700 0600
