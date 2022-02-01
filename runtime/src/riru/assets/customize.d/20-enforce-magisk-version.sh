# Enforce magisk version
# Supported version: 21000+

[ "$MAGISK_VER_CODE" -lt 21000 ] && abort "! Unsupported magisk version: $MAGISK_VER_CODE (21000+ required)"

ui_print "- Magisk version: $MAGISK_VER($MAGISK_VER_CODE)"
