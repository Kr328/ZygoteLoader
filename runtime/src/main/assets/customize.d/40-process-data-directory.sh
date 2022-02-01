# Generate ZygoteLoader data directory

ui_print "- Process data directory"

if [ ! -f "/data/adb/zloader/data-root-directory" ]; then
  mkdir -p "/data/adb/zloader"

  RANDOM_SUFFIX="const"

  [ -e "/dev/urandom" ] && RANDOM_SUFFIX=$(tr -dc A-Za-z0-9 < /dev/urandom | head -c 16)

  echo -n "/data/system/zloader-$RANDOM_SUFFIX" > "/data/adb/zloader/data-root-directory"
fi

DATA_PATH=$(cat "/data/adb/zloader/data-root-directory")
MODULE_DATA_PATH="$DATA_PATH/$(grep_prop id "$MODPATH/module.prop")"

mkdir -p "$MODULE_DATA_PATH"

echo "dataDirectory=$MODULE_DATA_PATH" >> "$MODPATH/module.prop"
