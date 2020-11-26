#!/usr/bin/env python3
import datetime
import os
import time
import signal
import sys
import struct
import subprocess
from pathlib import Path
from uuid import uuid4

def get_new_filename(folder):
    # type + seconds + uuid
    return (folder + "-"
        + str(int((datetime.datetime.now() - datetime.datetime(1970,1,1)).total_seconds())) + '-' 
        + str(uuid4())[:8]
        + '.bin')

def to_int(signal):
    return -int(signal)

sig_str = {
    to_int(signal.SIGHUP): 'hup',
    to_int(signal.SIGINT): 'int',
    to_int(signal.SIGQUIT): 'quit',
    to_int(signal.SIGILL): 'ill',
    to_int(signal.SIGTRAP): 'trap',
    to_int(signal.SIGABRT): 'abrt',
    to_int(signal.SIGBUS): 'bus',
    to_int(signal.SIGFPE): 'fpe',
    to_int(signal.SIGUSR1): 'usr1',
    to_int(signal.SIGSEGV): 'segv',
    to_int(signal.SIGUSR2): 'usr2',
    to_int(signal.SIGPIPE): 'pipe',
    to_int(signal.SIGALRM): 'alrm',
    to_int(signal.SIGTERM): 'term',
    'skipped': 'skipped'
}

def save_input(result_returncode, input_bytes, cwd=os.getcwd(), get_new_filename=get_new_filename):
    # result_string = result.stdout

    if result_returncode in sig_str:
        folder = sig_str[result_returncode]
    else:
        folder = 'unclassified'

    error_folder_path = os.path.join(cwd, folder)

    # Create folder if it doesn't exist
    os.makedirs(error_folder_path, exist_ok=True)

    # Create new file and save the info
    file_name = get_new_filename(folder)
    with open(error_folder_path + "/" + file_name, 'wb') as f:
        f.write(input_bytes)

def test_input(binary_path, binary_input):
    # run program
    try:
        result = subprocess.Popen(
            binary_path,
            bufsize=-1,
            stdin=subprocess.PIPE,
            )
    except ValueError:
        # Skipping string containing \0 value
        # Probably a good idea to save it somewhere else
        # We skip it because it cannot be passed as argument
        return 'skipped'

    # Used for communicating using stdin/stdout
    print(f'Testing {binary_path} with input {binary_input}')
    result.stdin.write(binary_input)
    result.stdin.flush()
    result.stdin.close()
    result.wait()
    return result.returncode

def test_input_and_save(binary_path, binary_input, outputs_dir):
    result = test_input(binary_path, binary_input)

    # Skipping inputs containing null bytes
    if result:
        save_input(result, binary_input, cwd=outputs_dir)

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print ("Usage: {} {} {} {}".format(sys.argv[0], "path_to_binary", "inputs_dir/", "outputs_dir/"))
        exit(-1)                

    binary_file_path = sys.argv[1]
    inputs_dir = sys.argv[2]
    outputs_dir = sys.argv[3]
    
    print("Started processing binary {}, input folder: {}, containing {} files".format(
        binary_file_path, inputs_dir, len(list(Path(inputs_dir).glob('*.bin')))))
    for path in Path(inputs_dir).glob('*.bin'):
        test_input_and_save('./' + binary_file_path, open('./' + str(path), 'rb').read(), outputs_dir)
