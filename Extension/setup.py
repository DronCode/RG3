from setuptools import setup
from typing import List
from dataclasses import dataclass
from os import getenv


@dataclass
class PlatformTargetDesc:
    id: str
    dest_folder: str
    native_ext: str


target_version: str = getenv('RG3_DEPLOY_VERSION', '0.0.1')
platforms: List[PlatformTargetDesc] = [PlatformTargetDesc(id="Windows", dest_folder='RG3_Windows', native_ext='pyd'),
                                       PlatformTargetDesc(id="Linux", dest_folder='RG3_Linux', native_ext='so')]
platform_desc: PlatformTargetDesc

print(f" *** RG3 Package builder for {target_version} (0.0.1 default) *** ")

for platform_desc in platforms:
    print(f"Run for platform {platform_desc.id} (rg3py.{platform_desc.native_ext})")
    setup(
        name='rg3py',
        version=target_version,
        author='DronCode',
        author_email='alexandrleutin@gmail.com',
        description='RG3 is a C/C++ analyzer framework',
        long_description='RG3 is a C/C++ analyzer framework',
        packages=['rg3py'],
        package_data={'rg3py': [f'{platform_desc.dest_folder}/rg3py.{platform_desc.native_ext}',
                                f'{platform_desc.dest_folder}/rg3py.pyi']},
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
