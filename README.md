# Zathas AI

A full-stack AI chatbot with a pure C++ backend powered by [llama.cpp](https://github.com/ggml-org/llama.cpp). No Python, no Node.js — just a single compiled binary serving both the inference API and the frontend.

---

## Architecture

```
browser ──HTTP──▶ cpp-httplib server (C++)
                    ├── POST /api/chat  → llama.cpp inference
                    ├── GET  /health    → { "status": "ok" }
                    └── GET  /*         → frontend/index.html
```

Conversation history is maintained in the browser and sent with every request, keeping the backend fully stateless.

---

## Prerequisites

| Tool | Version |
|------|---------|
| CMake | ≥ 3.16 |
| C++ compiler | GCC ≥ 11 / Clang ≥ 14 / MSVC 2022 |
| Git | any recent version |

> **GPU (optional):** CUDA toolkit if you want GPU offload via `--gpu-layers`.

---

## 1. Clone the repository

```bash
git clone https://github.com/zatwik511/Zathas-AI.git
cd Zathas-AI
```

No submodules — dependencies are pulled automatically by CMake's `FetchContent`.

---

## 2. Download a model

Zathas AI requires a GGUF-format model. Two lightweight options that run well on CPU:

### Phi-3 Mini (recommended for most hardware)
- **Size:** ~2 GB (Q4_K_M quantisation)
- **URL:** https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf

```bash
# Example — pick the Q4_K_M variant
wget https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf/resolve/main/Phi-3-mini-4k-instruct-q4.gguf
```

### Llama 3.2 1B (ultra-light, for low-RAM machines)
- **Size:** ~0.8 GB (Q4_K_M quantisation)
- **URL:** https://huggingface.co/bartowski/Llama-3.2-1B-Instruct-GGUF

```bash
wget https://huggingface.co/bartowski/Llama-3.2-1B-Instruct-GGUF/resolve/main/Llama-3.2-1B-Instruct-Q4_K_M.gguf
```

---

## 3. Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The binary is placed at `build/zathas_ai` (Linux/macOS) or `build\Release\zathas_ai.exe` (Windows).

> **First build** downloads llama.cpp, cpp-httplib, and nlohmann/json — expect a few minutes.

### GPU build (CUDA)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DLLAMA_CUDA=ON
cmake --build build --parallel
```

---

## 4. Run

### Via CLI argument

```bash
./build/zathas_ai --model ./Phi-3-mini-4k-instruct-q4.gguf
```

### Via .env file

Create a `.env` file in the project root:

```
MODEL_PATH=./Phi-3-mini-4k-instruct-q4.gguf
```

Then run without `--model`:

```bash
./build/zathas_ai
```

### All options

```
--model       <path>   Path to the .gguf model file (required)
--port        <int>    HTTP port                       (default: 8080)
--host        <addr>   Host address                   (default: 0.0.0.0)
--ctx         <int>    Context window size in tokens  (default: 4096)
--threads     <int>    CPU threads for inference      (default: 4)
--gpu-layers  <int>    Layers to offload to GPU       (default: 0)
--max-tokens  <int>    Max tokens per response        (default: 512)
--temperature <float>  Sampling temperature           (default: 0.7)
--static-dir  <path>   Frontend directory             (default: ./frontend)
```

---

## 5. Open the chat

Navigate to **http://localhost:8080** in your browser.

---

## Project structure

```
.
├── CMakeLists.txt          # Build system (FetchContent for all deps)
├── frontend/
│   └── index.html          # Single-file vanilla JS chat UI
├── src/
│   ├── main.cpp            # Entry point, CLI parsing, .env loading
│   ├── inference.h/.cpp    # llama.cpp wrapper — load model, run generation
│   └── server.h/.cpp       # cpp-httplib HTTP server, /api/chat, /health
└── README.md
```

---

## License

MIT
