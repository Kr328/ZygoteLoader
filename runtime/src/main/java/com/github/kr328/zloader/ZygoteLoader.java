package com.github.kr328.zloader;

import com.github.kr328.zloader.internal.Loader;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.attribute.PosixFilePermissions;

public final class ZygoteLoader {
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
}
