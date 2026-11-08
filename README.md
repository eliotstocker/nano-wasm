# nano-wasm

GNU `nano 8.0` text editor compiled to WebAssembly (`wasm32-wasip1`), statically linked with wide-character `ncurses 6.5` (`libncursesw.a` + `libtinfow.a`), built for [brsh](https://github.com/eliotstocker/Brsh-JS) and browser WASI environments.

## Features

- **Compiled-in Terminfo Fallbacks**: `xterm-256color`, `xterm`, `vt220`, `vt100`, and `dumb` are compiled directly into `libtinfow.a` via `MKfallback.sh` — no external `/usr/share/terminfo` database files required at runtime.
- **Dynamic Terminal Dimensions (`TIOCGWINSZ`)**: Queries `LINES` and `COLUMNS` environment variables (defaulting to `80x24`) via a lightweight WASI `termios` / `ioctl` shim.
- **Standard `wasi_snapshot_preview1` ABI**: Zero custom host (`env.*`) imports; runs directly in `brsh`'s `WasmCommand` or any `wasi_snapshot_preview1` runtime.
- **Compact Binary**: `574 KB` stripped (`dist/nano.wasm`).

## Usage in `brsh`

### Via `wbt` (`brsh` WebAssembly Binary Tool)

```sh
wbt install https://cdn.jsdelivr.net/gh/eliotstocker/nano-wasm@main/dist/nano.wasm --name nano
nano hello.txt
```

### Mounting Directly in `brsh` Virtual Filesystem

```js
import Brsh from 'brsh';

const response = await fetch('https://cdn.jsdelivr.net/gh/eliotstocker/nano-wasm@main/dist/nano.wasm');
const wasmBuffer = await response.arrayBuffer();

const shell = new Brsh({
  bin: {
    nano: new Brsh.BinaryFile(wasmBuffer, 'application/wasm')
  }
});
```

## Building from Source

```sh
npm run build
# or directly:
bash scripts/build.sh
```

The build script automatically downloads `wasi-sdk-25.0`, `ncurses-6.5`, and `nano-8.0`, builds the host `tic`/`infocmp` tools to generate the fallback terminfo table, compiles the WASI POSIX/termios shim (`shim/`), and links `dist/nano.wasm`.

## License

GPL-3.0-or-later (matching GNU `nano`).
