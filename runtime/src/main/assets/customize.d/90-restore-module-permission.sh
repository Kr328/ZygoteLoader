# Restore module directory permissions

ui_print "- Restore module permissions"
set_perm_recursive "$MODPATH"          0    0    0755 0644
set_perm           "$DATA_PATH"        1000 1000 0700 0600 u:object_r:system_data_file:s0
set_perm           "$MODULE_DATA_PATH" 1000 1000 0700 0600 u:object_r:system_data_file:s0
