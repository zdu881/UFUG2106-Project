"""Linux grader-side control. Run with SOLVER CASE NEW_DIRECTORY."""
import json
import os
from pathlib import Path
import sys


def measure(command, directory, label, separate):
    peak_file = directory / (label + '.kib')
    if separate:
        command = ['/usr/bin/time', '-f', '%M', '-o', peak_file, *command]
    with (directory / (label + '.log')).open('w') as log:
        pid = os.fork()
        if pid == 0:
            os.dup2(log.fileno(), 1)
            os.dup2(log.fileno(), 2)
            try:
                os.execv(command[0], command)
            except OSError:
                os._exit(127)
        _, status, usage = os.wait4(pid, 0)
    if os.waitstatus_to_exitcode(status) != 0:
        raise RuntimeError(f'{label} failed; see its log')
    peak = int(peak_file.read_text()) if separate else usage.ru_maxrss
    return {'peak_kib': peak, 'over_64_mib': peak > 65536}


def main():
    solver, case, directory = (Path(s).resolve() for s in sys.argv[1:])
    directory.mkdir(parents=True, exist_ok=False)
    rows = []
    for loaded in (False, True):
        # Keep the source text and its tokens live in the input-loaded control.
        raw = case.read_bytes() if loaded else b''
        tokens = raw.split()
        rss = int(Path('/proc/self/statm').read_text().split()[1]) * os.sysconf('SC_PAGE_SIZE') // 1024
        for separate in (False, True):
            label = f'{"loaded" if loaded else "empty"}-{"worker" if separate else "direct"}'
            circuit = directory / (label + '.out')
            solver_peak = measure([solver, case, circuit], directory, label, separate)
            # A genuine over-limit workload independently checks the rejection path.
            heavy = [sys.executable, '-c', 'a=bytearray(80*1024*1024); a[::4096]=bytes([1])*20480']
            heavy_peak = measure(heavy, directory, label + '-80m', separate)
            rows.append(dict(input_bytes=len(raw), input_tokens=len(tokens), parent_rss_kib=rss, mode=label,
                             solver=solver_peak, over_limit_control=heavy_peak))
    (directory / 'comparison.json').write_text(json.dumps(rows, indent=2) + '\n')


if __name__ == '__main__':
    main()
