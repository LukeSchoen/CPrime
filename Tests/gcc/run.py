"""Serial GCC C++ compatibility assessment; never executes upstream Tcl code."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
REVISION = '5f6257c26b814de1a14c71b2d3a49291765b6577'
UPSTREAM = ROOT / 'build/gcc-upstream'
TREES = ['g++.dg', 'g++.old-deja', 'c-c++-common']


def fetch():
    if not UPSTREAM.exists():
        subprocess.run(['git', 'clone', '--filter=blob:none', '--no-checkout',
                        'https://github.com/gcc-mirror/gcc.git', str(UPSTREAM)], check=True)
    subprocess.run(['git', 'sparse-checkout', 'set',
                    *['gcc/testsuite/' + t for t in TREES], 'gcc/testsuite/lib'],
                   cwd=UPSTREAM, check=True)
    subprocess.run(['git', 'fetch', '--depth=1', 'origin', REVISION], cwd=UPSTREAM, check=True)
    subprocess.run(['git', 'checkout', '--detach', REVISION], cwd=UPSTREAM, check=True)


def directives(source):
    """Extract balanced directive braces, respecting Tcl quoted strings."""
    result = []
    for match in re.finditer(r'\{\s*(dg-[\w-]+)\b', source):
        start = match.start()
        depth, quoted, escaped = 0, False, False
        for end in range(start, len(source)):
            ch = source[end]
            if escaped:
                escaped = False
                continue
            if ch == '\\':
                escaped = True
                continue
            if ch == '"':
                quoted = not quoted
            if not quoted:
                depth += (ch == '{') - (ch == '}')
                if depth == 0:
                    result.append((match[1], source[match.end():end].strip()))
                    break
        else:
            result.append(('unclosed-directive', source[start:start + 100]))
    return result


def assess(source):
    action, options, reasons = 'compile', [], []
    negative = False
    for name, args in directives(source):
        if name == 'dg-do' and args in ('compile', 'assemble', 'link', 'run', 'preprocess'):
            action = args
        elif name in ('dg-options', 'dg-additional-options'):
            m = re.fullmatch(r'"([^"\\]*)"', args)
            if not m:
                reasons.append(name + ': conditional or escaped options')
                continue
            opts = m[1].split()
            if name == 'dg-options':
                options = []
            for opt in opts:
                if opt in ('-O0', '-O1', '-O2', '-O3', '-g', '-fno-inline', '-fno-builtin'):
                    options.append(opt)
                else:
                    reasons.append('unverified option: ' + opt)
        elif name in ('dg-error', 'dg-warning', 'dg-message', 'dg-note', 'dg-bogus'):
            negative |= name == 'dg-error'
            reasons.append('diagnostic expectations not checked')
        else:
            reasons.append(name + ': ' + args)
    return action, options, sorted(set(reasons)), negative


def invoke(command, timeout):
    started = time.perf_counter()
    try:
        p = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           timeout=timeout)
        return dict(exit=p.returncode, seconds=time.perf_counter() - started,
                    output=p.stdout.decode('utf-8', errors='replace'))
    except subprocess.TimeoutExpired as e:
        return dict(exit=None, seconds=time.perf_counter() - started,
                    output=(e.stdout or b'').decode('utf-8', errors='replace'))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fetch', action='store_true')
    ap.add_argument('--compiler', type=Path, default=ROOT / 'cpc.exe')
    ap.add_argument('--select', action='append', help='Path prefix relative to gcc/testsuite; repeatable')
    ap.add_argument('--probe', action='store_true', help='Also compile unsupported tests as observations, never passes')
    ap.add_argument('--timeout', type=float, default=5)
    ap.add_argument('--limit', type=int)
    ap.add_argument('--out', type=Path, default=ROOT / 'build/gcc-results')
    args = ap.parse_args()
    if args.fetch:
        fetch()
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=UPSTREAM, text=True).strip()
    if revision != REVISION:
        ap.error('Upstream revision differs from pin; run --fetch')
    dirty = subprocess.check_output(['git', 'status', '--porcelain'], cwd=UPSTREAM, text=True)
    if dirty:
        ap.error('Upstream checkout is modified; refusing an ambiguous baseline')
    compiler = args.compiler.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    testroot = UPSTREAM / 'gcc/testsuite'
    files = sorted(p for t in TREES for p in (testroot / t).rglob('*')
                   if p.suffix in ('.C', '.cc', '.cpp', '.c'))
    if args.select:
        files = [p for p in files if any(p.relative_to(testroot).as_posix().startswith(s)
                                        for s in args.select)]
    if args.limit:
        files = files[:args.limit]
    counts = Counter()
    metadata = dict(revision=revision, compiler=str(compiler),
                    compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),
                    selected=len(files), serial=True, probe=args.probe,
                    note='CPC default language mode; no claim of GCC diagnostic or standard-mode conformance')
    (args.out / 'metadata.json').write_text(json.dumps(metadata, indent=2), encoding='utf-8')
    with (args.out / 'results.jsonl').open('w', encoding='utf-8') as log:
        for index, path in enumerate(files):
            source = path.read_text(encoding='utf-8', errors='replace')
            action, options, reasons, negative = assess(source)
            relative = path.relative_to(testroot)
            # g++.dg/dg.exp includes only the shared root and cpp directory.
            # Other shared trees are inputs to specialized GCC drivers.
            if relative.parts[0] == 'c-c++-common' and relative.parent.as_posix() not in (
                    'c-c++-common', 'c-c++-common/cpp'):
                reasons.append('shared source requires its specialized GCC driver')
            # Dedicated .exp drivers can impose additional semantics/options.
            for parent in path.parents:
                if parent == testroot:
                    break
                if parent.name not in ('g++.dg', 'g++.old-deja') and list(parent.glob('*.exp')):
                    reasons.append('special upstream driver: ' + parent.relative_to(testroot).as_posix())
            row = dict(path=path.relative_to(testroot).as_posix(), action=action,
                       reasons=reasons, expects_error=negative)
            if reasons and not args.probe:
                row['status'] = 'UNSUPPORTED'
            else:
                artifact = args.out / ('test.exe' if action in ('run', 'link') and not reasons else 'test.o')
                artifact.unlink(missing_ok=True)
                command = [str(compiler), '-x', 'c++', *options, str(path), '-o', str(artifact)]
                if action == 'preprocess' and not reasons:
                    command += ['-E']
                elif action not in ('run', 'link') or reasons:
                    command += ['-c']
                row['command'] = command
                compiled = invoke(command, args.timeout)
                row['compile'] = compiled
                rc = compiled['exit']
                if rc is None:
                    status = 'TIMEOUT'
                elif rc < 0 or rc > 255:
                    status = 'CRASH'
                elif reasons:
                    status = 'PROBE_ACCEPTED' if rc == 0 else 'PROBE_REJECTED'
                elif rc != 0:
                    status = 'FAIL_COMPILE'
                elif action != 'preprocess' and not artifact.is_file():
                    status = 'FAIL_NO_OUTPUT'
                elif action == 'run':
                    row['run'] = invoke([str(artifact.resolve())], args.timeout)
                    status = 'PASS_RUN' if row['run']['exit'] == 0 else 'FAIL_RUN'
                else:
                    status = 'PASS_' + action.upper()
                row['status'] = status
            counts[row['status']] += 1
            log.write(json.dumps(row) + '\n')
            log.flush()
            if (index + 1) % 1000 == 0:
                print(index + 1, '/', len(files), dict(counts), flush=True)
    (args.out / 'summary.json').write_text(json.dumps(dict(counts), indent=2), encoding='utf-8')
    print(json.dumps(dict(counts), indent=2), flush=True)
    return int(any(k.startswith('FAIL') or k in ('CRASH', 'TIMEOUT') for k in counts))


if __name__ == '__main__':
    raise SystemExit(main())
