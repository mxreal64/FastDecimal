package com.fastdecimal;

import java.lang.foreign.*;
import java.lang.invoke.MethodHandle;
import java.nio.file.Path;

/**
 * Java Interop for FastDecimal using Project Panama (Foreign Function & Memory API).
 * Zero JNI overhead.
 */
public final class FastDecimal implements Comparable<FastDecimal> {

    public static final GroupLayout LAYOUT = MemoryLayout.structLayout(
        ValueLayout.JAVA_LONG.withName("sig_lo"),
        ValueLayout.JAVA_LONG.withName("sig_hi"),
        ValueLayout.JAVA_SHORT.withName("exp"),
        ValueLayout.JAVA_BYTE.withName("sign"),
        ValueLayout.JAVA_BYTE.withName("tag"),
        MemoryLayout.paddingLayout(12)
    ).withName("fd_dec_t");

    private static final Linker LINKER = Linker.nativeLinker();
    private static final SymbolLookup LOOKUP;

    private static final MethodHandle MH_FROM_I64;
    private static final MethodHandle MH_FROM_U64;
    private static final MethodHandle MH_FROM_DOUBLE;
    private static final MethodHandle MH_FROM_STR;
    private static final MethodHandle MH_NAN;
    private static final MethodHandle MH_INF;

    private static final MethodHandle MH_ADD;
    private static final MethodHandle MH_SUB;
    private static final MethodHandle MH_MUL;
    private static final MethodHandle MH_DIV;
    private static final MethodHandle MH_NEG;
    private static final MethodHandle MH_ABS;
    private static final MethodHandle MH_SQRT;
    private static final MethodHandle MH_POW_I64;

    private static final MethodHandle MH_ROUND;
    private static final MethodHandle MH_TRUNC;
    private static final MethodHandle MH_FLOOR;
    private static final MethodHandle MH_CEIL;

    private static final MethodHandle MH_CMP;
    private static final MethodHandle MH_EQ;

    private static final MethodHandle MH_TO_I64;
    private static final MethodHandle MH_TO_DOUBLE;
    private static final MethodHandle MH_TO_STR;

    private static final MethodHandle MH_IS_NAN;
    private static final MethodHandle MH_IS_INF;
    private static final MethodHandle MH_IS_ZERO;
    private static final MethodHandle MH_IS_NEG;
    private static final MethodHandle MH_IS_FINITE;

    private static SymbolLookup findLibrary() {
        String prop = System.getProperty("fastdecimal.lib.path");
        if (prop != null && java.nio.file.Files.exists(Path.of(prop))) {
            return SymbolLookup.libraryLookup(Path.of(prop), Arena.global());
        }

        String os = System.getProperty("os.name", "").toLowerCase();
        String libName = os.contains("mac") || os.contains("darwin") ? "libfastdecimal.dylib"
                       : os.contains("win") ? "fastdecimal.dll"
                       : "libfastdecimal.so";

        Path[] candidates = {
            Path.of(libName),
            Path.of("build", libName),
            Path.of("../build", libName),
            Path.of("../../build", libName),
            Path.of("/usr/local/lib", libName),
            Path.of("/usr/lib", libName)
        };

        for (Path p : candidates) {
            if (java.nio.file.Files.exists(p)) {
                return SymbolLookup.libraryLookup(p.toAbsolutePath(), Arena.global());
            }
        }

        try {
            return SymbolLookup.libraryLookup(System.mapLibraryName("fastdecimal"), Arena.global());
        } catch (Throwable t) {
            throw new UnsatisfiedLinkError("Cannot locate FastDecimal native library: " + libName);
        }
    }

