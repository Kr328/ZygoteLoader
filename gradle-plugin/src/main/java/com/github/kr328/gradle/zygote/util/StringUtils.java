package com.github.kr328.gradle.zygote.util;

import javax.annotation.Nonnull;

public final class StringUtils {
    public static String capitalize(@Nonnull final String in) {
        if (in.isEmpty()) {
            return "";
        }

        return Character.toUpperCase(in.charAt(0)) + in.substring(1);
    }
}
