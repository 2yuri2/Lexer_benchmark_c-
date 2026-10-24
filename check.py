import subprocess, sys, re

# Works with clang output on Windows and Linux/Mac (column spacing and flag position differ)
PAT = r"^(\w+)[ \t]+'(.*?)'(?:[ \t]+\[\w+\])*[ \t]+Loc=<"

def tokens(path):
    r = subprocess.run(["clang++", "-fsyntax-only", "-Xclang", "-dump-raw-tokens", path],
                       capture_output=True, text=True)
    out = (r.stderr + r.stdout).replace("\r\n", "\n")
    found = re.findall(PAT, out, re.M | re.S)
    return [f"{t} {s}" for t, s in found if t != "unknown"]

actual = tokens(sys.argv[1])
expected = [l.strip() for l in open(sys.argv[2], encoding="utf-8").read().splitlines() if l.strip()]

if not actual:   sys.exit("Clang returned no tokens. Is the .cpp file saved and non-empty?")
if not expected: sys.exit("expected file is empty. Save your answer key first.")

ok = 0
for i in range(max(len(actual), len(expected))):
    a = actual[i] if i < len(actual) else None
    e = expected[i] if i < len(expected) else None
    if a == e: ok += 1
    else: print(f"Token {i+1}: expected {e!r} got {a!r}")
print(f"\nResults:\nCorrect: {ok}/{len(expected)} \nAccuracy: {100*ok/len(expected):.1f}%")
print()