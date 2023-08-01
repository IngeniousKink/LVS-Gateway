#!/usr/bin/env python3

import argparse
import subprocess
import sys
import os

def parse_output(line):
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument('python_path', type=str, help="Path to python")
    parser.add_argument('esptool_path', type=str, help="Path to esptool.py")
    parser.add_argument('--chip', type=str, help="Chip name")
    parser.add_argument('--port', type=str, help="Port")
    parser.add_argument('--baud', type=int, help="Baud rate")
    parser.add_argument('--flash_mode', type=str, help="Flash mode", default='dio')
    parser.add_argument('--flash_freq', type=str, help="Flash frequency", default='40m')
    parser.add_argument('--flash_size', type=str, help="Flash size", default='4MB')
    parser.add_argument('--output', '-o', type=str, help="Output file")

    args, unrecognized = parser.parse_known_args(line.split())

    binary_files = {}
    for i in range(len(unrecognized) - 1):
        if unrecognized[i].startswith('0x'):
            offset, bin_path = unrecognized[i:i+2]
            binary_files[offset] = bin_path

    print(f'DEBUG line: {line}', file=sys.stderr)
    print(f'DEBUG args: {args}', file=sys.stderr)
    print(f'DEBUG unrecognized: {unrecognized}', file=sys.stderr)
    print(f'DEBUG binary_files: {binary_files}', file=sys.stderr)

    # Extract the GitHub build number from the environment (assuming it is set)
    github_build_number = os.environ.get('GITHUB_RUN_NUMBER', '00000')
    github_owner_name, github_repo_name = os.environ.get('GITHUB_REPOSITORY', 'user/repo').split('/')

    # Generate the output command
    new_command = f'{args.python_path} ' \
                  f'{args.esptool_path}' \
                  f' --chip {args.chip} merge_bin ' \
                  f'-o web/firmware/{github_owner_name}_{github_repo_name}_b{github_build_number}_{args.chip}.bin ' \
                  f'--flash_mode {args.flash_mode} --flash_freq {args.flash_freq} --flash_size {args.flash_size} '

    # Append binary files and their respective offsets to the new command
    for offset, bin_path in binary_files.items():
        new_command += f'{offset} "{bin_path}" '

    return new_command

def generate_merge_firmware_command(pio_env=None):
    command = ["pio", "run"]
    
    if pio_env is not None:
        command.extend(["-e", pio_env])

    command.extend(["-v", "-t", "upload", "--upload-port", "/dev/null"])

    try:
        completed_process = subprocess.run(command, capture_output=True, text=True, check=False)
    except subprocess.CalledProcessError as e:
        print(f"Error running pio command: {e}", file=sys.stderr)

    if not completed_process.stdout:
        print("No output received from pio command.", file=sys.stderr)
        sys.exit(1)

    output_lines = completed_process.stdout.splitlines()
    relevant_lines = [
        line
        for line 
        in output_lines 
        if ("/esptool.py" in line) and ("write_flash" in line)
    ]

    if not relevant_lines:
        print("No relevant lines found in the output.", file=sys.stderr)
        sys.exit(1)

    for line in relevant_lines:
        new_command = parse_output(line)
        if new_command:
            print(new_command)
        else:
            sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        for pio_env in sys.argv[1:]:
            generate_merge_firmware_command(pio_env)
    else:
        generate_merge_firmware_command()
