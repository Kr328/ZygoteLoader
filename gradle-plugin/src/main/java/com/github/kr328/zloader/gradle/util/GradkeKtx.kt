package com.github.kr328.zloader.gradle.util

import com.android.build.api.dsl.CommonExtension
import com.android.build.api.variant.AndroidComponentsExtension
import com.android.build.api.variant.Variant
import com.android.build.api.variant.VariantBuilder
import org.gradle.api.NamedDomainObjectContainer
import org.gradle.api.Project
import org.gradle.api.Task
import org.gradle.api.file.CopySpec
import org.gradle.api.plugins.ExtensionContainer
import org.gradle.api.provider.Provider
import org.gradle.api.tasks.AbstractCopyTask
import org.gradle.api.tasks.TaskContainer
import kotlin.reflect.KClass

fun <T : Task> TaskContainer.registerKtx(
    name: String,
    clazz: KClass<T>,
    configure: T.() -> Unit
): Provider<T> {
    return register(name, clazz.java, configure)
}

fun AbstractCopyTask.fromKtx(vararg objs: Any, block: CopySpec.() -> Unit) {
    from(objs, block)
}

fun AbstractCopyTask.fromKtx(vararg objs: Any) {
    from(objs)
}

fun Project.syncKtx(block: CopySpec.() -> Unit) {
    sync(block)
}

fun CopySpec.fromKtx(vararg objs: Any, block: CopySpec.() -> Unit) {
    from(objs, block)
}

fun <T : Any> ExtensionContainer.configureKtx(clazz: KClass<T>, configure: T.() -> Unit) {
    configure(clazz.java) {
        it.configure()
    }
}

fun <DslExtensionT : CommonExtension<*, *, *, *>, VariantBuilderT : VariantBuilder, VariantT : Variant>
        AndroidComponentsExtension<DslExtensionT, VariantBuilderT, VariantT>.finalizeDslKtx(block: DslExtensionT.() -> Unit) {
    finalizeDsl {
        it.block()
    }
}

fun <DslExtensionT : CommonExtension<*, *, *, *>, VariantBuilderT : VariantBuilder, VariantT : Variant>
        AndroidComponentsExtension<DslExtensionT, VariantBuilderT, VariantT>.beforeVariantsKtx(block: VariantBuilderT.() -> Unit) {
    beforeVariants {
        it.block()
    }
}

fun <DslExtensionT : CommonExtension<*, *, *, *>, VariantBuilderT : VariantBuilder, VariantT : Variant>
        AndroidComponentsExtension<DslExtensionT, VariantBuilderT, VariantT>.onVariantsKtx(block: VariantT.() -> Unit) {
    onVariants {
        it.block()
    }
}

fun <T> NamedDomainObjectContainer<T>.createKtx(name: String, block: T.() -> Unit) {
    create(name) {
        it.block()
    }
}
