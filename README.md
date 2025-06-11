# SharkWire libdragon nonsense

### Building

Make sure you have Rust and libdragon installed.

#### Prepping libdragon

Make sure libdragon is built from source and you're running the `preview` branch.

Make these changes:

- Find `boot/ique_trampoline.S`. Replace `lui     $t2, 0xB000` (should be at or around line 24) with `lui     $t2, 0xB0C0`.
- Find `boot/loader.c`. Replace `uint32_t elf_header = 0x10001000;` (should be at or around line 246) with `uint32_t elf_header = 0x10C01000;`.

Navigate to the libdragon directory in a terminal. Run these commands:
- `make -C boot clean`
- `make -C boot PROD=1 install`
- `make tools-clean tools-install`

#### Cloning

Clone this repo recursively (`git clone https://github.com/Jhynjhiruu/sharkwire-libdragon.git --recursive` or equivalent).

#### Building

Navigate to the root of the repository and run `make`. This should build the ROM, then build `gscrc` and run it, producing output in `fw.bin`.
