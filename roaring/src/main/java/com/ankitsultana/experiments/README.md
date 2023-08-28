# README

This code tries to estimate the size of a Roaring bitmap when the set of integers it represents
has very low cardinality, and the integers themselves are random.

```
NumDocs=100000, NumRandomElements=1, Size=18, Integer Bytes=4, Ratio=4.500000
NumDocs=100000, NumRandomElements=2, Size=20, Integer Bytes=8, Ratio=2.500000
NumDocs=100000, NumRandomElements=3, Size=22, Integer Bytes=12, Ratio=1.833333
NumDocs=100000, NumRandomElements=4, Size=32, Integer Bytes=16, Ratio=2.000000
NumDocs=100000, NumRandomElements=5, Size=50, Integer Bytes=20, Ratio=2.500000
NumDocs=100000, NumRandomElements=6, Size=52, Integer Bytes=24, Ratio=2.166667
NumDocs=100000, NumRandomElements=7, Size=46, Integer Bytes=28, Ratio=1.642857
NumDocs=100000, NumRandomElements=8, Size=48, Integer Bytes=32, Ratio=1.500000
NumDocs=100000, NumRandomElements=9, Size=58, Integer Bytes=36, Ratio=1.611111
NumDocs=100000, NumRandomElements=10, Size=60, Integer Bytes=40, Ratio=1.500000
NumDocs=200000, NumRandomElements=1, Size=18, Integer Bytes=4, Ratio=4.500000
NumDocs=200000, NumRandomElements=2, Size=28, Integer Bytes=8, Ratio=3.500000
NumDocs=200000, NumRandomElements=3, Size=30, Integer Bytes=12, Ratio=2.500000
NumDocs=200000, NumRandomElements=4, Size=48, Integer Bytes=16, Ratio=3.000000
NumDocs=200000, NumRandomElements=5, Size=50, Integer Bytes=20, Ratio=2.500000
NumDocs=200000, NumRandomElements=6, Size=44, Integer Bytes=24, Ratio=1.833333
NumDocs=200000, NumRandomElements=7, Size=54, Integer Bytes=28, Ratio=1.928571
NumDocs=200000, NumRandomElements=8, Size=72, Integer Bytes=32, Ratio=2.250000
NumDocs=200000, NumRandomElements=9, Size=74, Integer Bytes=36, Ratio=2.055556
NumDocs=200000, NumRandomElements=10, Size=76, Integer Bytes=40, Ratio=1.900000
NumDocs=500000, NumRandomElements=1, Size=18, Integer Bytes=4, Ratio=4.500000
NumDocs=500000, NumRandomElements=2, Size=28, Integer Bytes=8, Ratio=3.500000
NumDocs=500000, NumRandomElements=3, Size=22, Integer Bytes=12, Ratio=1.833333
NumDocs=500000, NumRandomElements=4, Size=48, Integer Bytes=16, Ratio=3.000000
NumDocs=500000, NumRandomElements=5, Size=58, Integer Bytes=20, Ratio=2.900000
NumDocs=500000, NumRandomElements=6, Size=68, Integer Bytes=24, Ratio=2.833333
NumDocs=500000, NumRandomElements=7, Size=78, Integer Bytes=28, Ratio=2.785714
NumDocs=500000, NumRandomElements=8, Size=88, Integer Bytes=32, Ratio=2.750000
NumDocs=500000, NumRandomElements=9, Size=82, Integer Bytes=36, Ratio=2.277778
NumDocs=500000, NumRandomElements=10, Size=92, Integer Bytes=40, Ratio=2.300000
NumDocs=1000000, NumRandomElements=1, Size=18, Integer Bytes=4, Ratio=4.500000
NumDocs=1000000, NumRandomElements=2, Size=28, Integer Bytes=8, Ratio=3.500000
NumDocs=1000000, NumRandomElements=3, Size=38, Integer Bytes=12, Ratio=3.166667
NumDocs=1000000, NumRandomElements=4, Size=48, Integer Bytes=16, Ratio=3.000000
NumDocs=1000000, NumRandomElements=5, Size=58, Integer Bytes=20, Ratio=2.900000
NumDocs=1000000, NumRandomElements=6, Size=68, Integer Bytes=24, Ratio=2.833333
NumDocs=1000000, NumRandomElements=7, Size=78, Integer Bytes=28, Ratio=2.785714
NumDocs=1000000, NumRandomElements=8, Size=88, Integer Bytes=32, Ratio=2.750000
NumDocs=1000000, NumRandomElements=9, Size=98, Integer Bytes=36, Ratio=2.722222
NumDocs=1000000, NumRandomElements=10, Size=84, Integer Bytes=40, Ratio=2.100000
NumDocs=2000000, NumRandomElements=1, Size=18, Integer Bytes=4, Ratio=4.500000
NumDocs=2000000, NumRandomElements=2, Size=28, Integer Bytes=8, Ratio=3.500000
NumDocs=2000000, NumRandomElements=3, Size=38, Integer Bytes=12, Ratio=3.166667
NumDocs=2000000, NumRandomElements=4, Size=48, Integer Bytes=16, Ratio=3.000000
NumDocs=2000000, NumRandomElements=5, Size=58, Integer Bytes=20, Ratio=2.900000
NumDocs=2000000, NumRandomElements=6, Size=60, Integer Bytes=24, Ratio=2.500000
NumDocs=2000000, NumRandomElements=7, Size=62, Integer Bytes=28, Ratio=2.214286
NumDocs=2000000, NumRandomElements=8, Size=80, Integer Bytes=32, Ratio=2.500000
NumDocs=2000000, NumRandomElements=9, Size=98, Integer Bytes=36, Ratio=2.722222
NumDocs=2000000, NumRandomElements=10, Size=100, Integer Bytes=40, Ratio=2.500000
```