# Contributing

One rule: never commit secrets, model weights, or private conversation data.

- Keep `.env` out of commits — use `.env.example` as the template.
- Model files (`.gguf`, `.bin`, `.ggml`) are in `.gitignore` — do not force-add them.
- The `memory/` directory holds private conversation history — its contents are ignored by `.gitignore` intentionally.
