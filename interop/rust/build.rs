use std::env;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    
    // Check multiple candidate locations where libfastdecimal might reside
    let candidates = [
        // Environment override
        env::var("FASTDECIMAL_LIB_DIR").map(PathBuf::from).ok(),
        // Relative to repo root build directory
        Some(manifest_dir.join("../../build")),
        Some(manifest_dir.join("../build")),
        Some(manifest_dir.join("build")),
        // Standard system library paths
        Some(PathBuf::from("/usr/local/lib")),
        Some(PathBuf::from("/usr/lib")),
    ];

    let mut found = false;
    for candidate in candidates.into_iter().flatten() {
        if candidate.join("libfastdecimal.so").exists() 
            || candidate.join("libfastdecimal.dylib").exists()
            || candidate.join("fastdecimal.dll").exists() 
        {
            let lib_dir = candidate.canonicalize().unwrap_or(candidate);
            println!("cargo:rustc-link-search=native={}", lib_dir.display());
            println!("cargo:rustc-link-lib=dylib=fastdecimal");
            println!("cargo:rustc-link-arg=-Wl,-rpath,{}", lib_dir.display());
            found = true;
            break;
        }
    }

    if !found {
        // Fallback default: ../../build
        let default_dir = manifest_dir.join("../../build");
        println!("cargo:rustc-link-search=native={}", default_dir.display());
        println!("cargo:rustc-link-lib=dylib=fastdecimal");
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", default_dir.display());
    }
}
