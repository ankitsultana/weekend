package com.ankitsultana.experiments;

import java.util.Arrays;
import java.util.Random;
import org.roaringbitmap.buffer.MutableRoaringBitmap;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


public class Main {
  private static final Logger LOGGER = LoggerFactory.getLogger(Main.class);

  public static void main(String[] args) {
    // sizeWithFirstNBitsOn();
    for (int numDocs : Arrays.asList(100_000, 200_000, 500_000, 1_000_000, 2_000_000)) {
      for (int numRandomElements = 1; numRandomElements < 11; numRandomElements++) {
        sizeWithRandom(numDocs, numRandomElements);
      }
    }
  }

  public static void sizeWithFirstNBitsOn() {
    for (int n = 1; n < (1 << 30); n *= 2) {
      MutableRoaringBitmap bitmap = new MutableRoaringBitmap();
      bitmap.add(0L, (long) n);
      System.out.printf("n=%s. size=%s%n", n, bitmap.serializedSizeInBytes());
    }
  }

  public static void sizeWithRandom(int numDocs, int numRandomElements) {
    Random random = new Random();
    MutableRoaringBitmap bitmap = new MutableRoaringBitmap();
    for (int i = 0; i < numRandomElements; i++) {
      int r = random.nextInt() % numDocs;
      while (bitmap.contains(r)) {
        r = random.nextInt() % numDocs;
      }
      bitmap.add(r);
    }
    double ratio = (bitmap.serializedSizeInBytes() * 1.0) / (Integer.BYTES * numRandomElements);
    System.out.printf("NumDocs=%s, NumRandomElements=%s, Size=%s, Integer Bytes=%s, Ratio=%f%n",
        numDocs, numRandomElements, bitmap.serializedSizeInBytes(), Integer.BYTES * numRandomElements, ratio);
  }
}
