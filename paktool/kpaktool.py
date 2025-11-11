from pakutils import PakClass
import argparse
from pathlib import Path


def get_args():
    parser = argparse.ArgumentParser(
        prog='KPakTool',
        description='KPakTool v1.0 - Python command-line tool for managing Quake PAK files',
        epilog='This program was made by Nightwolf-47 for KleleAtoms-PSP.')
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument('-x', '--extract', action='store_true', help='Extract the PAK file (input = PAK file path, output = folder to extract to)')
    modes.add_argument('-p', '--pack', action='store_true', help='Put all files from input folder to a PAK file (output = PAK file path)')
    modes.add_argument('-l', '--list', action='store_true', help='List the elements of the input PAK file (does not use output)')
    parser.add_argument('-o', '--output', help='Output path. If not provided, current path + file/folder name based on input')
    parser.add_argument('input')
    return parser.parse_args()


def get_name_from_path(path):
    path_obj = Path(path)
    return path_obj.name.rsplit('.', 1)[0]


def arg_extract(args):
    output = args.output or get_name_from_path(args.input)
    pak = PakClass()
    pak.readFile(args.input)
    pak.extract(output)


def arg_pack(args):
    output = args.output or (get_name_from_path(args.input) + ".pak")
    pak = PakClass()
    pak.import_entries(args.input)
    pak.writeFile(output)


def arg_list(args):
    pak = PakClass()
    pak.readFile(args.input)
    print(pak)


if __name__ == "__main__":
    args = get_args()
    if args.extract:
        arg_extract(args)
    elif args.pack:
        arg_pack(args)
    elif args.list:
        arg_list(args)
