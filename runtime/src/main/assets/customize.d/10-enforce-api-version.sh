# Enforce android sdk version
# from module.prop

MODULE_MIN_SDK=$(grep_prop minSdkVersion "$MODPATH/module.prop")
MODULE_MAX_SDK=$(grep_prop maxSdkVersion "$MODPATH/module.prop")

[ -n "$MODULE_MIN_SDK" ] && [ "$API" -lt "$MODULE_MIN_SDK" ] && abort "! Unsupported API: $API min $MODULE_MIN_SDK"
[ -n "$MODULE_MAX_SDK" ] && [ "$API" -gt "$MODULE_MAX_SDK" ] && abort "! Unsupported API: $API max $MODULE_MAX_SDK"

ui_print "- Device platform: $API"
