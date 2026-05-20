# Knight script adapter

`adapter_client` connects to the game server, handles role negotiation (`ClientBase`), and talks to a **comp-lang** brain over TCP using a line-based int protocol (`sock_send_int` / `sock_recv_int` compatible).

Gameplay logic lives only in `clients/knight/knight_brain.cmm`.

## Build (Windows / MSYS2 MinGW)

From repo root:

```powershell
cmake -B build -S . -G "MinGW Makefiles" -DBUILD_GRAPHICS=OFF -DSANITIZE=OFF
cmake --build build --target adapter_client knight_client
```

`inih` is vendored under `third_party/inih` (no pkg-config needed).
`msva` server still needs Linux (`dlfcn`) — run the server elsewhere, clients build on Windows.

Compile the brain with [comp-lang](https://github.com/NeIIor/comp-lang) (from its `build/` directory):

```powershell
.\cmm_frontend.exe path\to\knight_brain.cmm knight_ast.tree
.\lang_optimizer.exe knight_ast.tree knight_opt.tree
.\lang_compile.exe knight_opt.tree knight_brain.exe
```

Copy `sfasmlib.dll` next to `knight_brain.exe`.

## Run

Terminal 1 — server:

```bash
./build/servers/msva/msva ./servers/msva/sample.ini
```

Terminal 2a — adapter auto-starts brain (Linux / WSL):

```bash
./build/clients/adapter/adapter_client ./knight_brain.exe ./clients/adapter/default.ini
```

Terminal 2b — manual brain (Windows-friendly):

```powershell
.\knight_brain.exe
```

```powershell
.\adapter_client.exe - .\clients\adapter\default.ini
```

`brain.ini` sets the brain TCP port (default `17771`). The brain executable must use the same port in `knight_brain.cmm`.

## Int protocol

Adapter → brain: `tick`, `hp`, `at`, `root`, `enemy`, `wall`, `ability slash` (see `brain_protocol.hpp`).

Brain → adapter: `none`, `move(dx,dy)`, `use(target)`, `stop`.

The adapter maps `use` to `knight:use` with ability id `"slash"`.
