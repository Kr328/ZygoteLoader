# Restore module directory permissions

ui_print "- Restore permissions"
set_perm_recursive "$MODPATH" 0 0 0755 0644
