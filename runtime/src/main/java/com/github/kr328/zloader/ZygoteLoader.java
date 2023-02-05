package com.github.kr328.zloader;

import androidx.annotation.NonNull;

import com.github.kr328.zloader.internal.Loader;

import java.io.IOException;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.attribute.BasicFileAttributes;
import java.nio.file.attribute.PosixFilePermissions;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * Utilize to manage ZygoteLoader.
 */
public final class ZygoteLoader {
    /**
     * Package name that indicate currently in system_server
     * <p>
     * {@link #getPackageName}
     */
    public static final String PACKAGE_SYSTEM_SERVER = ".android";


    /**
     * Set should ZygoteLoader inject to {@code packageName}
     *
     * @param packageName target package name
     * @param enabled     should inject to {@code packageName}
     * @throws IOException if permission denied
     */
    public static void setPackageEnabled(@NonNull final String packageName, final boolean enabled) throws IOException {
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
     * @throws IOException if permission denied
     */
    public static boolean isPackageEnabled(@NonNull final String packageName) throws IOException {
        return Files.readAttributes(
                Paths.get(Loader.getDynamicPackagesPath(), packageName),
                BasicFileAttributes.class
        ).isRegularFile();
    }

    /**
     * List all enabled packages.
     *
     * @return enabled package names
     * @throws IOException if permission denied
     */
    @NonNull
    public static Set<String> getEnabledPackages() throws IOException {
        try (final Stream<Path> files = Files.list(Paths.get(Loader.getDynamicPackagesPath()))) {
            return files
                    .map(Path::getFileName)
                    .map(Path::toString)
                    .collect(Collectors.toSet());
        }
    }

    /**
     * Disable all enabled packages
     *
     * @throws IOException if permission denied
     */
    public static void disableAllPackages() throws IOException {
        try (final DirectoryStream<Path> files = Files.newDirectoryStream(Paths.get(Loader.getDynamicPackagesPath()))) {
            for (final Path file : files) {
                Files.delete(file);
            }
        }
    }

    /**
     * Get module data directory path
     *
     * @return module data directory
     */
    @NonNull
    public static String getDataDirectory() {
        return Loader.getDataDirectory();
    }

    /**
     * Get currently injected package name
     *
     * @return package name
     */
    @NonNull
    public static String getPackageName() {
        return Loader.getPackageName();
    }

    /**
     * Get properties that read from module.prop
     *
     * @return map of module.prop
     */
    @NonNull
    public static Map<String, String> getProperties() {
        return Loader.getProperties();
    }
}
