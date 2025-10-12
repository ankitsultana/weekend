clang++ toy.cpp -o test \
  -I/opt/homebrew/opt/llvm/include \
  -L/opt/homebrew/opt/llvm/lib \
  -lLLVM && ./test
