package com.github.kr328.zloader.internal;

import android.util.Log;

import java.io.IOException;
import java.io.StringReader;
import java.util.Properties;

public final class Loader {
    private static final String TAG = "ZygoteLoader[Java]";
    private static final String DYNAMIC_PACKAGES_PATH = "/data/misc/zygote-loader";
    private static String dynamicPackagesPath;

    public static void load(final String packageName, final String properties) {
        final Properties prop = new Properties();

        try {
            prop.load(new StringReader(properties));
        } catch (IOException e) {
            Log.e(TAG, "Load properties: " + properties, e);

            return;
        }

        final String moduleId = prop.getProperty("id", "");
        final String moduleName = prop.getProperty("name", "");
        if (moduleId.isEmpty() || moduleName.isEmpty()) {
            Log.e(TAG, "Module id/name not found");

            return;
        }

        dynamicPackagesPath = DYNAMIC_PACKAGES_PATH + "/" + moduleId;

        final String entrypointName = prop.getProperty("entrypoint", "");

        try {
            final ClassLoader loader = Loader.class.getClassLoader();
            if (loader == null) {
                throw new ClassNotFoundException("ClassLoader of " + Loader.class + " unavailable");
            }

            loader.loadClass(entrypointName)
                    .getMethod("main", String.class, Properties.class)
                    .invoke(null, packageName, prop);
        } catch (ReflectiveOperationException e) {
            Log.e(moduleName, "Invoke main of " + entrypointName, e);
        }
    }

    public static String getDynamicPackagesPath() {
        return dynamicPackagesPath;
    }
}
