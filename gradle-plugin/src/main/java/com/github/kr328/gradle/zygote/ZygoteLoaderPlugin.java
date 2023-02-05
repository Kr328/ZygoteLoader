package com.github.kr328.gradle.zygote;

import com.android.build.api.dsl.ProductFlavor;
import com.android.build.api.variant.ApplicationAndroidComponentsExtension;

import org.gradle.api.GradleException;
import org.gradle.api.Plugin;
import org.gradle.api.Project;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;

import javax.annotation.Nonnull;

import kotlin.Unit;

@SuppressWarnings("UnstableApiUsage")
public class ZygoteLoaderPlugin implements Plugin<Project> {
    public static final String LOADER_FLAVOR_DIMENSION = "loader";

    @Override
    public void apply(@Nonnull final Project target) {
        if (!target.getPlugins().hasPlugin("com.android.application")) {
            throw new GradleException("com.android.application not applied");
        }

        final ZygoteLoaderExtension extension = target.getExtensions()
                .create("zygote", ZygoteLoaderExtension.class);

        target.getDependencies().add("implementation", BuildConfig.RUNTIME_DEPENDENCY);

        target.getExtensions().configure(ApplicationAndroidComponentsExtension.class, components -> {
            components.finalizeDsl(dsl -> {
                dsl.getFlavorDimensions().add(LOADER_FLAVOR_DIMENSION);

                final MethodHandle flavorSetDimension;
                try {
                    flavorSetDimension = MethodHandles.lookup().findVirtual(
                            ProductFlavor.class,
                            "setDimension",
                            MethodType.methodType(void.class, String.class)
                    );
                } catch (final Exception e) {
                    throw new RuntimeException(e);
                }

                dsl.productFlavors(flavors -> {
                    flavors.create(ZygoteLoaderDecorator.Loader.Zygisk.flavorName, flavor -> {
                        try {
                            flavorSetDimension.invoke(flavor, LOADER_FLAVOR_DIMENSION);
                        } catch (final Throwable e) {
                            throw new RuntimeException(e);
                        }

                        flavor.setMultiDexEnabled(false);
                    });

                    flavors.create(ZygoteLoaderDecorator.Loader.Riru.flavorName, flavor -> {
                        try {
                            flavorSetDimension.invoke(flavor, LOADER_FLAVOR_DIMENSION);
                        } catch (final Throwable e) {
                            throw new RuntimeException(e);
                        }

                        flavor.setMultiDexEnabled(false);
                    });

                    return Unit.INSTANCE;
                });
            });

            final ZygoteLoaderDecorator decorator = new ZygoteLoaderDecorator(target, extension);
            components.onVariants(components.selector().all(), variant -> {
                target.afterEvaluate(prj -> decorator.decorateVariant(
                        ZygoteLoaderDecorator.Loader.fromFlavorName(variant.getFlavorName()),
                        variant
                ));
            });
        });
    }
}
