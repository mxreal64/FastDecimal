using System;
using System.Runtime.InteropServices;
using System.Text;

namespace FastDecimal
{
    [StructLayout(LayoutKind.Sequential, Pack = 16)]
    public unsafe struct RawDecimal
    {
        public ulong SigLo;
        public ulong SigHi;
        public short Exp;
        public sbyte Sign;
        public byte Tag;
        public fixed byte Pad[12];
    }

    public readonly unsafe struct Decimal : IEquatable<Decimal>, IComparable<Decimal>
    {
    static Decimal()
    {
        NativeLibrary.SetDllImportResolver(typeof(Decimal).Assembly, (name, assembly, path) =>
        {
            if (name == LibName)
            {
                string[] candidates = {
                    "fastdecimal",
                    "libfastdecimal.so",
                    "libfastdecimal.dylib",
                    "build/libfastdecimal.so",
                    "build/libfastdecimal.dylib",
                    "../build/libfastdecimal.so",
                    "../build/libfastdecimal.dylib",
                    "../../build/libfastdecimal.so",
                    "../../build/libfastdecimal.dylib"
                };
                foreach (var c in candidates)
                {
                    if (NativeLibrary.TryLoad(c, out IntPtr handle))
                        return handle;
                }
            }
            return IntPtr.Zero;
        });
    }

        private const string LibName = "fastdecimal";

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_from_i64(long v, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_from_u64(ulong v, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_from_double(double v, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_from_str(byte* s, nuint len, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_nan(out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_inf(int negative, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_add(in RawDecimal a, in RawDecimal b, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_sub(in RawDecimal a, in RawDecimal b, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_mul(in RawDecimal a, in RawDecimal b, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_div(in RawDecimal a, in RawDecimal b, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_neg(in RawDecimal a, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_abs(in RawDecimal a, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_sqrt(in RawDecimal a, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_pow_i64(in RawDecimal a, long n, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_round(in RawDecimal a, int places, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_trunc(in RawDecimal a, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_floor(in RawDecimal a, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void fd_ceil(in RawDecimal a, out RawDecimal result);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int fd_cmp(in RawDecimal a, in RawDecimal b);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int fd_eq(in RawDecimal a, in RawDecimal b);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern long fd_to_i64(in RawDecimal a);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern double fd_to_double(in RawDecimal a);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int fd_to_str(in RawDecimal a, byte* buf, nuint buflen);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int fd_is_nan(in RawDecimal a);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int fd_is_inf(in RawDecimal a);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int fd_is_zero(in RawDecimal a);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int fd_is_neg(in RawDecimal a);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int fd_is_finite(in RawDecimal a);

        public readonly RawDecimal Raw;

        public Decimal(in RawDecimal raw) => Raw = raw;

        public Decimal(long v)
        {
            fd_from_i64(v, out Raw);
        }

        public Decimal(ulong v)
        {
            fd_from_u64(v, out Raw);
        }

        public Decimal(double v)
        {
            fd_from_double(v, out Raw);
        }

        public static Decimal Parse(string s)
        {
            if (s == null) throw new ArgumentNullException(nameof(s));
            byte[] bytes = Encoding.UTF8.GetBytes(s);
            fixed (byte* p = bytes)
            {
                fd_from_str(p, (nuint)bytes.Length, out RawDecimal raw);
                return new Decimal(in raw);
            }
        }

        public static Decimal NaN
        {
            get
            {
                fd_nan(out RawDecimal raw);
                return new Decimal(in raw);
            }
        }

        public static Decimal PositiveInfinity
        {
            get
            {
                fd_inf(0, out RawDecimal raw);
                return new Decimal(in raw);
            }
        }

        public static Decimal NegativeInfinity
        {
            get
            {
                fd_inf(1, out RawDecimal raw);
                return new Decimal(in raw);
            }
        }

        public bool IsNaN => fd_is_nan(in Raw) != 0;
        public bool IsInfinity => fd_is_inf(in Raw) != 0;
        public bool IsZero => fd_is_zero(in Raw) != 0;
        public bool IsNegative => fd_is_neg(in Raw) != 0;
        public bool IsFinite => fd_is_finite(in Raw) != 0;

        public static Decimal operator +(in Decimal a, in Decimal b)
        {
            fd_add(in a.Raw, in b.Raw, out RawDecimal res);
            return new Decimal(in res);
        }

        public static Decimal operator -(in Decimal a, in Decimal b)
        {
            fd_sub(in a.Raw, in b.Raw, out RawDecimal res);
            return new Decimal(in res);
        }

        public static Decimal operator *(in Decimal a, in Decimal b)
        {
            fd_mul(in a.Raw, in b.Raw, out RawDecimal res);
            return new Decimal(in res);
        }

        public static Decimal operator /(in Decimal a, in Decimal b)
        {
            fd_div(in a.Raw, in b.Raw, out RawDecimal res);
            return new Decimal(in res);
        }

        public static Decimal operator -(in Decimal a)
        {
            fd_neg(in a.Raw, out RawDecimal res);
            return new Decimal(in res);
        }

        public Decimal Abs()
        {
            fd_abs(in Raw, out RawDecimal res);
            return new Decimal(in res);
        }

        public Decimal Sqrt()
        {
            fd_sqrt(in Raw, out RawDecimal res);
            return new Decimal(in res);
        }

        public Decimal Pow(long n)
        {
            fd_pow_i64(in Raw, n, out RawDecimal res);
            return new Decimal(in res);
        }

        public Decimal Round(int places)
        {
            fd_round(in Raw, places, out RawDecimal res);
            return new Decimal(in res);
        }

        public Decimal Trunc()
        {
            fd_trunc(in Raw, out RawDecimal res);
            return new Decimal(in res);
        }

        public Decimal Floor()
        {
            fd_floor(in Raw, out RawDecimal res);
            return new Decimal(in res);
        }

        public Decimal Ceil()
        {
            fd_ceil(in Raw, out RawDecimal res);
            return new Decimal(in res);
        }

        public long ToInt64() => fd_to_i64(in Raw);
        public double ToDouble() => fd_to_double(in Raw);

        public bool Equals(Decimal other) => fd_eq(in Raw, in other.Raw) != 0;
        public override bool Equals(object obj) => obj is Decimal d && Equals(d);
        public override int GetHashCode() => HashCode.Combine(Raw.SigLo, Raw.SigHi, Raw.Exp, Raw.Sign);

        public int CompareTo(Decimal other) => fd_cmp(in Raw, in other.Raw);

        public static bool operator ==(in Decimal a, in Decimal b) => a.Equals(b);
        public static bool operator !=(in Decimal a, in Decimal b) => !a.Equals(b);
        public static bool operator <(in Decimal a, in Decimal b) => a.CompareTo(b) < 0;
        public static bool operator >(in Decimal a, in Decimal b) => a.CompareTo(b) > 0;
        public static bool operator <=(in Decimal a, in Decimal b) => a.CompareTo(b) <= 0;
        public static bool operator >=(in Decimal a, in Decimal b) => a.CompareTo(b) >= 0;

        public override string ToString()
        {
            byte* buf = stackalloc byte[64];
            int len = fd_to_str(in Raw, buf, 64);
            return len > 0 ? Encoding.UTF8.GetString(buf, len) : "0";
        }
    }
}
