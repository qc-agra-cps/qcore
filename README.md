# qcore

QCORE: Core Quantum Library

### Introductory Notebook
   An introduction to QCore's python interface can be found [here](introduction.ipynb)

### For use in a C++ Project

    git submodule add <<QCORE Library URL>>


### For use in Python Environment

  Download (clone) the source to a local directory and run the following commands

    1. Create a python virtual environment
        python3 -m venv <env name> 

    2. Activate the environment
        source <env name>/bin/activate

    3. Install the following packages 
        pip install cmake ninja setuptools setuptools_scm 

    4. Install the QCORE tool
        python3 setup.py install
