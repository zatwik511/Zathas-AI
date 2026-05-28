
#include "inference.h"
#include "server.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <cstdlib>

// ── Simple .env parser ─────────────────────────────────────────────────────────
// Reads KEY=VALUE lines; ignores blank lines and lines starting with #.
static std::string read_env_file(const std::string& path, const std::string& key)
{
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        if (line.substr(0, eq) == key)
            return line.substr(eq + 1);
    }
    return {};
}

static void print_usage(const char* prog)
{
    std::cerr
        << "Usage:\n"
        << "  " << prog << " --model <path/to/model.gguf> [options]\n\n"
        << "Options:\n"
        << "  --model       <path>   Path to the .gguf model file (required)\n"
        << "  --port        <int>    HTTP port to listen on        (default: 8080)\n"
        << "  --host        <addr>   Host address                  (default: 0.0.0.0)\n"
        << "  --ctx         <int>    Context size in tokens        (default: 4096)\n"
        << "  --threads     <int>    CPU threads for inference     (default: 4)\n"
        << "  --gpu-layers  <int>    Layers to offload to GPU      (default: 0)\n"
        << "  --max-tokens  <int>    Max tokens per response       (default: 512)\n"
        << "  --temperature <float>  Sampling temperature          (default: 0.7)\n"
        << "  --static-dir    <path>   Directory with frontend files (default: ./frontend)\n"
        << "  --memory-depth  <int>    Past sessions to load as context (default: 5)\n"
        << "  --env           <path>   Load settings from .env file  (default: .env)\n"
        << "\n"
        << "You can also set MODEL_PATH in a .env file instead of --model.\n";
}

int main(int argc, char* argv[])
{
    // Defaults
    std::string model_path;
    std::string env_file    = ".env";
    ServerConfig srv_cfg;
    int n_ctx        = 4096;
    int n_threads    = 4;
    int n_gpu_layers = 0;

    // ── Parse CLI args ─────────────────────────────────────────────────────────
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << arg << " requires a value\n";
                std::exit(1);
            }
            return argv[++i];
        };

        if      (arg == "--model")       model_path           = next();
        else if (arg == "--port")        srv_cfg.port         = std::stoi(next());
        else if (arg == "--host")        srv_cfg.host         = next();
        else if (arg == "--ctx")         n_ctx                = std::stoi(next());
        else if (arg == "--threads")     n_threads            = std::stoi(next());
        else if (arg == "--gpu-layers")  n_gpu_layers         = std::stoi(next());
        else if (arg == "--max-tokens")  srv_cfg.max_tokens   = std::stoi(next());
        else if (arg == "--temperature") srv_cfg.temperature  = std::stof(next());
        else if (arg == "--static-dir")    srv_cfg.static_dir    = next();
        else if (arg == "--memory-depth")  srv_cfg.memory_depth  = std::stoi(next());
        else if (arg == "--env")           env_file              = next();
        else if (arg == "--help" || arg == "-h") { print_usage(argv[0]); return 0; }
        else { std::cerr << "Unknown option: " << arg << "\n"; print_usage(argv[0]); return 1; }
    }

    // ── Fall back to .env for model path ───────────────────────────────────────
    if (model_path.empty()) {
        model_path = read_env_file(env_file, "MODEL_PATH");
    }
    if (model_path.empty()) {
        // Last resort: check environment variable
        if (const char* env_val = std::getenv("MODEL_PATH")) {
            model_path = env_val;
        }
    }
    if (model_path.empty()) {
        std::cerr << "Error: No model path specified.\n\n";
        print_usage(argv[0]);
        return 1;
    }

    // ── Boot ───────────────────────────────────────────────────────────────────
    std::cout << "=== Zathas AI ===\n";
    std::cout << "[main] Loading model: " << model_path << "\n";

    try {
        auto engine = std::make_shared<InferenceEngine>(
            model_path, n_ctx, n_threads, n_gpu_layers
        );

        ChatServer server(engine, srv_cfg);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "[fatal] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
