use std::collections::HashSet;
use std::sync::Arc;

use axum::Router;
use axum::body::Body;
use axum::extract::{Path, Query, State};
use axum::http::{HeaderValue, StatusCode, header};
use axum::response::Response;
use axum::routing::get;
use chrono::{DateTime, Utc};
use serde::Deserialize;

use crate::storage::{ListEntry, Storage};

const S3_XMLNS: &str = "http://s3.amazonaws.com/doc/2006-03-01/";
const DEFAULT_MAX_KEYS: i32 = 1000;

#[derive(Clone)]
pub struct AppState {
    pub storage: Arc<Storage>,
}

pub fn router(storage: Arc<Storage>) -> Router {
    let state = AppState { storage };
    Router::new()
        .route("/{bucket}", get(list_objects_v2))
        .route("/{bucket}/{*key}", get(get_object).head(head_object))
        .with_state(state)
}

#[derive(Debug, Deserialize)]
pub struct ListObjectsV2Query {
    #[serde(rename = "list-type")]
    list_type: Option<String>,
    prefix: Option<String>,
    delimiter: Option<String>,
    #[serde(rename = "max-keys")]
    max_keys: Option<i32>,
    #[serde(rename = "continuation-token")]
    continuation_token: Option<String>,
}

#[derive(Debug, Deserialize)]
pub struct ObjectKeyPath {
    bucket: String,
    key: String,
}

fn xml_escape(s: &str) -> String {
    s.chars()
        .flat_map(|c| match c {
            '&' => "&amp;".chars().collect::<Vec<_>>(),
            '<' => "&lt;".chars().collect::<Vec<_>>(),
            '>' => "&gt;".chars().collect::<Vec<_>>(),
            '"' => "&quot;".chars().collect::<Vec<_>>(),
            '\'' => "&apos;".chars().collect::<Vec<_>>(),
            _ => vec![c],
        })
        .collect()
}

fn s3_iso8601(st: std::time::SystemTime) -> String {
    let dt: DateTime<Utc> = st.into();
    dt.format("%Y-%m-%dT%H:%M:%S%.3fZ").to_string()
}

fn http_last_modified(st: std::time::SystemTime) -> HeaderValue {
    let dt: DateTime<Utc> = st.into();
    let s = dt.format("%a, %d %b %Y %H:%M:%S GMT").to_string();
    HeaderValue::from_str(&s)
        .unwrap_or_else(|_| HeaderValue::from_static("Thu, 01 Jan 1970 00:00:00 GMT"))
}

fn s3_error_xml(code: &str, message: &str, resource: &str) -> String {
    format!(
        r#"<?xml version="1.0" encoding="UTF-8"?>
<Error>
  <Code>{}</Code>
  <Message>{}</Message>
  <Resource>{}</Resource>
</Error>
"#,
        xml_escape(code),
        xml_escape(message),
        xml_escape(resource)
    )
}

fn xml_response(status: StatusCode, body: String) -> Response {
    let mut res = Response::new(Body::from(body));
    *res.status_mut() = status;
    res.headers_mut().insert(
        header::CONTENT_TYPE,
        HeaderValue::from_static("application/xml"),
    );
    res
}

/// Keys matching `prefix` are split into virtual prefixes (first `delimiter` after `prefix`) or full object keys.
fn partition_with_delimiter(
    all_keys: &[String],
    prefix: &str,
    delimiter: &str,
) -> (HashSet<String>, Vec<String>) {
    let mut common = HashSet::new();
    let mut contents = Vec::new();

    for key in all_keys {
        let suffix = &key[prefix.len()..];
        if let Some(i) = suffix.find(delimiter) {
            let cp = format!("{}{}{}", prefix, &suffix[..i], delimiter);
            common.insert(cp);
        } else {
            contents.push(key.clone());
        }
    }

    (common, contents)
}

