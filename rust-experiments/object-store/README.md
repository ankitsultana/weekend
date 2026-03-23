# Mock S3 Object Store

A minimal S3-compatible object store backed by the local filesystem. Implements path-style S3 APIs over HTTP.

## Running

```bash
cargo run
```

By default, the server listens on port `9000` and stores objects under `./data/`. Both can be overridden:

```bash
PORT=8080 S3_MOCK_DATA_DIR=/tmp/s3-data cargo run
```

Objects are stored at `<data_dir>/<bucket>/<key>` on disk. To seed data, just create files there:

```bash
mkdir -p data/my-bucket/some/prefix
echo "hello" > data/my-bucket/some/prefix/file.txt
```

## API

All endpoints use path-style URLs: `http://localhost:9000/<bucket>/<key>`

### List Objects V2

```
GET /<bucket>?list-type=2
```

Query parameters:
- `prefix` — filter keys by prefix
- `delimiter` — group keys into virtual directories (returns `CommonPrefixes`)
- `max-keys` — page size (default 1000, max 1000)
- `continuation-token` — offset for pagination (returned as `NextContinuationToken` when truncated)

### Get Object

```
GET /<bucket>/<key>
```

### Head Object

```
HEAD /<bucket>/<key>
```

## Example curl Commands

```bash
# List all objects in a bucket
curl "http://localhost:9000/bkt?list-type=2"

# List with a prefix filter
curl "http://localhost:9000/bkt?list-type=2&prefix=prefix/d1/"

# List with delimiter (virtual directory listing)
curl "http://localhost:9000/bkt?list-type=2&prefix=prefix/&delimiter=/"

# Get an object
curl "http://localhost:9000/bkt/prefix/d1/f1"

# Head an object (headers only)
curl -I "http://localhost:9000/bkt/prefix/d1/f1"

# Missing key returns 404 with S3-style XML error
curl "http://localhost:9000/bkt/does/not/exist"
```

## Response Format

List responses are S3-compatible XML (`ListBucketResult`). Object responses include standard S3 headers: `ETag`, `Last-Modified`, `Content-Type`, `Content-Length`, and `Accept-Ranges`.

Errors are returned as S3-style XML:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Error>
  <Code>NoSuchKey</Code>
  <Message>The specified key does not exist.</Message>
  <Resource>/bkt/does/not/exist</Resource>
</Error>
```
