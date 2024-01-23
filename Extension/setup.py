from setuptools import setup

setup(
    name='rg3py',
    version='0.1',
    author='DronCode',
    author_email='alexandrleutin@gmail.com',
    description='RG3 is a C/C++ analyzer framework',
    long_description='RG3 is a C/C++ analyzer framework',
    packages=['rg3py'],
    package_data={'rg3py': ['rg3py.pyd', 'rg3py.pyi']},
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: Microsoft :: Windows',
        'Programming Language :: Python :: Implementation :: CPython',
    ],
    python_requires='>=3.10',
    platforms=['Windows'],
)
