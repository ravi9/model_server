from transformers import AutoTokenizer, AutoModel
from optimum.intel import IPEXModel
from optimum.intel import OVModelForFeatureExtraction
import torch
import datetime


from datasets import load_dataset
docs = load_dataset(f"Cohere/wikipedia-22-12-simple-embeddings", split="train")

# Sentences we want sentence embeddings for
# Load model from HuggingFace Hub
model_id = "BAAI/bge-large-en-v1.5"
tokenizer = AutoTokenizer.from_pretrained(model_id)
model_ipex = IPEXModel.from_pretrained(model_id)
model_pt = AutoModel.from_pretrained(model_id)
model_ov = model = OVModelForFeatureExtraction.from_pretrained('bge-large-en-v1.5_embeddings',ov_config={"MODEL_DISTRIBUTION_POLICY":"TENSOR_PARALLEL"})

def run_model(model):
    with torch.no_grad():
        start_time = datetime.datetime.now()
        iter = docs.iter(batch_size=16)
        count = 0
        for i in iter:
            text = i['text']
            input = tokenizer(text, padding=True, truncation=True, return_tensors='pt')
            #emb = i['emb']
            model_output = model(**input)
            sentence_embeddings = model_output[0][:, 0]
            count += 1
            if count > 100:
                break
        end_time = datetime.datetime.now()
        duration = (end_time - start_time).total_seconds() * 1000
        print("Duration:", duration, "ms", type(model).__name__)
        return model_output
    
run_model(model_ov)
run_model(model_ipex)
run_model(model_pt)



