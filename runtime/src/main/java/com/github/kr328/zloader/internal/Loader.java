package com.github.kr328.zloader.internal;

import android.os.Binder;
import android.os.Parcel;
import android.util.Log;

import androidx.annotation.Keep;
import androidx.annotation.RestrictTo;

import com.github.kr328.zloader.BuildConfig;

import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.WeakHashMap;

@SuppressWarnings({"JavaJniMissingFunction", "unused"})
@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class Loader {
    private static final String TAG = "ZygoteLoader[Java]";

    @Keep
    private static final WeakHashMap<Binder, Binder> redirectBinders = new WeakHashMap<>();

    private static String dynamicPackagesPath;
    private static String dataDirectory;
    private static String packageName;
    private static Map<String, String> properties;
    private static boolean isBinderInterceptorAvailable;

    @Keep
    private static native boolean nativeCallExecTransact(final Binder binder, final int code, final Parcel data, final Parcel reply, final int flags);

    @Keep
    private static void load(final String packageName, final ByteBuffer properties, final boolean isBinderInterceptorAvailable) {
        try {
            if (BuildConfig.DEBUG) {
                Log.d(TAG, "Loading in " + packageName);
            }

            init(packageName, StandardCharsets.UTF_8.decode(properties).toString(), isBinderInterceptorAvailable);
        } catch (final Throwable throwable) {
            Log.e(TAG, "doLoad: " + throwable, throwable);
        }
    }

    public static void init(final String packageName, final String propertiesText, final boolean isBinderInterceptorAvailable) {
        final Map<String, String> properties = new HashMap<>();

        for (final String line : propertiesText.split("\n")) {
            final String[] kv = line.split("=", 2);
            if (kv.length != 2)
                continue;

            properties.put(kv[0].trim(), kv[1].trim());
        }

        final String dataDirectory = properties.get("dataDirectory");
        if (dataDirectory == null) {
            Log.e(TAG, "Data directory not found");

            return;
        }

        final String entrypointName = properties.get("entrypoint");
        if (entrypointName == null) {
            Log.e(TAG, "Entrypoint not found");

            return;
        }

        Loader.dynamicPackagesPath = dataDirectory + "/packages";
        Loader.dataDirectory = dataDirectory;
        Loader.packageName = packageName;
        Loader.properties = Collections.unmodifiableMap(properties);
        Loader.isBinderInterceptorAvailable = isBinderInterceptorAvailable;

        try {
            final ClassLoader loader = Loader.class.getClassLoader();
            if (loader == null) {
                throw new ClassNotFoundException("ClassLoader of " + Loader.class + " unavailable");
            }

            loader.loadClass(entrypointName).getMethod("main").invoke(null);
        } catch (final ReflectiveOperationException e) {
            Log.e(TAG, "Invoke main of " + entrypointName, e);
        }
    }

    public static String getDynamicPackagesPath() {
        return dynamicPackagesPath;
    }

    public static String getDataDirectory() {
        return dataDirectory;
    }

    public static Map<String, String> getProperties() {
        return properties;
    }

    public static String getPackageName() {
        return packageName;
    }

    public static void registerRedirectBinder(final Binder source, final Binder target) {
        if (!isBinderInterceptorAvailable) {
            throw new IllegalStateException("Binder interceptor disabled");
        }

        synchronized (redirectBinders) {
            if (redirectBinders.containsKey(source)) {
                throw new IllegalStateException("Binder " + source + " already registered");
            }

            redirectBinders.put(source, target);
        }
    }

    public static boolean unregisterRedirectBinder(final Binder source) {
        if (isBinderInterceptorAvailable) {
            throw new IllegalStateException("Binder interceptor disabled");
        }

        synchronized (redirectBinders) {
            return redirectBinders.remove(source) != null;
        }
    }

    public static boolean callExecTransact(
            final Binder binder,
            final int code,
            final Parcel parcel,
            final Parcel reply,
            final int flags
    ) {
        if (isBinderInterceptorAvailable) {
            throw new IllegalStateException("Binder interceptor disabled");
        }

        return nativeCallExecTransact(binder, code, parcel, reply, flags);
    }
}
