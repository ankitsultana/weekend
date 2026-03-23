use std::collections::HashMap;
use std::io;
use std::path::{Path, PathBuf};
use std::sync::Mutex;
use std::time::SystemTime;

use tokio::fs;
use tokio::io::AsyncReadExt;

/// Filesystem-backed storage with in-memory ETag cache (mtime → weak consistency with disk).
pub struct Storage {
    root: PathBuf,
    etag_cache: Mutex<HashMap<PathBuf, (SystemTime, String)>>,
}

impl Storage {
    pub fn new(root: PathBuf) -> Self {
        Self {
            root,
            etag_cache: Mutex::new(HashMap::new()),
        }
    }

    pub fn object_path(&self, bucket: &str, key: &str) -> PathBuf {
        self.root.join(bucket).join(key)
    }

    pub async fn read_object(&self, bucket: &str, key: &str) -> io::Result<Option<ObjectData>> {
        let path = self.object_path(bucket, key);
        let meta = match fs::metadata(&path).await {
            Ok(m) => m,
            Err(e) if e.kind() == io::ErrorKind::NotFound => return Ok(None),
            Err(e) => return Err(e),
        };
        if !meta.is_file() {
            return Ok(None);
        }
        let mut file = fs::File::open(&path).await?;
        let mut buf = Vec::new();
        file.read_to_end(&mut buf).await?;
        let modified = meta.modified()?;
        let etag = self.etag_for(&path, modified, &buf)?;
        let content_type = mime_guess::from_path(&path)
            .first_raw()
            .unwrap_or("application/octet-stream")
            .to_string();
        Ok(Some(ObjectData {
            bytes: buf,
            etag,
            last_modified: modified,
            content_type,
        }))
    }

    pub async fn object_meta(&self, bucket: &str, key: &str) -> io::Result<Option<ObjectMeta>> {
        let path = self.object_path(bucket, key);
        let meta = match fs::metadata(&path).await {
            Ok(m) => m,
            Err(e) if e.kind() == io::ErrorKind::NotFound => return Ok(None),
            Err(e) => return Err(e),
        };
        if !meta.is_file() {
            return Ok(None);
        }
        let len = meta.len();
        let mut file = fs::File::open(&path).await?;
        let mut buf = Vec::new();
        file.read_to_end(&mut buf).await?;
        let modified = meta.modified()?;
        let etag = self.etag_for(&path, modified, &buf)?;
        let content_type = mime_guess::from_path(&path)
            .first_raw()
            .unwrap_or("application/octet-stream")
            .to_string();
        Ok(Some(ObjectMeta {
            size: len,
            etag,
            last_modified: modified,
            content_type,
        }))
    }

    fn etag_for(&self, path: &Path, mtime: SystemTime, content: &[u8]) -> io::Result<String> {
        let mut cache = self.etag_cache.lock().map_err(|e| {
            io::Error::new(io::ErrorKind::Other, format!("etag cache poisoned: {e}"))
        })?;
        if let Some((cached_mtime, etag)) = cache.get(path) {
            if *cached_mtime == mtime {
                return Ok(etag.clone());
            }
        }
        let digest = md5::compute(content);
        let etag = format!("\"{}\"", hex::encode(digest.0));
        cache.insert(path.to_path_buf(), (mtime, etag.clone()));
        Ok(etag)
    }

    /// Returns object keys for `bucket` with optional `prefix`, using `/` as path separator.
    pub fn list_keys(&self, bucket: &str, prefix: Option<&str>) -> io::Result<Vec<String>> {
        let bucket_root = self.root.join(bucket);
        if !bucket_root.exists() {
            return Ok(Vec::new());
        }
        let prefix = prefix.unwrap_or("");
        let mut keys = Vec::new();
        Self::walk_files(&bucket_root, &bucket_root, &mut keys)?;
        keys.retain(|k| k.starts_with(prefix));
        keys.sort();
        Ok(keys)
    }

    fn walk_files(dir: &Path, bucket_root: &Path, keys: &mut Vec<String>) -> io::Result<()> {
        for entry in std::fs::read_dir(dir)? {
            let entry = entry?;
            let path = entry.path();
            let meta = entry.metadata()?;
            if meta.is_dir() {
                Self::walk_files(&path, bucket_root, keys)?;
            } else if meta.is_file() {
                let rel = path
                    .strip_prefix(bucket_root)
                    .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e.to_string()))?;
                let key = rel.to_string_lossy().replace('\\', "/");
                keys.push(key);
            }
        }
        Ok(())
    }

    pub fn meta_for_key_sync(&self, bucket: &str, key: &str) -> io::Result<Option<ListEntry>> {
        let path = self.object_path(bucket, key);
        let meta = match std::fs::metadata(&path) {
            Ok(m) => m,
            Err(e) if e.kind() == io::ErrorKind::NotFound => return Ok(None),
            Err(e) => return Err(e),
        };
        if !meta.is_file() {
            return Ok(None);
        }
        let mut file = std::fs::File::open(&path)?;
        let mut buf = Vec::new();
        std::io::Read::read_to_end(&mut file, &mut buf)?;
        let modified = meta.modified()?;
        let etag = self.etag_for(&path, modified, &buf)?;
        Ok(Some(ListEntry {
            key: key.to_string(),
            size: meta.len(),
            etag,
            last_modified: modified,
        }))
    }
}

pub struct ObjectData {
    pub bytes: Vec<u8>,
    pub etag: String,
    pub last_modified: SystemTime,
    pub content_type: String,
}

pub struct ObjectMeta {
    pub size: u64,
    pub etag: String,
    pub last_modified: SystemTime,
    pub content_type: String,
}

pub struct ListEntry {
    pub key: String,
    pub size: u64,
    pub etag: String,
    pub last_modified: SystemTime,
}
