mod s3;
mod storage;

use std::path::PathBuf;
use std::sync::Arc;

use storage::Storage;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let data_dir: PathBuf = std::env::var("S3_MOCK_DATA_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|_| PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("data"));

    std::fs::create_dir_all(&data_dir)?;

    eprintln!("S3 mock: data directory {}", data_dir.display());

    let port: u16 = std::env::var("PORT")
        .ok()
        .and_then(|s| s.parse().ok())
        .unwrap_or(9000);

    let storage = Arc::new(Storage::new(data_dir));
    let app = s3::router(storage);

    let addr = format!("0.0.0.0:{port}");
    let listener = tokio::net::TcpListener::bind(&addr).await?;
    eprintln!("Listening on http://{addr} (path-style: http://127.0.0.1:{port}/<bucket>/<key>)");

    axum::serve(listener, app).await?;
    Ok(())
}
