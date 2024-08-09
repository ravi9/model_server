from openvino import compile_model, convert_model
from openvino_tokenizers import convert_tokenizer, connect_models
import openvino as ov

core = ov.Core()
# embedding model
tokenizer_model = core.read_model(model="bge-large-en-v1.5_tokenizer/openvino_tokenizer.xml")
embedding_model = core.read_model(model="bge-large-en-v1.5_embeddings/openvino_model.xml")

combined_model = connect_models(tokenizer_model, embedding_model)
compiled_combined_model = compile_model(combined_model)

ov.save_model(combined_model, "bge_combined/model.xml")

# rerank model

tokenizer_rerank_model = core.read_model(model="bge-reranker-large/openvino_tokenizer.xml")
rerank_model = core.read_model(model="bge-reranker-large/openvino_model.xml")

combined_rerank_model = connect_models(tokenizer_rerank_model, rerank_model)
compiled_combined_rerank_model = compile_model(combined_model)

ov.save_model(combined_rerank_model, "rerank_combined/model.xml")