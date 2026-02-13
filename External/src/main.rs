use std::fs;
use std::path::PathBuf;

use clap::Parser;

#[derive(Debug, Parser)]
#[command(
    name = "canalcv-cli",
    version,
    about = "Minimal CanalCV external CLI scaffold"
)]
struct Cli {
    #[arg(long, default_value = "127.0.0.1")]
    host: String,

    #[arg(long, default_value_t = 9000)]
    port: u16,

    #[arg(long, default_value = "./output")]
    output: PathBuf,
}

fn main() -> Result<(), String> {
    let cli = Cli::parse();

    fs::create_dir_all(&cli.output).map_err(|err| {
        format!(
            "failed to create output directory '{}': {err}",
            cli.output.display()
        )
    })?;

    println!("canalcv-cli configuration");
    println!("host={}", cli.host);
    println!("port={}", cli.port);
    println!("output={}", cli.output.display());

    Ok(())
}
