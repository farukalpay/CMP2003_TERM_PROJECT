#!/usr/bin/env python3
"""Check protected course code; optionally prepare the single-file submission."""
from pathlib import Path
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PROTECTED = {
    'interface': 'b4e63af363fa50ea49d1ab089b0f51a407aadfcfdff60dee2c2ece490dbb661b',
    'factory': '11f80ce5c2fa71354f46b167b68ce7a798404e6722eb775f76f410e75396b13a',
    'engine': 'c96d55518cc1867993aa9e69b739ec3e670e39c21fefb50899cba8402c970b19',
}


def check():
    source = (ROOT / 'findfast.cpp').read_bytes()
    text = source.decode().replace("\r\n", "\n")
    begin, end = '// BEGIN STUDENT IMPLEMENTATION', '// END STUDENT IMPLEMENTATION'
    if text.count(begin) != 1 or text.count(end) != 1:
        raise RuntimeError('Implementation markers changed')
    prefix, student = text.split(begin, 1)
    student, suffix = student.split(end, 1)
    implementation, factory = student.split('\nIndexPtr make_index', 1)
    for label, part in [('interface', prefix), ('factory', factory), ('engine', suffix)]:
        if hashlib.sha256(part.encode()).hexdigest() != PROTECTED[label]:
            raise RuntimeError('Protected course code changed: ' + label)
    code = re.sub(r'//[^\n]*|/\*.*?\*/', '', implementation, flags=re.S)
    banned = r'\b(?:vector|map|unordered_map|set|list|deque|stack|queue)\s*<|\b(?:sort|binary_search|lower_bound|upper_bound)\s*\(|\bstd::find\s*\('
    if re.search(banned, code):
        raise RuntimeError('Library container or search/sort in student implementation')
    print('PASS protected interface, make_index, engine, tests and main')
    return source


def run(command):
    result = subprocess.run(command, text=True, capture_output=True, timeout=180)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def prepare(source):
    compiler = shlex.split(os.environ.get('CXX', 'c++'))
    checks = {}
    with tempfile.TemporaryDirectory(prefix='findfast-submit-') as temporary:
        directory = Path(temporary)
        file = directory / 'findfast.cpp'
        file.write_bytes(source)
        for label, flags in [('release', ['-O3', '-DNDEBUG']),
                             ('sanitizer', ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])]:
            binary = directory / label
            run([*compiler, '-std=c++17', *flags, str(file), '-o', str(binary)])
            checks[label] = run([str(binary), '--test', 'all', '--scale', '10000'])
            if 'SUMMARY 20/20' not in checks[label]:
                raise RuntimeError('Course tests failed: ' + label)
        checks['csv'] = run([str(directory / 'release'), '--bench', '--workload', str(ROOT / 'data/small.csv')])
    if (ROOT / 'findfast.cpp').read_bytes() != source:
        raise RuntimeError('Source changed during verification')
    output = ROOT / 'build/submission'
    output.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256(source).hexdigest()
    (output / 'findfast.cpp').write_bytes(source)
    (output / 'SHA256SUMS').write_text(digest + '  findfast.cpp\n')
    (output / 'verification.json').write_text(json.dumps({'sha256': digest, 'checks': checks}, indent=2) + '\n')
    print('PASS standalone release 20/20, sanitizer 20/20 and CSV')
    print('Prepared build/submission/findfast.cpp; SHA-256:', digest)


if __name__ == '__main__':
    try:
        if sys.argv[1:] not in ([], ['--submission']):
            raise RuntimeError('Use check.py [--submission]')
        source = check()
        if sys.argv[1:]:
            prepare(source)
    except (RuntimeError, ValueError, subprocess.TimeoutExpired) as error:
        raise SystemExit(str(error))
