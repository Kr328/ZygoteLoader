package com.github.kr328.gradle.zygote;

import org.gradle.api.Action;

import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

public class ZygoteLoaderExtension {
    private final Properties zygisk = new Properties();
    private final Properties riru = new Properties();
    private final Set<String> packages = new HashSet<>();

    @Nonnull
    public Properties getZygisk() {
        return zygisk;
    }

    @Nonnull
    public Properties getRiru() {
        return riru;
    }

    @Nonnull
    public Set<String> getPackages() {
        return packages;
    }

    public void zygisk(@Nonnull final Action<Properties> action) {
        action.execute(zygisk);
    }

    public void riru(@Nonnull final Action<Properties> action) {
        action.execute(riru);
    }

    public void all(@Nonnull final Action<Properties> action) {
        action.execute(zygisk);
        action.execute(riru);
    }

    public void packages(@Nonnull final String... pkgs) {
        packages.addAll(List.of(pkgs));
    }

    public static class Properties extends LinkedHashMap<String, String> {
        @Override
        public String put(final String key, final String value) {
            if (value == null) {
                remove(key);
            }

            return super.put(key, value);
        }

        @Nullable
        public String getId() {
            return get("id");
        }

        public void setId(@Nullable final String id) {
            put("id", id);
        }

        @Nullable
        public String getName() {
            return get("name");
        }

        public void setName(@Nullable final String name) {
            put("name", name);
        }

        @Nullable
        public String getAuthor() {
            return get("author");
        }

        public void setAuthor(@Nullable final String author) {
            put("author", author);
        }

        @Nullable
        public String getDescription() {
            return get("description");
        }

        public void setDescription(@Nullable final String description) {
            put("description", description);
        }

        @Nullable
        public String getEntrypoint() {
            return get("entrypoint");
        }

        public void setEntrypoint(@Nullable final String entrypoint) {
            put("entrypoint", entrypoint);
        }

        @Nullable
        public String getArchiveName() {
            return get("archiveName");
        }

        public void setArchiveName(@Nullable final String archiveName) {
            put("archiveName", archiveName);
        }

        @Nullable
        public String getUpdateJson() {
            return get("updateJson");
        }

        public void setUpdateJson(@Nullable final String updateJson) {
            put("updateJson", updateJson);
        }

        public boolean isUseBinderInterceptors() {
            return Boolean.parseBoolean(get("useBinderInterceptors"));
        }

        public void setUseBinderInterceptors(final boolean enabled) {
            put("useBinderInterceptors", Boolean.toString(enabled));
        }
    }
}
