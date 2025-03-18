#!/usr/bin/env python3
# /// script
# requires-python = ">=3.13"
# dependencies = [
#     "polib",
#     "rich",
# ]
# ///

"""
Checks correctness of translation PO files.
"""

import argparse
import polib
import rich
import subprocess
import sys
import glob


def _matching_po_entry(po, lineno):
    """
    Find the PO entry that matches the given line number.
    Note that entries are multiline and lineno may be in the middle of an entry.
    """
    last = None
    for entry in po:
        if entry.linenum > lineno:
            break
        last = entry
    return last


def _print_error(fn, entry, text, github):
    """
    Print the error message for the given entry.
    """
    if github:
        if entry:
            details = text + '\n' + str(entry)
            details = details.replace('\n', '%0A')
            print(f'::error title={text}::{details}')
        else:
            print(f'::error title=error in {fn}:{text}')
    else:
        rich.print(f'[red]{text}[/red]')
        if entry:
            rich.print(entry)


def process_po(filename, stderr, github, remove_errors):
    """
    Parse and pretty-print errors in the file.
    The error format is: <file>:<line>: <error>
    """
    po = polib.pofile(filename)
    errors = []
    for line in stderr.split('\n'):
        if not line:
            continue
        parts = line.split(':')
        fn = parts[0]
        if fn != filename:
            continue
        lineno = int(parts[1])
        entry = _matching_po_entry(po, lineno)
        _print_error(filename, entry, line, github)
        if entry:
            errors.append(entry)
    if remove_errors and errors:
        for entry in errors:
            if entry.msgid: # don't remove header
                po.remove(entry)
        po.save(filename)


def check_translations(po_files, github=False, remove_errors=False):
    status = True
    for po_file in po_files:
        result = subprocess.run(['msgfmt', '-v', '-c', '-o', '/dev/null', po_file], capture_output=True, text=True)
        if result.returncode != 0:
            status = False
            process_po(po_file, result.stderr, github, remove_errors)
    return status


def main():
    parser = argparse.ArgumentParser(description='Check correctness of translation PO files.')
    parser.add_argument('--github', action='store_true', help='Format output for GitHub Actions')
    parser.add_argument('--remove-errors', action='store_true', help='Remove entries with errors')
    args = parser.parse_args()

    po_files = glob.glob('locales/*.po')
    status = check_translations(po_files, args.github, args.remove_errors)
    if not status:
        sys.exit(1)


if __name__ == "__main__":
    main()
