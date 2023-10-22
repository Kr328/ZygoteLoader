# Zygote Loader

A library for building Java only Zygisk/Riru modules.

## Getting Start

1. **Add plugin repository to settings.gradle.kts**

```kotlin
pluginManagement { 
    repositories { 
        // ... other repositories 
        maven(url = "https://maven.kr328.app/releases") 
    } 
}
```

2. **Add an android application module and create entrypoint**

```java
class Entrypoint {
    // ZygoteLoader will invoke this method after injected to target process
    public static void main() {
        // ... your code
    }
}
```

3. **Apply `com.github.kr328.gradle.zygote` plugin**

```kotlin
plugins {
    id("com.android.application") // required
    id("com.github.kr328.gradle.zygote") version "3.1" // apply plugin
    // ... other plugins
}
```

4. **Configure your module properties**

```kotlin
zygote {
    // initial inject packages
    packages(ZygoteLoader.PACKAGE_SYSTEM_SERVER) // initial inject to system_server

    // riru related properties
    riru {
        id = "your module id"
        name = "your module name"
        author = "your name"
        description = "your module description"
        entrypoint = "your entrypoint class qualified name" // see also step 2
        archiveName = "generated zip archive name" // optional
        updateJson = "your updateJson property" // optional, see also https://topjohnwu.github.io/Magisk/guides.html#moduleprop
    }

    // zygisk related properties
    zygisk {
        // same with riru
    }
}
```

5. **Build module**

    1. Run gradle task `<module>:assembleRelease`
       
    2. Pick generated zip from `<module>/build/outputs/magsisk`
   

## Examples

- [Riru-ClipboardWhitelist](https://github.com/Kr328/Riru-ClipboardWhitelist)
  
- [Riru-IFWEnhance](https://github.com/Kr328/Riru-IFWEnhance)