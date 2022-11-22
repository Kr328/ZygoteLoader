package com.github.kr328.gradle.zygote.tasks;

import org.gradle.api.DefaultTask;
import org.gradle.api.file.DirectoryProperty;
import org.gradle.api.provider.SetProperty;
import org.gradle.api.tasks.Input;
import org.gradle.api.tasks.OutputDirectory;
import org.gradle.api.tasks.TaskAction;

import java.io.File;
import java.nio.file.Files;

public abstract class PackagesTask extends DefaultTask {
    @Input
    public abstract SetProperty<String> getPackages();

    @OutputDirectory
    public abstract DirectoryProperty getDestinationDirectory();

    @TaskAction
    public void doAction() throws Exception {
        for (final File file : getDestinationDirectory().getAsFileTree()) {
            Files.delete(file.toPath());
        }

        for (final String pkg : getPackages().get()) {
            Files.createFile(getDestinationDirectory().file(pkg).get().getAsFile().toPath());
        }
    }
}
