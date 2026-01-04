"""
LLM natural-language query interface for the distributed cache.
Translates plain-English prompts into cache operations.

Usage:
    python3 llm_query.py "store the string hello under the key greeting"
    python3 llm_query.py "what is the value of greeting?"
    python3 llm_query.py "which node is the leader?"

Requires: ANTHROPIC_API_KEY in environment (or .env file)
"""

# TODO: implement in a future commit
# Planned:
#   - send user prompt + system prompt to LLM
#   - parse structured JSON response (op, key, value, ttl)
#   - execute op via client.py send()
#   - return natural-language result

import sys

def main():
    if len(sys.argv) < 2:
        print("Usage: llm_query.py <natural language prompt>")
        sys.exit(1)
    prompt = " ".join(sys.argv[1:])
    print(f"[stub] received prompt: {prompt!r}")
    print("LLM interface not yet implemented — coming soon.")

if __name__ == "__main__":
    main()
