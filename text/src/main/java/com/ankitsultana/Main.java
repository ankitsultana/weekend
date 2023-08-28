package com.ankitsultana;

import java.io.FileNotFoundException;
import java.io.FileReader;
import org.apache.lucene.analysis.Analyzer;
import org.apache.lucene.analysis.TokenStream;
import org.apache.lucene.analysis.standard.StandardAnalyzer;


public class Main {
  public static void main(String[] args)
      throws FileNotFoundException {
    try (Analyzer analyzer = new StandardAnalyzer()) {
      TokenStream tokenStream = analyzer.tokenStream("my_field", new FileReader("data/test.txt"));
    }
  }
}
