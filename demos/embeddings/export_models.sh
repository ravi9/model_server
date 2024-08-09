#!/bin/bash
optimum-cli export openvino --model BAAI/bge-large-en-v1.5 --task feature-extraction bge-large-en-v1.5_embeddings
convert_tokenizer -o bge-large-en-v1.5_tokenizer --skip-special-tokens BAAI/bge-large-en-v1.5

optimum-cli export openvino --model BAAI/bge-reranker-large  bge-reranker-large --trust-remote-code

optimum-cli export openvino -m Alibaba-NLP/gte-large-en-v1.5 --library sentence_transformers --task feature-extraction gte-large-en-v1.5 --trust-remote-code
optimum-cli export openvino -m Alibaba-NLP/gte-Qwen2-7B-instruct --library sentence_transformers --task feature-extraction gte-Qwen2-7B-instruct --trust-remote-code
