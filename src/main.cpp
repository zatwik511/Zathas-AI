
#include "inference.h"
#include "cloud_inference.h"
#include "server.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <cstdlib>

// ── Simple .env parser ─────────────────────────────────────────────────────────
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
        << "  " << prog << " [options]\n\n"
        << "Options:\n"
        << "  --model       <path>   Path to the .gguf model file\n"
        << "                         (required unless GROQ_API_KEY is set in .env)\n"
        << "  --port        <int>    HTTP port to listen on        (default: 8080)\n"
        << "  --host        <addr>   Host address                  (default: 0.0.0.0)\n"
        << "  --ctx         <int>    Context size in tokens        (default: 4096)\n"
        << "  --threads     <int>    CPU threads for inference     (default: 4)\n"
        << "  --gpu-layers  <int>    Layers to offload to GPU      (default: 0)\n"
        << "  --max-tokens  <int>    Max tokens per response       (default: 512)\n"
        << "  --temperature <float>  Sampling temperature          (default: 0.7)\n"
        << "  --static-dir  <path>   Directory with frontend files (default: ./frontend)\n"
        << "  --recent-depth <int>   Verbatim sessions kept before summarising (default: 10)\n"
        << "  --env         <path>   Load settings from .env file  (default: .env)\n"
        << "\n"
        << ".env keys:\n"
        << "  MODEL_PATH   — model file path (same as --model)\n"
        << "  PRIME_TOKEN  — bearer token for /api/prime (never use CLI for this)\n"
        << "  GROQ_API_KEY — enables cloud inference on the public route\n"
        << "  CLOUD_MODEL  — Groq model name (default: llama-3.3-70b-versatile)\n";
}

int main(int argc, char* argv[])
{
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

        if      (arg == "--model")        model_path           = next();
        else if (arg == "--port")         srv_cfg.port         = std::stoi(next());
        else if (arg == "--host")         srv_cfg.host         = next();
        else if (arg == "--ctx")          n_ctx                = std::stoi(next());
        else if (arg == "--threads")      n_threads            = std::stoi(next());
        else if (arg == "--gpu-layers")   n_gpu_layers         = std::stoi(next());
        else if (arg == "--max-tokens")   srv_cfg.max_tokens   = std::stoi(next());
        else if (arg == "--temperature")  srv_cfg.temperature  = std::stof(next());
        else if (arg == "--static-dir")   srv_cfg.static_dir   = next();
        else if (arg == "--recent-depth") srv_cfg.recent_depth = std::stoi(next());
        else if (arg == "--env")          env_file             = next();
        else if (arg == "--help" || arg == "-h") { print_usage(argv[0]); return 0; }
        else { std::cerr << "Unknown option: " << arg << "\n"; print_usage(argv[0]); return 1; }
    }

    // ── Read .env ──────────────────────────────────────────────────────────────
    if (model_path.empty())
        model_path = read_env_file(env_file, "MODEL_PATH");
    if (model_path.empty())
        if (const char* v = std::getenv("MODEL_PATH")) model_path = v;

    // PRIME_TOKEN and cloud keys only ever come from .env — never CLI flags.
    srv_cfg.prime_token = read_env_file(env_file, "PRIME_TOKEN");
    const std::string groq_api_key = read_env_file(env_file, "GROQ_API_KEY");
    std::string cloud_model        = read_env_file(env_file, "CLOUD_MODEL");
    if (cloud_model.empty()) cloud_model = "llama-3.3-70b-versatile";

    if (model_path.empty() && groq_api_key.empty()) {
        std::cerr << "Error: Need either --model <path> or GROQ_API_KEY in .env\n\n";
        print_usage(argv[0]);
        return 1;
    }

    // ── Boot ───────────────────────────────────────────────────────────────────
    std::cout << "=== Zathas AI ===\n";

    try {
        // Local engine — loaded only when a model path is given.
        // Required for /api/prime and for memory summarisation.
        std::shared_ptr<InferenceEngine> private_engine;
        if (!model_path.empty()) {
            std::cout << "[main] Loading local model: " << model_path << "\n";
            private_engine = std::make_shared<InferenceEngine>(
                model_path, n_ctx, n_threads, n_gpu_layers);
        } else {
            std::cout << "[main] No local model — /prime disabled\n";
        }

        // Public engine — cloud if key is set, otherwise falls back to local.
        std::shared_ptr<IInferenceEngine> public_engine;
        if (!groq_api_key.empty()) {
            std::cout << "[main] Public route: Groq API (" << cloud_model << ")\n";
            public_engine = std::make_shared<CloudInferenceEngine>(groq_api_key, cloud_model);
        } else {
            std::cout << "[main] Public route: local model\n";
            public_engine = private_engine;
        }

        ChatServer server(private_engine, public_engine, srv_cfg);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "[fatal] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
