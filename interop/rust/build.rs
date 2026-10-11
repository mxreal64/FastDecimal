use std::env;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());

    let candidates = [
        env::var("FASTDECIMAL_LIB_DIR").map(PathBuf::from).ok(),
        Some(manifest_dir.join("../../build")),
        Some(manifest_dir.join("../build")),
        Some(manifest_dir.join("build")),
        Some(PathBuf::from("/usr/local/lib")),
        Some(PathBuf::from("/usr/lib")),
    ];

    let mut found_dir = None;
    for candidate in candidates.into_iter().flatten() {
        if candidate.join("libfastdecimal.so").exists()
            || candidate.join("libfastdecimal.dylib").exists()
            || candidate.join("fastdecimal.dll").exists()
        {
            found_dir = Some(candidate.canonicalize().unwrap_or(candidate));
            break;
        }
    }

    let lib_dir = found_dir.unwrap_or_else(|| manifest_dir.join("../../build"));

    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=dylib=fastdecimal");

    // rpath for Linux & macOS dyld
    println!("cargo:rustc-link-arg=-Wl,-rpath,{}", lib_dir.display());
    // Also add loader_path for macOS dyld
    println!("cargo:rustc-link-arg=-Wl,-rpath,@loader_path/../../build");
    println!("cargo:rustc-link-arg=-Wl,-rpath,@executable_path/../../build");
}
