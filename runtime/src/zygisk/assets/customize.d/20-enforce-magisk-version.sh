# Enforce magisk version
# Supported version: 24000+

[ "$MAGISK_VER_CODE" -lt 24000 ] && abort "! Unsupported magisk version: $MAGISK_VER_CODE (24000+ required)"

ui_print "- Magisk version: $MAGISK_VER($MAGISK_VER_CODE)"