    static {
        LOOKUP = findLibrary();

        MH_FROM_I64 = find("fd_from_i64", FunctionDescriptor.ofVoid(ValueLayout.JAVA_LONG, ValueLayout.ADDRESS));
        MH_FROM_U64 = find("fd_from_u64", FunctionDescriptor.ofVoid(ValueLayout.JAVA_LONG, ValueLayout.ADDRESS));
        MH_FROM_DOUBLE = find("fd_from_double", FunctionDescriptor.ofVoid(ValueLayout.JAVA_DOUBLE, ValueLayout.ADDRESS));
        MH_FROM_STR = find("fd_from_str", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS));
        MH_NAN = find("fd_nan", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
        MH_INF = find("fd_inf", FunctionDescriptor.ofVoid(ValueLayout.JAVA_INT, ValueLayout.ADDRESS));

        MH_ADD = find("fd_add", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        MH_SUB = find("fd_sub", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        MH_MUL = find("fd_mul", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        MH_DIV = find("fd_div", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        MH_NEG = find("fd_neg", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        MH_ABS = find("fd_abs", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        MH_SQRT = find("fd_sqrt", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        MH_POW_I64 = find("fd_pow_i64", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS));

        MH_ROUND = find("fd_round", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.JAVA_INT, ValueLayout.ADDRESS));
        MH_TRUNC = find("fd_trunc", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        MH_FLOOR = find("fd_floor", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        MH_CEIL = find("fd_ceil", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));

        MH_CMP = find("fd_cmp", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        MH_EQ = find("fd_eq", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.ADDRESS));

        MH_TO_I64 = find("fd_to_i64", FunctionDescriptor.of(ValueLayout.JAVA_LONG, ValueLayout.ADDRESS));
        MH_TO_DOUBLE = find("fd_to_double", FunctionDescriptor.of(ValueLayout.JAVA_DOUBLE, ValueLayout.ADDRESS));
        MH_TO_STR = find("fd_to_str", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.JAVA_LONG));

        MH_IS_NAN = find("fd_is_nan", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS));
        MH_IS_INF = find("fd_is_inf", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS));
        MH_IS_ZERO = find("fd_is_zero", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS));
        MH_IS_NEG = find("fd_is_neg", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS));
        MH_IS_FINITE = find("fd_is_finite", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS));
    }

    private static MethodHandle find(String name, FunctionDescriptor desc) {
        MemorySegment sym = LOOKUP.find(name).orElseThrow(() -> new UnsatisfiedLinkError("Symbol not found: " + name));
        return LINKER.downcallHandle(sym, desc);
    }

    final MemorySegment segment;

    private FastDecimal(MemorySegment seg) {
        this.segment = seg;
    }

    public static FastDecimal of(long v) {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_FROM_I64.invokeExact(v, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public static FastDecimal of(double v) {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_FROM_DOUBLE.invokeExact(v, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public static FastDecimal parse(String s) {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment cStr = arena.allocateFrom(s);
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_FROM_STR.invokeExact(cStr, (long) s.length(), seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public static FastDecimal nan() {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_NAN.invokeExact(seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public static FastDecimal inf(boolean negative) {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_INF.invokeExact(negative ? 1 : 0, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal add(FastDecimal o) {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_ADD.invokeExact(this.segment, o.segment, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal subtract(FastDecimal o) {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_SUB.invokeExact(this.segment, o.segment, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal multiply(FastDecimal o) {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_MUL.invokeExact(this.segment, o.segment, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal divide(FastDecimal o) {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_DIV.invokeExact(this.segment, o.segment, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal negate() {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_NEG.invokeExact(this.segment, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal abs() {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_ABS.invokeExact(this.segment, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal sqrt() {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_SQRT.invokeExact(this.segment, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal pow(long n) {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_POW_I64.invokeExact(this.segment, n, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal round(int places) {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_ROUND.invokeExact(this.segment, places, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal trunc() {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_TRUNC.invokeExact(this.segment, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal floor() {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_FLOOR.invokeExact(this.segment, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public FastDecimal ceil() {
        try {
            MemorySegment seg = Arena.ofAuto().allocate(LAYOUT);
            MH_CEIL.invokeExact(this.segment, seg);
            return new FastDecimal(seg);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public long longValue() {
        try {
            return (long) MH_TO_I64.invokeExact(this.segment);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public double doubleValue() {
        try {
            return (double) MH_TO_DOUBLE.invokeExact(this.segment);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public boolean isNaN() {
        try {
            return ((int) MH_IS_NAN.invokeExact(this.segment)) != 0;
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public boolean isInfinite() {
        try {
            return ((int) MH_IS_INF.invokeExact(this.segment)) != 0;
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public boolean isZero() {
        try {
            return ((int) MH_IS_ZERO.invokeExact(this.segment)) != 0;
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public boolean isNegative() {
        try {
            return ((int) MH_IS_NEG.invokeExact(this.segment)) != 0;
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public boolean isFinite() {
        try {
            return ((int) MH_IS_FINITE.invokeExact(this.segment)) != 0;
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    @Override
    public int compareTo(FastDecimal o) {
        try {
            return (int) MH_CMP.invokeExact(this.segment, o.segment);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) return true;
        if (!(obj instanceof FastDecimal o)) return false;
        try {
            return ((int) MH_EQ.invokeExact(this.segment, o.segment)) == 1;
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    @Override
    public String toString() {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment buf = arena.allocate(64);
            int len = (int) MH_TO_STR.invokeExact(this.segment, buf, 64L);
            return len > 0 ? buf.getString(0) : "0";
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }
}
