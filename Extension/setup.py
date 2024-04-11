from setuptools import setup
from typing import Dict
from os import getenv
from pathlib import Path


known_os_ext: Dict[str, str] = {
    'linux': 'so',
    'macos': 'so',
    'windows': 'pyd'
}

known_os_to_os_tag: Dict[str, str] = {
    'linux':   'Linux',
    'macos':   'MacOS',
    'windows': 'Windows'
}

os_classifiers: Dict[str, str] = {
    'linux':   'Operating System :: POSIX :: Linux',
    'macos':   'Operating System :: MacOS :: MacOS X',
    'windows': 'Operating System :: Microsoft :: Windows'
}

# Read env
target_version: str = getenv('RG3_DEPLOY_VERSION', 'none')
target_os: str = getenv('RG3_DEPLOY_OS', 'none')

if target_version == 'none':
    raise RuntimeError("RG3_DEPLOY_VERSION not specified!")

if target_os not in known_os_ext:
    raise RuntimeError(f"RG3_DEPLOY_OS not specified or unknown ext for '{target_os}'")

platform_ext: str = known_os_ext[target_os]

print("*************************************************")
print(f"RG3 Deploy for version {target_version} target os {target_os} target ext {platform_ext}")
print("*************************************************")

# Readme
this_dir: Path = Path(__file__).parent
long_description_contents: str = (this_dir / "README.md").read_text()

setup(
    name='rg3py',
    version=target_version,
    author='DronCode',
    author_email='alexandrleutin@gmail.com',
    description='RG3 is a C/C++ analyzer framework',
    long_description=long_description_contents,
    long_description_content_type='text/markdown',
    packages=['rg3py'],
    package_data={'rg3py': [f'rg3py.{platform_ext}', 'rg3py.pyi']},
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: POSIX :: Linux',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows'
        'Programming Language :: Python :: Implementation :: CPython',
        'Topic :: Software Development :: Code Generators',
        'Topic :: Software Development :: Libraries'
    ],
    python_requires='>=3.10',
    platforms=[known_os_to_os_tag[target_os]],
)
