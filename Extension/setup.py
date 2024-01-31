from setuptools import setup
from typing import Dict
from os import getenv


known_os_ext: Dict[str, str] = {
    'linux': 'so',
    'macos': 'so',
    'windows': 'pyd'
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


setup(
    name='rg3py',
    version=target_version,
    author='DronCode',
    author_email='alexandrleutin@gmail.com',
    description='RG3 is a C/C++ analyzer framework',
    long_description='RG3 is a C/C++ analyzer framework',
    packages=['rg3py'],
    package_data={'rg3py': [f'rg3py.{platform_ext}', 'rg3py.pyi']},
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: Implementation :: CPython',
    ],
    python_requires='>=3.10',
    platforms=['Windows', 'Linux'],
)
