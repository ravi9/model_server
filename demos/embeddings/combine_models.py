from openvino import compile_model, convert_model
from openvino_tokenizers import convert_tokenizer, connect_models
import openvino as ov

core = ov.Core()
tokenizer_model = core.read_model(model="bge/openvino_tokenizer.xml")
tokenizer_model_compiled = core.compile_model(model=tokenizer_model, device_name="CPU")

embedding_model = core.read_model(model="bge_embeddings/openvino_model.xml")
embedding_model_compiled = core.compile_model(model=embedding_model, device_name="CPU")

combined_model = connect_models(tokenizer_model, embedding_model)
compiled_combined_model = compile_model(combined_model)

ov.save_model(combined_model, "bge_combined/model.xml")
