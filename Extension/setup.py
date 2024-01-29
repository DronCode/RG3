from setuptools import setup
from typing import List
from dataclasses import dataclass


@dataclass
class PlatformTargetDesc:
    dest_folder: str
    native_ext: str


platforms: List[PlatformTargetDesc] = [PlatformTargetDesc(dest_folder='windows_dist', native_ext='.pyd'),
                                       PlatformTargetDesc(dest_folder='linux_dist', native_ext='.so')]

platform_desc: PlatformTargetDesc
for platform_desc in platforms:
    setup(
        name='rg3py',
        version='0.1',
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
