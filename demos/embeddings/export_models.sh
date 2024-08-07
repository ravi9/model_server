#!/bin/bash
optimum-cli export openvino --model BAAI/bge-large-en-v1.5 --task feature-extraction bge-large-en-v1.5_embeddings
convert_tokenizer -o bge-large-en-v1.5_tokenizer --skip-special-tokens BAAI/bge-large-en-v1.5