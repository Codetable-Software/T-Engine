# T-Engine

T-Engine adalah fondasi bahasa pemrograman baru dengan frontend C++20 dan
runtime C11. Versi awal ini sudah memiliki:

- lexer dengan lokasi baris/kolom dan komentar `#`;
- parser recursive-descent untuk deklarasi, ekspresi, dan `print`;
- type checker untuk menangkap kesalahan tipe sebelum runtime;
- AST yang aman dengan `std::unique_ptr`;
- interpreter numerik, string, boolean, variabel, assignment, dan comparison;
- bytecode compiler dan VM dengan call frame untuk mode runtime;
- standard library bawaan: `len`, `sqrt`, `abs`, dan `typeOf`;
- runtime C ABI untuk output;
- build reproducible dengan CMake;
- test frontend minimal.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Jalankan program

```sh
./build/tengine examples/hello.te
```

Gunakan bytecode VM:

```sh
./build/tengine --vm examples/control_flow.te
./build/tengine --vm examples/builtins.te
```

## Contoh sintaks

```te
let price = 12.5;
let total = price * 2;
print total;

let enabled = false;
enabled = !enabled;
print enabled == true;

fn add(a, b) {
  return a + b;
}

print add(2, 3);

let counter = 0;
while (counter < 3) {
  counter = counter + 1;
}

if (counter == 3 && !false) {
  print counter;
}
```

## Arah arsitektur

Function memiliki parameter, scope lokal, dan bisa dijalankan melalui interpreter
atau bytecode VM. Control flow mendukung `if`,
`else`, `while`, `&&`, dan `||`. Type checker memeriksa jumlah argumen, tipe
kondisi, dan mencegah `return` di luar function. Tahap berikutnya adalah error
recovery yang lebih lengkap, source locations di AST, lalu backend bytecode
sebelum optimasi native atau JIT. Pemisahan frontend, type checker, interpreter,
dan runtime sengaja dibuat agar perubahan bahasa tidak mengikat implementasi
runtime.