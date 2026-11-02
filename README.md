# PokeRNG (`3ds_rngtool`)

> **RNG & Cheat Generator** — Nintendo 3DS Homebrew (`CTR-P-PRNG`, UniqueId `0x0D3E5`) by [**lafiabrew**](https://github.com/orgs/lafiabrew/repositories)

Pokémon RNG shiny frame finder, TSV/G7TID calculator, and Luma3DS Rosalina cheats.txt generator.

## Build & Verify

```bash
# Run host C11 ASan/UBSan unit & UI simulation tests
make test

# Build ARM11 3DS .elf, .smdh, .3dsx, and .cia (via devkitARM or rootless Podman container)
make 3ds && make verify-3ds

# Serve FBI QR code or push .cia over LAN
make qr
```
