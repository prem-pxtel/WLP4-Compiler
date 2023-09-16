# WLP4-Compiler

A full compiler covering tokenization, parsing, type-checking, and code generation for WLP4, a high-level programming
language that uses the C syntax.

With any WLP4 program, by going through the pipeline of wlp4scan, wlp4parse, wlp4type and wlp4gen, we are left with a MIPS assembly program which can then be used as input for asm.cc. This will ultimately give us a final output in MIPS machine code, encoding binary instructions with the same behaviour as the initial WLP4 program.

## Example WLP4 Program
```
int wain(int* a, int b) {
  int* last = NULL;
  last = a+b-1;
  while(a < last) {
    a = a+1;
  }
  return *a;
}
```
