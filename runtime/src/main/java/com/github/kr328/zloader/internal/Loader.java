package com.github.kr328.zloader.internal;

import android.util.Log;
import android.util.Pair;

import java.io.IOException;
import java.io.StringReader;
import java.util.Collections;
import java.util.Map;
import java.util.Properties;
import java.util.stream.Collectors;

public final class Loader {
    private static final String TAG = "ZygoteLoader[Java]";
    private static final String DYNAMIC_PACKAGES_PATH = "/data/misc/zygote-loader";
    private static String dynamicPackagesPath;

    private static String packageName;
    private static Map<String, String> properties;

    public static void load(final String packageName, final String properties) {
        init(packageName, properties);
    }

    public static void init(final String packageName, final String properties) {
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
        if (entrypointName.isEmpty()) {
            Log.e(TAG, "Entry of module " + moduleName + " not found");

            return;
        }

        Loader.packageName = packageName;
        Loader.properties = prop.entrySet().stream()
                .map((entry) -> new Pair<>((String) entry.getKey(), entry.getValue().toString()))
                .collect(Collectors.toMap(p -> p.first, p -> p.second));
        Loader.properties = Collections.unmodifiableMap(Loader.properties);

        try {
            final ClassLoader loader = Loader.class.getClassLoader();
            if (loader == null) {
                throw new ClassNotFoundException("ClassLoader of " + Loader.class + " unavailable");
            }

            loader.loadClass(entrypointName).getMethod("main").invoke(null);
        } catch (ReflectiveOperationException e) {
            Log.e(moduleName, "Invoke main of " + entrypointName, e);
        }
    }

    public static String getDynamicPackagesPath() {
        return dynamicPackagesPath;
    }

    public static Map<String, String> getProperties() {
        return properties;
    }

    public static String getPackageName() {
        return packageName;
    }
}
