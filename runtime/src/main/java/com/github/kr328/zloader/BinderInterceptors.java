package com.github.kr328.zloader;

import android.os.Binder;
import android.os.DeadObjectException;
import android.os.IBinder;
import android.os.IInterface;
import android.os.Parcel;
import android.os.RemoteException;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.github.kr328.zloader.internal.Loader;

import java.io.FileDescriptor;
import java.lang.ref.WeakReference;

/**
 * Utilize to intercept {@link Binder} onTransact methods.
 * <p>
 * NOTE: Should enable useBinderInterceptors properties in gradle plugin.
 */
public final class BinderInterceptors {
    /**
     * Install an interceptor.
     * <p>
     * NOTE: Avoid holding strong reference to {@code src} that may cause memory leak.
     *
     * @param src     a Binder to intercept, eg {@link com.android.server.am.ActivityManagerService}.
     * @param factory factory to create interceptor.
     * @throws IllegalStateException if useBinderInterceptors not enabled.
     */
    public static void install(final Binder src, final InterceptorFactory factory) {
        final IBinder interceptor = factory.create(new BinderProxy(src));
        if (interceptor instanceof Binder) {
            Loader.registerRedirectBinder(src, (Binder) interceptor);
        } else {
            Loader.registerRedirectBinder(src, new Binder() {
                @Override
                protected boolean onTransact(final int code, @NonNull final Parcel data, @Nullable final Parcel reply, final int flags) throws RemoteException {
                    return interceptor.transact(code, data, reply, flags);
                }
            });
        }
    }

    /**
     * Uninstall a installed interceptor.
     *
     * @param src a Binder that installed.
     * @return if the interceptor is registered.
     * @throws IllegalStateException if useBinderInterceptors not enabled.
     */
    public static boolean uninstall(final Binder src) {
        return Loader.unregisterRedirectBinder(src);
    }

    /**
     * Factory to create interceptor.
     */
    public interface InterceptorFactory {
        /**
         * Create a IBinder to receive transact calls.
         *
         * @param next wrapper to call next interceptor.
         * @return a interceptor.
         */
        IBinder create(final IBinder next);
    }

    private static class BinderProxy implements IBinder {
        private final WeakReference<Binder> src;

        private BinderProxy(final Binder src) {
            this.src = new WeakReference<>(src);
        }

        @Nullable
        @Override
        public String getInterfaceDescriptor() throws RemoteException {
            final Binder s = src.get();
            if (s == null) {
                throw new DeadObjectException();
            }

            return s.getInterfaceDescriptor();
        }

        @Override
        public boolean pingBinder() {
            final Binder s = src.get();
            if (s == null) {
                return false;
            }

            return s.pingBinder();
        }

        @Override
        public boolean isBinderAlive() {
            final Binder s = src.get();
            if (s == null) {
                return false;
            }

            return s.isBinderAlive();
        }

        @Nullable
        @Override
        public IInterface queryLocalInterface(@NonNull final String descriptor) {
            final Binder s = src.get();
            if (s == null) {
                return null;
            }

            return s.queryLocalInterface(descriptor);
        }

        @Override
        public void dump(@NonNull final FileDescriptor fd, @Nullable final String[] args) throws RemoteException {
            final Binder s = src.get();
            if (s == null) {
                throw new DeadObjectException();
            }

            s.dump(fd, args);
        }

        @Override
        public void dumpAsync(@NonNull final FileDescriptor fd, @Nullable final String[] args) throws RemoteException {
            final Binder s = src.get();
            if (s == null) {
                throw new DeadObjectException();
            }

            s.dumpAsync(fd, args);
        }

        @Override
        public boolean transact(final int code, @NonNull final Parcel data, @Nullable final Parcel reply, final int flags) throws RemoteException {
            final Binder s = src.get();
            if (s == null) {
                throw new DeadObjectException();
            }

            return Loader.callExecTransact(s, code, data, reply, flags);
        }

        @Override
        public void linkToDeath(@NonNull final DeathRecipient recipient, final int flags) throws RemoteException {
            final Binder s = src.get();
            if (s == null) {
                throw new DeadObjectException();
            }

            s.linkToDeath(recipient, flags);
        }

        @Override
        public boolean unlinkToDeath(@NonNull final DeathRecipient recipient, final int flags) {
            final Binder s = src.get();
            if (s == null) {
                return false;
            }

            return s.unlinkToDeath(recipient, flags);
        }
    }
}
