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

def save_input(result, input_bytes, cwd=os.getcwd(), get_new_filename=get_new_filename):
    result_string = result.stdout

    folder = 'unclassified'
    if result.returncode == to_int(signal.SIGHUP): folder = 'hup'
    elif result.returncode == to_int(signal.SIGINT): folder = 'int'
    elif result.returncode == to_int(signal.SIGQUIT): folder = 'quit'
    elif result.returncode == to_int(signal.SIGILL): folder = 'ill'
    elif result.returncode == to_int(signal.SIGTRAP): folder = 'trap'
    elif result.returncode == to_int(signal.SIGABRT): folder = 'abrt'
    elif result.returncode == to_int(signal.SIGBUS): folder = 'bus'
    elif result.returncode == to_int(signal.SIGFPE): folder = 'fpe'
    elif result.returncode == to_int(signal.SIGUSR1): folder = 'usr1'
    elif result.returncode == to_int(signal.SIGSEGV): folder = 'segv'
    elif result.returncode == to_int(signal.SIGUSR2): folder = 'usr2'
    elif result.returncode == to_int(signal.SIGPIPE): folder = 'pipe'
    elif result.returncode == to_int(signal.SIGALRM): folder = 'alrm'
    elif result.returncode == to_int(signal.SIGTERM): folder = 'term'

    error_folder_path = os.path.join(cwd, folder)

    # Create folder if it doesn't exist
    os.makedirs(error_folder_path, exist_ok=True)

    # Create new file and save the info
    file_name = get_new_filename(folder)
    with open(error_folder_path + "/" + file_name, 'wb') as f:
        f.write(input_bytes)

def test_input(binary_path, binary_input, outputs_dir):
    # run program
    try:
        result = subprocess.Popen(
            [binary_path, binary_input], 
            bufsize=-1, 
            stdout=subprocess.PIPE, 
            stdin=subprocess.PIPE)
    except ValueError:
        # Skipping string containing \0 value
        # Probably a good idea to save it somewhere else
        print("Skipping:", binary_input)
        return

    # used for communicating using stdin/stdout
    # result.stdin.write(bytearray("BBBB", encoding='UTF-8'))
    # result.stdin.flush()
    # result.stdin.close()
    result.wait()

    # get stdout
    result_string = result.stdout.read()
    print("Completed:", result, 'Output:', result_string, result.returncode, result.poll())

    # save output
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
        test_input('./' + binary_file_path, open('./' + str(path), 'rb').read(), outputs_dir)
