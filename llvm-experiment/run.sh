clang++ fourth.cpp -o test \
  -O3 \
  -I/opt/homebrew/opt/llvm/include \
  -L/opt/homebrew/opt/llvm/lib \
  -lLLVM && ./test
