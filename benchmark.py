"""Small, disposable copy benchmark. No existing user paths are modified."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import tempfile
import time


def fingerprint(root):
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--binary', default='./bounded_copy')
    parser.add_argument('--repeats', type=int, default=7, choices=range(1, 31))
    parser.add_argument('--output', default='benchmark-results.json')
    args = parser.parse_args()
    binary = Path(args.binary).resolve(strict=True)
    results = {'platform': platform.platform(), 'python': platform.python_version(),
               'cpu_count': os.cpu_count(), 'repeats': args.repeats,
               'notes': 'Wall-clock subprocess time; generated local temporary files; alternating order; warm filesystem cache likely.',
               'workloads': {}}
    with tempfile.TemporaryDirectory(prefix='copy-lab-benchmark-') as temporary:
        root = Path(temporary)
        for name, count, size in [('small_files', 256, 4096), ('larger_files', 8, 524288)]:
            source = root / name
            source.mkdir()
            for i in range(count):
                directory = source / f'dir-{i % 16:02d}'
                directory.mkdir(exist_ok=True)
                (directory / f'file-{i:04d}.bin').write_bytes(bytes([i % 251]) * size)
            expected = fingerprint(source)
            elapsed = {'sequential': [], 'pool_4': []}
            for repetition in range(args.repeats):
                modes = [('sequential', 0), ('pool_4', 4)]
                if repetition % 2:
                    modes.reverse()
                for mode, workers in modes:
                    dest = root / f'{name}-{mode}-{repetition}'
                    started = time.perf_counter()
                    subprocess.run([str(binary), str(source), str(dest), str(workers)], check=True, capture_output=True)
                    elapsed[mode].append(round((time.perf_counter() - started) * 1000, 3))
                    if fingerprint(dest) != expected:
                        raise RuntimeError(f'Copy mismatch: {name} / {mode}')
            results['workloads'][name] = {'files': count, 'bytes_per_file': size,
                'samples_ms': elapsed,
                'median_ms': {mode: round(statistics.median(times), 3) for mode, times in elapsed.items()}}
    Path(args.output).write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
