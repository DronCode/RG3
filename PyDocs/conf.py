import os
import sys

sys.path.insert(0, os.path.abspath('../PyBind'))

project = 'rg3py'
copyright = '2024, DronCode'
author = 'DronCode'

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',
    'sphinx_autodoc_typehints',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

html_theme = 'alabaster'
html_static_path = ['_static']
