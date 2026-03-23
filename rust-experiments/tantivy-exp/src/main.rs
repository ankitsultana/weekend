use tantivy::collector::TopDocs;
use tantivy::query::QueryParser;
use tantivy::schema::{Schema, TantivyDocument, STORED, TEXT};
use tantivy::{doc, Document, Index, ReloadPolicy};

fn main() -> tantivy::Result<()> {
    let mut schema_builder = Schema::builder();
    let title = schema_builder.add_text_field("title", TEXT | STORED);
    let schema = schema_builder.build();

    let index = Index::create_in_ram(schema.clone());

    let mut index_writer = index.writer(50_000_000)?;
    index_writer.add_document(doc!(title => "Hello, Tantivy!"))?;
    index_writer.commit()?;

    let reader = index
        .reader_builder()
        .reload_policy(ReloadPolicy::OnCommitWithDelay)
        .try_into()?;
    let searcher = reader.searcher();

    let query_parser = QueryParser::for_index(&index, vec![title]);
    let query = query_parser.parse_query("hello")?;
    let top_docs = searcher.search(&query, &TopDocs::with_limit(10))?;

    println!("Hello from Tantivy — matches for \"hello\":");
    for (_score, doc_address) in top_docs {
        let retrieved_doc: TantivyDocument = searcher.doc(doc_address)?;
        println!("  {}", retrieved_doc.to_json(&schema));
    }

    Ok(())
}
