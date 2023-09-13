fn main() -> Result<(), Box<dyn std::error::Error>> {
    tonic_build::compile_protos("proto/service.proto")?;
    println!("cargo:rerun-if-changed=proto/service.proto");
    println!("cargo:rerun-if-changed=build.rs");
    Ok(())
}
