<h1 align="center">
  <img
    src="https://raw.githubusercontent.com/catppuccin/catppuccin/main/assets/misc/transparent.png"
    height="30"
    width="0px"
  />
  picos
  <img
    src="https://raw.githubusercontent.com/catppuccin/catppuccin/main/assets/misc/transparent.png"
    height="30"
    width="0px"
  />
</h1>

<p align="center">
  <a href="https://github.com/crhowell3/yggdrasil-os/stargazers">
    <img
      alt="Stargazers"
      src="https://img.shields.io/github/stars/crhowell3/yggdrasil-os?style=for-the-badge&logo=starship&color=b16286&logoColor=d9e0ee&labelColor=282a36"
    />
  </a>
  <a href="https://github.com/crhowell3/yggdrasil-os/issues">
    <img
      alt="Issues"
      src="https://img.shields.io/github/issues/crhowell3/yggdrasil-os?style=for-the-badge&logo=gitbook&color=d79921&logoColor=d9e0ee&labelColor=282a36"
    />
  </a>
  <a href="https://github.com/crhowell3/yggdrasil-os/contributors">
    <img
      alt="Contributors"
      src="https://img.shields.io/github/contributors/crhowell3/yggdrasil-os?style=for-the-badge&logo=opensourceinitiative&color=689d6a&logoColor=d9e0ee&labelColor=282a36"
    />
  </a>
</p>

&nbsp;

## 💭 About

A very, very small operating system.

## 📕 Documentation

TODO

## 🔰 Getting Started

### Prerequisites

#### Build dependencies

The following tools are required for building the OS image from source:

- `make`
- `nasm`
- `mtools`
- gcc cross compiler dependencies (see Building section)

#### Testing and debugging dependencies

These utilities are optional and are for testing and debugging purposes only:

- `qemu-system-x86`
- `bochs-x`
- `bochsbios`
- `vgabios`

### Building

1. Install dependencies with your package manager:

Ubuntu / Debian:

```shell
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev \
                 texinfo nasm mtools qemu-system-x86
```

Fedora:

```shell
sudo apt install gcc gcc-c++ make bison flex gmp-devel libmpc-devel mpfr-devel \
                 texinfo nasm mtools qemu-system-x86
```

1. Build the required tools with `make toolchain`.

1. Finally, run `make`.

### Running

With `qemu` installed, simply execute the provided `run.sh` script.

### Debugging

With `bochs` installed, simply execute the provided `debug.sh` script.

<p align="center">
  Copyright &copy; 2025-present
  <a href="https://github.com/crhowell3" target="_blank">Cameron Howell</a>
</p>
<p align="center">
  <a href="https://github.com/crhowell3/yggdrasil-os/blob/main/LICENSE"
    ><img
      src="https://img.shields.io/static/v1.svg?style=for-the-badge&label=License&message=MIT&logoColor=d9e0ee&colorA=282a36&colorB=b16286"
  /></a>
</p>
