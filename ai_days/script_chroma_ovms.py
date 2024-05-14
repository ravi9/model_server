#*****************************************************************************
# Copyright 2024 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#*****************************************************************************

# pip install langchain_community openai sentence_transformers chromadb unstructured

import sys
import time
import os
import requests

from langchain_community.llms import HuggingFacePipeline, VLLMOpenAI
from langchain_community.embeddings import HuggingFaceEmbeddings
from langchain.chains import RetrievalQA

TARGET_FOLDER = "./ovms/html_files/"

# Document Splitter
from typing import List
from langchain.text_splitter import CharacterTextSplitter, RecursiveCharacterTextSplitter, MarkdownTextSplitter
from langchain_community.document_loaders import (
    CSVLoader,
    EverNoteLoader,
    PDFMinerLoader,
    TextLoader,
    UnstructuredEPubLoader,
    UnstructuredHTMLLoader,
    UnstructuredMarkdownLoader,
    UnstructuredODTLoader,
    UnstructuredPowerPointLoader,
    UnstructuredWordDocumentLoader, )
from langchain.prompts import PromptTemplate
from langchain_community.vectorstores import Chroma
from langchain.chains import RetrievalQA
from langchain.docstore.document import Document

TEXT_SPLITERS = {
    "Character": CharacterTextSplitter,
    "RecursiveCharacter": RecursiveCharacterTextSplitter,
    "Markdown": MarkdownTextSplitter,
}

LOADERS = {
    ".csv": (CSVLoader, {}),
    ".doc": (UnstructuredWordDocumentLoader, {}),
    ".docx": (UnstructuredWordDocumentLoader, {}),
    ".enex": (EverNoteLoader, {}),
    ".epub": (UnstructuredEPubLoader, {}),
    ".html": (UnstructuredHTMLLoader, {}),
    ".md": (UnstructuredMarkdownLoader, {}),
    ".odt": (UnstructuredODTLoader, {}),
    ".pdf": (PDFMinerLoader, {}),
    ".ppt": (UnstructuredPowerPointLoader, {}),
    ".pptx": (UnstructuredPowerPointLoader, {}),
    ".txt": (TextLoader, {"encoding": "utf8"}),
}

def load_single_document(file_path: str) -> List[Document]:
    """
    helper for loading a single document

    Params:
      file_path: document path
    Returns:
      documents loaded

    """
    ext = "." + file_path.rsplit(".", 1)[-1]
    if ext in LOADERS:
        loader_class, loader_args = LOADERS[ext]
        loader = loader_class(file_path, **loader_args)
        return loader.load()

    raise ValueError(f"File does not exist '{ext}'")


embeddings=HuggingFaceEmbeddings(
            model_name="sentence-transformers/all-mpnet-base-v2",
            model_kwargs={"device":"cpu"},
            show_progress=True
            )


documents = []
for file_path in os.listdir(TARGET_FOLDER):
    abs_path = os.path.join(TARGET_FOLDER, file_path)
    print(f"Reading document {abs_path}...", flush=True)
    documents.extend(load_single_document(abs_path))

spliter_name = "RecursiveCharacter"  # TODO: Param?
chunk_size=1000  # TODO: Param?
chunk_overlap=200  # TODO: Param?
text_splitter = TEXT_SPLITERS[spliter_name](chunk_size=chunk_size, chunk_overlap=chunk_overlap)

texts = text_splitter.split_documents(documents)
db = Chroma.from_documents(texts, embeddings)

vector_search_top_k = 4
retriever = db.as_retriever(search_kwargs={"k": vector_search_top_k})

llm = VLLMOpenAI(
    openai_api_key="EMPTY",
    openai_api_base="http://ov-spr-19.sclab.intel.com:8002/v1",
    model_name="mistralai/Mistral-7B-Instruct-v0.1",
    verbose=True
)

qa = RetrievalQA.from_chain_type(llm=llm, chain_type="stuff", retriever=retriever)

while True:
    print("> ", end='', flush=True)
    q = input()
    answer = qa.invoke(q)['result']
    print(answer)
