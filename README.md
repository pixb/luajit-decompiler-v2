## LuaJIT Decompiler v2

*LuaJIT Decompiler v2* is a replacement tool for the old and now mostly defunct python decompiler.  
The project fixes all of the bugs and quirks the python decompiler had while also offering  
full support for gotos and stripped bytecode including locals and upvalues.

## Usage

1. Head to the release section and download the latest executable.
2. Drag and drop a valid LuaJIT bytecode file or a folder containing such files onto the exe.  
Alternatively, run the program in a command prompt. Use `-?` to show usage and options.
3. All successfully decompiled `.lua` files are placed by default into the `output` folder  
located in the same directory as the exe.

Feel free to [report any issues](https://github.com/marsinator358/luajit-decompiler-v2/issues/new) you have.

## macOS/Linux Support

This project has been successfully ported to macOS and Linux! The following modifications were made:

### Building on macOS/Linux

```bash
g++ -std=c++20 -funsigned-char -fms-extensions -I. -I./bytecode -I./ast -I./lua \
    main.cpp bytecode/bytecode.cpp bytecode/prototype.cpp ast/ast.cpp lua/lua.cpp \
    -o luajit-decompiler-v2
```

### Or use CMake

```bash
mkdir build && cd build
cmake ..
make
```

### Usage

```bash
# Decompile a single file
./luajit-decompiler-v2 input.lua -o output

# Decompile a directory
./luajit-decompiler-v2 input_dir -o output_dir
```

## KTX/ETC2 Image Conversion

Games using KTX/ETC2 compressed textures (common in mobile games) can convert them to PNG:

### Python Method (Recommended)

```bash
# Install dependencies
pip install texture2ddecoder Pillow

# Convert single file
python test-pkm/tool/pkm2png.py input.pkm output.png

# Batch convert directory
python test-pkm/batch_convert.py test/asset/image2 output/asset/image
```

### Features
- Pure software decoding (no GPU required)
- Supports ETC1, ETC2, ETC2A1, ETC2A8 formats
- Also supports ASTC, BC1-7, ATC, PVRTC, EAC

## TODO

* bytecode big endian support
* improved decompilation logic for conditional assignments

---

This project uses an boolean expression decompilation algorithm that is based on this paper:  
[www.cse.iitd.ac.in/~sak/reports/isec2016-paper.pdf](https://www.cse.iitd.ac.in/~sak/reports/isec2016-paper.pdf)
