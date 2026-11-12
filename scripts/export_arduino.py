#!/usr/bin/env python3
"""Exporta la misma fuente a un sketch; no mantiene dos implementaciones."""
from pathlib import Path
import shutil
root=Path(__file__).resolve().parents[1]
target=root/'dist'/'MiniGranjaPaulett'
target.mkdir(parents=True,exist_ok=True)
for source in (root/'src').iterdir():
    if source.suffix in ('.h','.cpp'):
        dest=target/('MiniGranjaPaulett.ino' if source.name=='main.cpp' else source.name)
        shutil.copy2(source,dest)
shutil.copytree(root/'data',target/'data',dirs_exist_ok=True)
print(target)
