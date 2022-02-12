# Generate ZygoteLoader data directory

DATA_PATH_FILE="/data/adb/zloader/data-root-directory"

if [ ! -f "$DATA_PATH_FILE" ]; then
  ui_print "- Initialize data directory"

  mkdir -p "$(dirname "$DATA_PATH_FILE")"

  RANDOM_SUFFIX="const"

  [ -e "/dev/urandom" ] && RANDOM_SUFFIX="$(tr -dc A-Za-z0-9 < "/dev/urandom" | head -c 16)"

  echo -n "/data/system/zloader-$RANDOM_SUFFIX" > "$DATA_PATH_FILE"
fi

DATA_PATH="$(cat "$DATA_PATH_FILE" | head -n 1 | xargs printf)"
MODULE_DATA_PATH="$DATA_PATH/$(grep_prop id "$MODPATH/module.prop")"

ui_print "- Initialize module data directory"

mkdir -p "$MODULE_DATA_PATH"

echo "dataDirectory=$MODULE_DATA_PATH" >> "$MODPATH/module.prop"