async fn list_objects_v2(
    State(state): State<AppState>,
    Path(bucket): Path<String>,
    Query(q): Query<ListObjectsV2Query>,
) -> Response {
    if q.list_type.as_deref() != Some("2") {
        let body = s3_error_xml(
            "InvalidArgument",
            "Unknown or invalid list-type",
            &format!("/{bucket}"),
        );
        return xml_response(StatusCode::BAD_REQUEST, body);
    }

    let max_keys = q.max_keys.unwrap_or(DEFAULT_MAX_KEYS).clamp(1, 1000);
    let prefix = q.prefix.clone().unwrap_or_default();

    let all_keys = match state.storage.list_keys(&bucket, Some(&prefix)) {
        Ok(k) => k,
        Err(e) => {
            let body = s3_error_xml("InternalError", &e.to_string(), &format!("/{bucket}"));
            return xml_response(StatusCode::INTERNAL_SERVER_ERROR, body);
        }
    };

    let start: usize = q
        .continuation_token
        .as_ref()
        .and_then(|t| t.parse().ok())
        .unwrap_or(0);

    let (common_set, content_keys) = match &q.delimiter {
        Some(d) if !d.is_empty() => partition_with_delimiter(&all_keys, &prefix, d),
        _ => (HashSet::new(), all_keys),
    };

    let mut rows: Vec<String> = common_set.iter().cloned().collect();
    rows.extend(content_keys);
    rows.sort();

    let total = rows.len();
    let page: Vec<String> = rows
        .into_iter()
        .skip(start)
        .take(max_keys as usize)
        .collect();
    let is_truncated = start + page.len() < total;

    let mut xml = String::new();
    xml.push_str(r#"<?xml version="1.0" encoding="UTF-8"?>"#);
    xml.push('\n');
    xml.push_str(&format!(r#"<ListBucketResult xmlns="{S3_XMLNS}">"#));
    xml.push('\n');
    xml.push_str(&format!("  <Name>{}</Name>\n", xml_escape(&bucket)));
    xml.push_str(&format!("  <Prefix>{}</Prefix>\n", xml_escape(&prefix)));
    if let Some(ref d) = q.delimiter {
        xml.push_str(&format!("  <Delimiter>{}</Delimiter>\n", xml_escape(d)));
    } else {
        xml.push_str("  <Delimiter/>\n");
    }
    xml.push_str(&format!("  <MaxKeys>{max_keys}</MaxKeys>\n"));
    xml.push_str("  <EncodingType>url</EncodingType>\n");
    xml.push_str(&format!("  <KeyCount>{}</KeyCount>\n", page.len()));

    xml.push_str(&format!(
        "  <IsTruncated>{}</IsTruncated>\n",
        if is_truncated { "true" } else { "false" }
    ));

    if is_truncated {
        let next = start + page.len();
        xml.push_str(&format!(
            "  <NextContinuationToken>{next}</NextContinuationToken>\n"
        ));
    }

    for item in &page {
        if common_set.contains(item) {
            xml.push_str("  <CommonPrefixes>\n");
            xml.push_str(&format!("    <Prefix>{}</Prefix>\n", xml_escape(item)));
            xml.push_str("  </CommonPrefixes>\n");
        } else if let Ok(Some(entry)) = state.storage.meta_for_key_sync(&bucket, item) {
            append_contents(&mut xml, &entry);
        }
    }

    xml.push_str("</ListBucketResult>\n");

    xml_response(StatusCode::OK, xml)
}

fn append_contents(xml: &mut String, entry: &ListEntry) {
    xml.push_str("  <Contents>\n");
    xml.push_str(&format!("    <Key>{}</Key>\n", xml_escape(&entry.key)));
    xml.push_str(&format!(
        "    <LastModified>{}</LastModified>\n",
        s3_iso8601(entry.last_modified)
    ));
    xml.push_str(&format!("    <ETag>{}</ETag>\n", xml_escape(&entry.etag)));
    xml.push_str(&format!("    <Size>{}</Size>\n", entry.size));
    xml.push_str("    <StorageClass>STANDARD</StorageClass>\n");
    xml.push_str("  </Contents>\n");
}

async fn get_object(State(state): State<AppState>, Path(path): Path<ObjectKeyPath>) -> Response {
    object_response(&state, &path.bucket, &path.key, false).await
}

async fn head_object(State(state): State<AppState>, Path(path): Path<ObjectKeyPath>) -> Response {
    object_response(&state, &path.bucket, &path.key, true).await
}

async fn object_response(state: &AppState, bucket: &str, key: &str, head_only: bool) -> Response {
    if head_only {
        let meta = match state.storage.object_meta(bucket, key).await {
            Ok(Some(m)) => m,
            Ok(None) => {
                let body = s3_error_xml(
                    "NoSuchKey",
                    "The specified key does not exist.",
                    &format!("/{bucket}/{key}"),
                );
                return xml_response(StatusCode::NOT_FOUND, body);
            }
            Err(e) => {
                let body =
                    s3_error_xml("InternalError", &e.to_string(), &format!("/{bucket}/{key}"));
                return xml_response(StatusCode::INTERNAL_SERVER_ERROR, body);
            }
        };

        let mut res = Response::new(Body::empty());
        *res.status_mut() = StatusCode::OK;
        res.headers_mut().insert(
            header::CONTENT_TYPE,
            HeaderValue::from_str(&meta.content_type)
                .unwrap_or_else(|_| HeaderValue::from_static("application/octet-stream")),
        );
        res.headers_mut().insert(
            header::CONTENT_LENGTH,
            HeaderValue::from_str(&meta.size.to_string()).unwrap(),
        );
        res.headers_mut()
            .insert(header::ETAG, HeaderValue::from_str(&meta.etag).unwrap());
        res.headers_mut().insert(
            header::LAST_MODIFIED,
            http_last_modified(meta.last_modified),
        );
        res.headers_mut()
            .insert(header::ACCEPT_RANGES, HeaderValue::from_static("bytes"));
        return res;
    }

    let obj = match state.storage.read_object(bucket, key).await {
        Ok(Some(o)) => o,
        Ok(None) => {
            let body = s3_error_xml(
                "NoSuchKey",
                "The specified key does not exist.",
                &format!("/{bucket}/{key}"),
            );
            return xml_response(StatusCode::NOT_FOUND, body);
        }
        Err(e) => {
            let body = s3_error_xml("InternalError", &e.to_string(), &format!("/{bucket}/{key}"));
            return xml_response(StatusCode::INTERNAL_SERVER_ERROR, body);
        }
    };

    let len = obj.bytes.len();
    let mut res = Response::new(Body::from(obj.bytes));
    *res.status_mut() = StatusCode::OK;
    res.headers_mut().insert(
        header::CONTENT_TYPE,
        HeaderValue::from_str(&obj.content_type)
            .unwrap_or_else(|_| HeaderValue::from_static("application/octet-stream")),
    );
    res.headers_mut().insert(
        header::CONTENT_LENGTH,
        HeaderValue::from_str(&len.to_string()).unwrap(),
    );
    res.headers_mut()
        .insert(header::ETAG, HeaderValue::from_str(&obj.etag).unwrap());
    res.headers_mut()
        .insert(header::LAST_MODIFIED, http_last_modified(obj.last_modified));
    res.headers_mut()
        .insert(header::ACCEPT_RANGES, HeaderValue::from_static("bytes"));
    res
}
