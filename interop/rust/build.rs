fn main() {
    println!("cargo:rustc-link-search=native=/home/user1/Public/decimallib/build");
    println!("cargo:rustc-link-lib=dylib=fastdecimal");
    println!("cargo:rustc-link-arg=-Wl,-rpath,/home/user1/Public/decimallib/build");
}
