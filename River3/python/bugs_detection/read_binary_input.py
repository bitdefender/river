#!/usr/bin/env python3
import sys
from pathlib import Path


def read_binary_input(binary_input_dir):
    for path in Path(binary_input_dir).glob('*'):
        with open('./' + str(path), 'rb') as f:
            x = f.read()
            print(path, ':', x)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: %s %s" % (sys.argv[0], "directory_path"))
        exit(-1)

    binary_input_dir = sys.argv[1]
    read_binary_input(binary_input_dir)
