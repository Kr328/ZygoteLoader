package com.github.kr328.zloader;

import com.github.kr328.zloader.internal.Loader;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.attribute.PosixFilePermissions;
import java.util.Collections;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

public final class ZygoteLoader {
    /**
     * Package name that indicate currently in system_server
     *
     * {@link #getPackageName}
     */
    public static final String PACKAGE_SYSTEM_SERVER = ".android";


    /**
     * Set should ZygoteLoader inject to {@code packageName}
     *
     * @param packageName target package name
     * @param enabled should inject to {@code packageName}
     * @throws IOException if set status failed
     */
    public static void setPackageEnabled(String packageName, boolean enabled) throws IOException {
        if (packageName.isEmpty())
            return;

        Files.createDirectories(
                Paths.get(Loader.getDynamicPackagesPath()),
                PosixFilePermissions.asFileAttribute(
                        PosixFilePermissions.fromString("r-x------")
                )
        );

        final Path path = Paths.get(Loader.getDynamicPackagesPath(), packageName);
        if (enabled) {
            Files.createFile(path, PosixFilePermissions.asFileAttribute(
                    PosixFilePermissions.fromString("r--------"))
            );
        } else {
            Files.deleteIfExists(path);
        }
    }

    /**
     * Get target packages enabled
     *
     * @param packageName target package name
     * @return package enabled
     */
    public static boolean isPackageEnabled(String packageName) {
        return Files.isRegularFile(Paths.get(Loader.getDynamicPackagesPath(), packageName));
    }

    /**
     * List all enabled packages
     *
     * @return enabled package names
     */
    public static Set<String> getEnabledPackages() {
        try {
            return Files.list(Paths.get(Loader.getDynamicPackagesPath()))
                    .map(Path::getFileName)
                    .map(Path::toString)
                    .collect(Collectors.toSet());
        } catch (IOException e) {
            return Collections.emptySet();
        }
    }

    /**
     * Disable all enabled packages
     */
    public static void disableAllPackages() {
        try {
            Files.list(Paths.get(Loader.getDynamicPackagesPath()))
                    .forEach(p -> {
                        try {
                            Files.delete(p);
                        } catch (IOException ignored) {
                            // ignored
                        }
                    });
        } catch (IOException ignored) {
            // ignored
        }
    }

    /**
     * Get module data directory path
     *
     * @return module data directory
     */
    public static String getDataDirectory() {
        return Loader.getDataDirectory();
    }

    /**
     * Get currently injected package name
     *
     * @return package name
     */
    public static String getPackageName() {
        return Loader.getPackageName();
    }

    /**
     * Get properties that read from module.prop
     *
     * @return map of module.prop
     */
    public static Map<String, String> getProperties() {
        return Loader.getProperties();
    }
}
