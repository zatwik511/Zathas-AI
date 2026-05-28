# Zathas AI

A full-stack AI chatbot with a pure C++ backend powered by [llama.cpp](https://github.com/ggml-org/llama.cpp). No Python, no Node.js — just a single compiled binary serving both the inference API and the frontend.

---

## Architecture

```
browser ──HTTP──▶ cpp-httplib server (C++)
                    ├── POST /api/chat   → public persona, SSE streaming
                    ├── GET  /health     → { "status": "ok" }
                    └── GET  /*          → frontend/index.html
```

Persistent memory is stored in plain-text session files under `memory/`. Recent sessions are kept verbatim; older ones are compressed into `.summary` files automatically on shutdown.

---

## Prerequisites

| Tool | Version |
|------|---------|
| CMake | ≥ 3.16 |
| C++ compiler | GCC ≥ 11 / Clang ≥ 14 / MSVC 2022 |
| Git | any recent version |

> **GPU (optional):** Vulkan or CUDA. Pass `--gpu-layers <N>` at runtime.

---

## Setup

### 1. Clone the repository

```bash
git clone https://github.com/zatwik511/Zathas-AI.git
cd Zathas-AI
```

No submodules — dependencies are pulled automatically by CMake's `FetchContent`.

### 2. Download a model

Place your GGUF model in the `models/` directory (or anywhere — you reference it by path).

**Llama 3.2 1B Instruct** (recommended starting point — ~0.8 GB):
```bash
# Download from Hugging Face
wget -P models/ https://huggingface.co/bartowski/Llama-3.2-1B-Instruct-GGUF/resolve/main/Llama-3.2-1B-Instruct-Q4_K_M.gguf
```

**Phi-3 Mini** (~2 GB, higher quality):
```bash
wget -P models/ https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf/resolve/main/Phi-3-mini-4k-instruct-q4.gguf
```

### 3. Configure `.env`

```bash
cp .env.example .env
# Edit .env and set MODEL_PATH (and optionally PRIME_TOKEN)
```

```ini
MODEL_PATH=./models/Llama-3.2-1B-Instruct-Q4_K_M.gguf
PRIME_TOKEN=your-secret-token
```

`PRIME_TOKEN` gates access to the private conversation endpoint. Leave empty to disable authentication.

### 4. Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The binary is placed at `build/zathas_ai` (Linux/macOS) or `build\zathas_ai.exe` (Windows/MinGW).

> **First build** downloads llama.cpp, cpp-httplib, and nlohmann/json — expect a few minutes.

**GPU build (Vulkan — AMD/Intel):**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DGGML_VULKAN=ON
cmake --build build --parallel
```

**GPU build (CUDA — NVIDIA):**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DGGML_CUDA=ON
cmake --build build --parallel
```

### 5. Run

```bash
# Uses MODEL_PATH from .env
./build/zathas_ai

# Or pass the model path directly
./build/zathas_ai --model ./models/Llama-3.2-1B-Instruct-Q4_K_M.gguf --threads 8 --gpu-layers 20
```

Navigate to **http://localhost:8080** in your browser.

---

## All options

```
--model         <path>    Path to the .gguf model file (required if not in .env)
--port          <int>     HTTP port                         (default: 8080)
--host          <addr>    Host address                      (default: 0.0.0.0)
--ctx           <int>     Context window size in tokens     (default: 4096)
--threads       <int>     CPU threads for inference         (default: 4)
--gpu-layers    <int>     Layers to offload to GPU          (default: 0)
--max-tokens    <int>     Max tokens per response           (default: 512)
--temperature   <float>   Sampling temperature              (default: 0.7)
--static-dir    <path>    Frontend directory                (default: ./frontend)
--recent-depth  <int>     Verbatim sessions before summarising (default: 10)
--env           <path>    Path to .env file                 (default: .env)
```

---

## Project structure

```
.
├── CMakeLists.txt           # Build system — FetchContent for all deps
├── .env.example             # Template — copy to .env and fill in values
├── frontend/
│   └── index.html           # Single-file vanilla JS chat UI
├── memory/                  # Conversation memory (gitignored — only .gitkeep committed)
│   └── public/              # Public conversation summaries
├── models/                  # Place .gguf model files here (gitignored)
├── src/
│   ├── main.cpp             # Entry point, CLI parsing, .env loading
│   ├── inference.h/.cpp     # llama.cpp wrapper — load model, run generation
│   ├── memory.h/.cpp        # Persistent session memory with summarisation
│   └── server.h/.cpp        # HTTP server — /api/chat, /health, static files
└── README.md
```

---

## License

MIT
