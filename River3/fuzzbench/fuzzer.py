import os
import shutil
import subprocess

from fuzzers import utils

def build():
    # Environment variables
    os.environ['CC'] = 'clang'    # C compiler.
    os.environ['CXX'] = 'clang++' # C++ compiler.
    os.environ['FUZZER_LIB'] = '/River3/empty_lib.o' # Empty fuzz library

    utils.build_benchmark()
    shutil.copytree('/River3', os.environ['OUT'], dirs_exist_ok=True)
    print('[post_build] Finished river building process')

def fuzz(input_corpus, output_corpus, target_binary):
    return
    """Run fuzzer."""
    # Seperate out corpus and crash directories as sub-directories of
    # |output_corpus| to avoid conflicts when corpus directory is reloaded.
    crashes_dir = os.path.join(output_corpus, 'crashes')
    output_corpus = os.path.join(output_corpus, 'corpus')
    os.makedirs(crashes_dir)
    os.makedirs(output_corpus)

    print('[fuzz] Running target with river')
    command = [
        'python3', 'concolic_GenerationalSearch2.py',
        '--binaryPath', target_binary,
        '--architecture', 'x64',
        '--maxLen', '10',
        '--logLevel', 'CRITICAL',
        '--secondsBetweenStats', '10',
        '--outputType', 'textual',
        '--input', input_corpus,
        '--output', output_corpus,
        '--crashdir', crashes_dir,
    ]
    dictionary_path = utils.get_dictionary_path(target_binary)
    if dictionary_path:
        command.extend(['--dict', dictionary_path])
    command.extend(['--', target_binary])

    print('[fuzz] Running command: ' + ' '.join(command))
    # subprocess.check_call(command)
