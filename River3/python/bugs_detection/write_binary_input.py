#!/usr/bin/env python3
import sys
from uuid import uuid4
from datetime import datetime


def get_new_filename(folder):
    # type + seconds + uuid
    return (folder + "input-"
        + str(int((datetime.now() - datetime(1970,1,1)).total_seconds())) + '-' 
        + str(uuid4())[:8]
        + '.bin')


def write_binary_input(dir_path, list_of_bytes):
    new_file_path = get_new_filename(dir_path)
    with open(new_file_path, 'wb') as f:
        f.write(bytearray(list_of_bytes))

    return new_file_path

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: {} {}' % (sys.argv[0], "dir_path", "list_of_bytes"))
        exit(-1)

    dir_path = sys.argv[1]
    list_of_bytes_str = sys.argv[2]
    list_of_bytes_str = sys.argv[2][1:-1].replace(" ", "").split(",")
    list_of_bytes = [int(nr) for nr in list_of_bytes_str]
    new_file_path = write_binary_input(dir_path, list_of_bytes)

    print("Wrote:", list_of_bytes, "in", new_file_path)

