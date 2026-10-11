use std::ffi::{CStr, CString};
use std::fmt;
use std::ops::{Add, Sub, Mul, Div, Neg};
use std::cmp::Ordering;

#[repr(C, align(16))]
#[derive(Clone, Copy)]
pub struct RawDecimal {
    pub sig_lo: u64,
    pub sig_hi: u64,
    pub exp: i16,
    pub sign: i8,
    pub tag: u8,
    pub pad: [u8; 12],
}

impl Default for RawDecimal {
    fn default() -> Self {
        Self {
            sig_lo: 0,
            sig_hi: 0,
            exp: 0,
            sign: 0,
            tag: 0,
            pad: [0; 12],
        }
    }
}

extern "C" {
    fn fd_from_i64(v: i64, out: *mut RawDecimal);
    fn fd_from_u64(v: u64, out: *mut RawDecimal);
    fn fd_from_double(v: f64, out: *mut RawDecimal);
    fn fd_from_str(s: *const libc::c_char, len: libc::size_t, out: *mut RawDecimal);
    fn fd_nan(out: *mut RawDecimal);
    fn fd_inf(negative: libc::c_int, out: *mut RawDecimal);

    fn fd_add(a: *const RawDecimal, b: *const RawDecimal, out: *mut RawDecimal);
    fn fd_sub(a: *const RawDecimal, b: *const RawDecimal, out: *mut RawDecimal);
    fn fd_mul(a: *const RawDecimal, b: *const RawDecimal, out: *mut RawDecimal);
    fn fd_div(a: *const RawDecimal, b: *const RawDecimal, out: *mut RawDecimal);
    fn fd_neg(a: *const RawDecimal, out: *mut RawDecimal);
    fn fd_abs(a: *const RawDecimal, out: *mut RawDecimal);
    fn fd_sqrt(a: *const RawDecimal, out: *mut RawDecimal);
    fn fd_pow_i64(a: *const RawDecimal, n: i64, out: *mut RawDecimal);

    fn fd_round(a: *const RawDecimal, places: libc::c_int, out: *mut RawDecimal);
    fn fd_trunc(a: *const RawDecimal, out: *mut RawDecimal);
    fn fd_floor(a: *const RawDecimal, out: *mut RawDecimal);
    fn fd_ceil(a: *const RawDecimal, out: *mut RawDecimal);

    fn fd_cmp(a: *const RawDecimal, b: *const RawDecimal) -> libc::c_int;
    fn fd_eq(a: *const RawDecimal, b: *const RawDecimal) -> libc::c_int;

    fn fd_to_i64(a: *const RawDecimal) -> i64;
    fn fd_to_double(a: *const RawDecimal) -> f64;
    fn fd_to_str(a: *const RawDecimal, buf: *mut libc::c_char, buflen: libc::size_t) -> libc::c_int;

    fn fd_is_nan(a: *const RawDecimal) -> libc::c_int;
    fn fd_is_inf(a: *const RawDecimal) -> libc::c_int;
    fn fd_is_zero(a: *const RawDecimal) -> libc::c_int;
    fn fd_is_neg(a: *const RawDecimal) -> libc::c_int;
    fn fd_is_finite(a: *const RawDecimal) -> libc::c_int;
}

mod libc {
    pub type c_char = std::os::raw::c_char;
    pub type c_int = std::os::raw::c_int;
    pub type size_t = usize;
}

#[derive(Clone, Copy)]
pub struct Decimal(pub RawDecimal);

impl Decimal {
    pub fn from_i64(v: i64) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_from_i64(v, &mut out) };
        Decimal(out)
    }

    pub fn from_u64(v: u64) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_from_u64(v, &mut out) };
        Decimal(out)
    }

    pub fn from_f64(v: f64) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_from_double(v, &mut out) };
        Decimal(out)
    }

    pub fn nan() -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_nan(&mut out) };
        Decimal(out)
    }

    pub fn inf(negative: bool) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_inf(if negative { 1 } else { 0 }, &mut out) };
        Decimal(out)
    }

    pub fn parse(s: &str) -> Self {
        let c_str = CString::new(s).unwrap();
        let mut out = RawDecimal::default();
        unsafe { fd_from_str(c_str.as_ptr(), s.len(), &mut out) };
        Decimal(out)
    }

    pub fn abs(self) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_abs(&self.0, &mut out) };
        Decimal(out)
    }

    pub fn sqrt(self) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_sqrt(&self.0, &mut out) };
        Decimal(out)
    }

    pub fn pow(self, n: i64) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_pow_i64(&self.0, n, &mut out) };
        Decimal(out)
    }

    pub fn round(self, places: i32) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_round(&self.0, places, &mut out) };
        Decimal(out)
    }

    pub fn trunc(self) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_trunc(&self.0, &mut out) };
        Decimal(out)
    }

    pub fn floor(self) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_floor(&self.0, &mut out) };
        Decimal(out)
    }

    pub fn ceil(self) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_ceil(&self.0, &mut out) };
        Decimal(out)
    }

    pub fn to_i64(self) -> i64 { unsafe { fd_to_i64(&self.0) } }
    pub fn to_f64(self) -> f64 { unsafe { fd_to_double(&self.0) } }

    pub fn is_nan(self) -> bool { unsafe { fd_is_nan(&self.0) != 0 } }
    pub fn is_inf(self) -> bool { unsafe { fd_is_inf(&self.0) != 0 } }
    pub fn is_zero(self) -> bool { unsafe { fd_is_zero(&self.0) != 0 } }
    pub fn is_neg(self) -> bool { unsafe { fd_is_neg(&self.0) != 0 } }
    pub fn is_finite(self) -> bool { unsafe { fd_is_finite(&self.0) != 0 } }
}

impl Add for Decimal {
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_add(&self.0, &rhs.0, &mut out) };
        Decimal(out)
    }
}

impl Sub for Decimal {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_sub(&self.0, &rhs.0, &mut out) };
        Decimal(out)
    }
}

impl Mul for Decimal {
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_mul(&self.0, &rhs.0, &mut out) };
        Decimal(out)
    }
}

impl Div for Decimal {
    type Output = Self;
    fn div(self, rhs: Self) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_div(&self.0, &rhs.0, &mut out) };
        Decimal(out)
    }
}

impl Neg for Decimal {
    type Output = Self;
    fn neg(self) -> Self {
        let mut out = RawDecimal::default();
        unsafe { fd_neg(&self.0, &mut out) };
        Decimal(out)
    }
}

impl PartialEq for Decimal {
    fn eq(&self, other: &Self) -> bool { unsafe { fd_eq(&self.0, &other.0) != 0 } }
}

impl PartialOrd for Decimal {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        let res = unsafe { fd_cmp(&self.0, &other.0) };
        if self.is_nan() || other.is_nan() { None }
        else if res < 0 { Some(Ordering::Less) }
        else if res > 0 { Some(Ordering::Greater) }
        else { Some(Ordering::Equal) }
    }
}

impl fmt::Display for Decimal {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut buf = [0 as std::os::raw::c_char; 64];
        let len = unsafe { fd_to_str(&self.0, buf.as_mut_ptr(), buf.len()) };
        if len > 0 {
            let cstr = unsafe { CStr::from_ptr(buf.as_ptr()) };
            write!(f, "{}", cstr.to_str().unwrap_or("NaN"))
        } else {
            write!(f, "0")
        }
    }
}
