package com.github.kr328.gradle.zygote;

import com.android.build.api.variant.ApplicationVariant;
import com.android.build.api.variant.VariantOutput;
import com.github.kr328.gradle.zygote.tasks.ChecksumTask;
import com.github.kr328.gradle.zygote.tasks.CustomizeTask;
import com.github.kr328.gradle.zygote.tasks.PackagesTask;
import com.github.kr328.gradle.zygote.tasks.PropertiesTask;
import com.github.kr328.gradle.zygote.util.StringUtils;

import org.gradle.api.Project;
import org.gradle.api.Task;
import org.gradle.api.file.FileCollection;
import org.gradle.api.provider.Provider;
import org.gradle.api.tasks.Sync;
import org.gradle.api.tasks.TaskProvider;
import org.gradle.api.tasks.bundling.Zip;
import org.gradle.api.tasks.bundling.ZipEntryCompression;

import java.io.File;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Objects;
import java.util.stream.Stream;

import javax.annotation.Nullable;

public final class ZygoteLoaderDecorator {
    private final Project project;
    private final ZygoteLoaderExtension extension;

    public ZygoteLoaderDecorator(final Project project, final ZygoteLoaderExtension extension) {
        this.project = project;
        this.extension = extension;
    }

    public void decorateVariant(final Loader loader, final ApplicationVariant variant) {
        final TaskProvider<Task> pack = project.getTasks().named("package" + StringUtils.capitalize(variant.getName()));
        final VariantOutput variantOutput = variant.getOutputs().stream().findAny().orElseThrow();

        final Map<String, String> moduleProp = new LinkedHashMap<>();

        moduleProp.put("version", variantOutput.getVersionName().getOrElse(""));
        moduleProp.put("versionCode", variantOutput.getVersionCode().getOrElse(0).toString());
        moduleProp.put("minSdkVersion", String.valueOf(variant.getMaxSdk()));
        if (variant.getMaxSdk() != null) {
            moduleProp.put("maxSdkVersion", String.valueOf(variant.getMaxSdk()));
        }

        if (loader == Loader.Riru) {
            moduleProp.putAll(extension.getRiru());
        } else if (loader == Loader.Zygisk) {
            moduleProp.putAll(extension.getZygisk());
        }

        final TaskProvider<PropertiesTask> generateModuleProp = project.getTasks().register(
                "generateModuleProp" + StringUtils.capitalize(variant.getName()),
                PropertiesTask.class,
                task -> {
                    task.getDestinationFile().set(
                            project.getLayout().getBuildDirectory()
                                    .dir("generated/properties/" + variant.getName())
                                    .map(p -> p.file("module.prop"))
                    );
                    task.getProperties().set(moduleProp);
                }
        );

        final TaskProvider<PackagesTask> generateInitialPackages = project.getTasks().register(
                "generateInitialPackages" + StringUtils.capitalize(variant.getName()),
                PackagesTask.class,
                task -> {
                    task.getDestinationDirectory().set(
                            project.getLayout().getBuildDirectory()
                                    .dir("generated/packages/" + variant.getName())
                    );
                    task.getPackages().set(extension.getPackages());
                }
        );

        final TaskProvider<Sync> mergeMagisk = project.getTasks().register(
                "mergeMagisk" + StringUtils.capitalize(variant.getName()),
                Sync.class,
                task -> {
                    task.into(project.getLayout().getBuildDirectory().dir("intermediates/merged_magisk/" + variant.getName()));

                    final Provider<File> apkFile = pack.map(p ->
                            p.getOutputs().getFiles().getFiles().stream()
                                    .flatMap(f -> {
                                        if (f.isDirectory()) {
                                            return Stream.of(Objects.requireNonNull(f.listFiles()));
                                        } else {
                                            return Stream.of(f);
                                        }
                                    })
                                    .filter(f -> f.getName().endsWith(".apk"))
                                    .findAny()
                                    .orElseThrow()
                    );

                    final Provider<FileCollection> apk = apkFile.map(project::zipTree);

                    task.getInputs().file(apkFile);

                    // module prop
                    task.from(generateModuleProp);

                    // initial packages
                    task.from(generateInitialPackages, sp -> sp.into("packages"));

                    // classes.dex
                    task.from(apk, sp -> sp.include("classes.dex"));

                    // native libraries
                    task.from(apk, sp -> {
                        sp.include("lib/*/*.so");
                        sp.eachFile(file -> {
                            final String abi = file.getPath().split("/")[1];

                            if (loader == Loader.Riru) {
                                file.setPath("riru/" + abi + "/lib" + moduleProp.get("id") + ".so");
                            } else if (loader == Loader.Zygisk) {
                                file.setPath("zygisk/" + abi + ".so");
                            }
                        });
                    });

                    // assets
                    task.from(apk, sp -> {
                        sp.include("assets/**");
                        sp.eachFile(file -> file.setPath(file.getPath().substring("assets/".length())));
                    });
                }
        );

        final TaskProvider<ChecksumTask> generateChecksum = project.getTasks().register(
                "generateChecksum" + StringUtils.capitalize(variant.getName()),
                ChecksumTask.class,
                task -> {
                    task.getDestinationFile().set(
                            project.getLayout().getBuildDirectory()
                                    .dir("generated/checksum/" + variant.getName())
                                    .map(p -> p.file("00-verify-resources.sh"))
                    );
                    task.getRootDirectory().set(mergeMagisk.map(m ->
                            project.getLayout().getProjectDirectory()
                                    .dir(m.getDestinationDir().getAbsolutePath()))
                    );
                }
        );

        final TaskProvider<CustomizeTask> generateCustomize = project.getTasks().register(
                "generateCustomize" + StringUtils.capitalize(variant.getName()),
                CustomizeTask.class,
                task -> {
                    task.getDestinationFile().set(
                            project.getLayout().getBuildDirectory()
                                    .dir("generated/customize/" + variant.getName())
                                    .map(p -> p.file("customize.sh"))
                    );
                    task.getMergedDirectory().set(mergeMagisk.map(m ->
                            project.getLayout().getProjectDirectory()
                                    .dir(m.getDestinationDir().getAbsolutePath()))
                    );
                    task.getChecksumFileName().set(generateChecksum.map(c -> c.getDestinationFile().get().getAsFile().getName()));
                }
        );

        final TaskProvider<Zip> zipMagisk = project.getTasks().register(
                "zipMagisk" + StringUtils.capitalize(variant.getName()),
                Zip.class,
                zip -> {
                    zip.getDestinationDirectory().set(
                            project.getLayout().getBuildDirectory()
                                    .dir("outputs/magisk/" + variant.getFlavorName() + "/" + variant.getBuildType())
                    );

                    zip.getArchiveBaseName().set(project.getName());
                    if (loader == Loader.Riru && extension.getRiru().containsKey("archiveName")) {
                        zip.getArchiveBaseName().set(extension.getRiru().get("archiveName"));
                    } else if (loader == Loader.Zygisk && extension.getZygisk().containsKey("archiveName")) {
                        zip.getArchiveBaseName().set(extension.getZygisk().get("archiveName"));
                    }

                    zip.setIncludeEmptyDirs(false);
                    zip.setEntryCompression(ZipEntryCompression.DEFLATED);
                    zip.setPreserveFileTimestamps(false);

                    zip.from(mergeMagisk);
                    zip.from(generateChecksum, sp -> sp.into("customize.d"));
                    zip.from(generateCustomize);
                }
        );

        project.getTasks().named(
                "assemble" + StringUtils.capitalize(variant.getName()),
                t -> t.dependsOn(zipMagisk)
        );
    }

    enum Loader {
        Riru("riru"), Zygisk("zygisk");

        final String flavorName;

        Loader(final String flavorName) {
            this.flavorName = flavorName;
        }

        @Nullable
        public static Loader fromFlavorName(final String flavorName) {
            return Arrays.stream(Loader.values())
                    .filter(l -> l.flavorName.equals(flavorName))
                    .findFirst().orElse(null);
        }
    }
}
