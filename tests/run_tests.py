"""Optional integration checks: python tests/run_tests.py <executable> [args...]."""
import pathlib
import re
import subprocess
import sys
import contextlib
import os
import shutil
import uuid

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMAND = sys.argv[1:]
CHECKS = 0
if not COMMAND:
    raise SystemExit("Usage: python tests/run_tests.py <absolute executable> [args...]")


def run(folder, text):
    result = subprocess.run(COMMAND, input=text, text=True, capture_output=True,
                            cwd=folder, timeout=20)
    return result.returncode, result.stdout + result.stderr


def check(condition, label):
    global CHECKS
    if not condition:
        raise AssertionError(label)
    CHECKS += 1
    print("PASS:", label)


def rows(output):
    return [int(match) for match in re.findall(r"(?m)^(\d+)\s+\|", output)]


(ROOT / "work").mkdir(exist_ok=True)
@contextlib.contextmanager
def fixture():
    path = ROOT / "work" / ("integration-" + uuid.uuid4().hex)
    path.mkdir()
    try:
        yield path
    finally:
        resolved = path.resolve()
        if resolved.parent != (ROOT / "work").resolve() or not resolved.name.startswith("integration-"):
            raise RuntimeError("Refusing cleanup outside test fixtures")
        shutil.rmtree(resolved)


with fixture() as temp:
    folder = pathlib.Path(temp)
    data = folder / "data" / "trees.csv"
    code, output = run(folder, "4\n7\n0\n")
    check(code == 0 and "Total trees: 0" in output and not data.exists(),
          "Missing directory/file and empty startup")
    code, output = run(folder, (ROOT / "tests/test_input.txt").read_text())
    check(code == 0 and output.count("Tree added.") == 5, "Add five records")
    check("Duplicate ID" in output, "Reject duplicate ID")
    check(output.count("Invalid input.") == 5 and
          output.count("Invalid diameter.") == 4 and "Invalid text." in output,
          "Retry invalid IDs, diameters, health and empty species")
    check("Healthy: 2 (40.00%)" in output and "Average: 2 (40.00%)" in output and
          "Poor: 1 (20.00%)" in output and "Average diameter: 29.00 cm" in output,
          "Health counts, percentages and average")
    check([int(line.split(',')[0]) for line in data.read_text().splitlines()] ==
          [3, 1, 5, 2, 4], "Descending sort persisted")
    code, output = run(folder, "4\n0\n")
    check(code == 0 and "Loaded 5 tree(s)." in output and rows(output) == [3, 1, 5, 2, 4],
          "Reload saved records and list order")
    code, output = run(folder, "5\n1\n2\n0\n0\n")
    check(code == 0 and rows(output) == [2] and "Teaching Building" in output,
          "Exact ID search preserves full record after sort")
    code, output = run(folder, "5\n2\nGinkgo\n0\n0\n")
    check(code == 0 and rows(output) == [3, 1], "Species search returns all matches")
    code, output = run(folder, "5\n1\n999\n2\nginkgo\n0\n0\n")
    check(code == 0 and output.count("No matching trees.") == 2, "No result and case sensitivity")
    code, output = run(folder, "7\n0\n")
    check(code == 0 and rows(output) == [3] and "East Gate" in output, "Largest tree")
    with data.open("a") as file:
        file.write("broken record\n6,Oak,West Gate,20,1\n")
    code, output = run(folder, "0\n")
    check(code == 0 and "invalid record" in output and "Loaded 6 tree(s)." in output,
          "Skip damaged row and continue loading")
    data.write_text("1,Ginkgo,Library,32.5,1\n1,Oak,Gate,20,2\n" +
                    "2,Oak,Gate,nan,1\n3,,Gate,12,1\n4,Oak,Gate,12,4\n" +
                    "x" * 700 + "\n5,Oak,Gate,12,1,extra\n6,Maple,Gate,30,3\n")
    code, output = run(folder, "0\n")
    check(code == 0 and "Loaded 2 tree(s)." in output and
          output.count("invalid record") == 5 and "duplicate ID" in output,
          "Reject invalid CSV fields, duplicate IDs and overlong rows")
    data.write_text("")
    text = ("9" * 600 + "\n1\n99999999999999999999\n7\n" + "a" * 300 +
            "\nOak,Tree\nOak\n\nGate\ninf\n1e999\n12junk\n20\nx\n1\n0\n")
    code, output = run(folder, text)
    check(code == 0 and output.count("Tree added.") == 1 and
          data.read_text().strip() == "7,Oak,Gate,20,1",
          "Long input is drained; overflow, commas and trailing junk rejected")
    code, output = run(folder, "1\n8\nUnfinished\n")
    check(code == 0 and "Incomplete tree was not added" in output and
          len(data.read_text().splitlines()) == 1, "EOF cancels partial add without rewriting")
    data.write_text("".join(f"{i},Oak,Gate,{i},1\n" for i in range(1, 102)))
    code, output = run(folder, "1\n8\n0\n")
    check(code == 0 and "capacity limit" in output and "Storage full" in output and
          len(data.read_text().splitlines()) == 100, "CSV and interactive capacity limit")
    data.write_text("1,Oak,Gate,20,1\n2,Maple,Library,20,2\n")
    code, output = run(folder, "6\n7\n0\n")
    check(code == 0 and rows(output) == [1, 2, 1, 2], "Stable equal-diameter sort and all maxima")
    data.unlink()
    data.mkdir()
    code, output = run(folder, "0\n")
    check(code != 0 and data.is_dir() and "Startup failed" in output,
          "File-open failure exits without overwriting data")
    data.rmdir()
    data.parent.rmdir()
    data.parent.write_text("blocks directory creation")
    code, output = run(folder, "0\n")
    check(code != 0, "Invalid data directory handled")


SEED = "1,Oak,Gate,20,1\n2,Maple,Library,30,2\n3,Pine,Garden,10,3\n"


def scenario(text, expected, label, seed=SEED, unchanged=False, message=None):
    with fixture() as folder:
        data = folder / "data" / "trees.csv"
        data.parent.mkdir()
        data.write_text(seed)
        # A sentinel timestamp detects replacement even when bytes are identical.
        os.utime(data, ns=(1000000000000000000, 1000000000000000000))
        timestamp = data.stat().st_mtime_ns
        original = data.read_bytes()
        code, output = run(folder, text)
        good = code == 0 and data.read_text() == expected
        if unchanged:
            good = good and data.read_bytes() == original and data.stat().st_mtime_ns == timestamp
        if message:
            good = good and message in output
        check(good and not data.with_suffix(".csv.tmp").exists(), label)
        return output


scenario("2\n999\n0\n", SEED, "Edit nonexistent ID", unchanged=True, message="No matching")
scenario("2\n1\nBirch\n\n\n\ny\n0\n", SEED.replace("Oak", "Birch"), "Edit species")
scenario("2\n1\n\nNorth Gate\n\n\ny\n0\n", SEED.replace("Oak,Gate", "Oak,North Gate"), "Edit location")
scenario("2\n1\n\n\n42.5\n3\ny\n0\n", SEED.replace("Gate,20,1", "Gate,42.5,3"), "Edit diameter and health")
scenario("2\n1\n\n\n\n\n0\n", SEED, "Blank fields retain original without dirty", unchanged=True)
scenario("2\n1\nBirch\n", SEED, "EOF during edit rolls back all fields", unchanged=True)
scenario("2\n1\nOak\nGate\n20\n1\n0\n", SEED, "Identical edit does not rewrite", unchanged=True)
scenario("2\n1\nBirch\n:cancel\n0\n", SEED, "Explicit cancel rolls back staged edit", unchanged=True)
scenario("2\n1\nBirch\n\n\n\nn\n0\n", SEED, "Reject final edit confirmation", unchanged=True)
scenario("2\n1\nBirch\n\n\n\n", SEED, "EOF at final confirmation rolls back", unchanged=True)
scenario("3\n999\n0\n", SEED, "Delete nonexistent ID", unchanged=True)
scenario("3\n1\nn\n0\n", SEED, "Delete n cancels", unchanged=True)
scenario("3\n1\ny\n0\n", SEED.split("\n", 1)[1], "Delete first preserves remaining order")
scenario("3\n2\nY\n0\n", SEED.replace("2,Maple,Library,30,2\n", ""), "Delete middle preserves order (uppercase Y)")
scenario("3\n3\ny\n0\n", SEED.replace("3,Pine,Garden,10,3\n", ""), "Delete last preserves order")
scenario("3\n1\ny\n3\n1\n3\n2\ny\n3\n3\ny\n3\n2\n0\n", "", "Delete to empty; repeated delete and empty edit safe")
scenario("4\n5\n1\n1\n0\n7\n0\n", SEED, "Read-only exit preserves bytes and timestamp", unchanged=True)
scenario("4\n", SEED, "Read-only EOF preserves bytes and timestamp", unchanged=True)
out = scenario("2\n1\nBirch\n\n\n\ny\n8\n0\n", SEED.replace("Oak", "Birch"), "Edit and manual save persist")
check(out.count("Saved 3 tree(s).") == 1 and "Saving before exit" not in out,
      "Manual save clears dirty; exit does not save again")
out = scenario("2\n1\nBirch\n\n\n\ny\n8\n3\n3\ny\n0\n",
               SEED.replace("Oak", "Birch").replace("3,Pine,Garden,10,3\n", ""),
               "Further change after manual save becomes dirty again")
check(out.count("Saved ") == 2, "Manual save then later change saves twice")
scenario("4\n0\n", SEED + "broken record\n", "Viewing damaged CSV retains skipped rows",
         seed=SEED + "broken record\n", unchanged=True)
scenario("6\n0\n", "2,Maple,Library,30,2\n1,Oak,Gate,20,1\n3,Pine,Garden,10,3\n",
         "Actual sort marks dirty and persists")
scenario("6\n0\n", "1,Oak,Gate,20,1\n2,Maple,Library,20,2\n",
         "Equal diameter sort does not mark dirty", seed="1,Oak,Gate,20,1\n2,Maple,Library,20,2\n", unchanged=True)
scenario("6\n0\n", "2,Maple,Library,30,2\n1,Oak,Gate,20,1\n",
         "Already sorted data does not mark dirty", seed="2,Maple,Library,30,2\n1,Oak,Gate,20,1\n", unchanged=True)
for answer in ("", "yes", " y", "Y ", "y" * 600):
    scenario("3\n1\n" + answer + "\n0\n", SEED, "Only exact y/Y deletes: " + repr(answer[:8]), unchanged=True)
scenario("3\n1\n", SEED, "EOF cancels delete", unchanged=True)
scenario("2\n1\n" + "x" * 600 + "\nBad,Name\nBirch\n\n0\nnan\ninf\n25junk\n25\n4\nx\n2\ny\n0\n",
         SEED.replace("1,Oak,Gate,20,1", "1,Birch,Gate,25,2"), "Edit reuses text and numeric validation")
scenario("3\n2\ny\n1\n2\nAsh\nField\n15\n1\n0\n",
         SEED.replace("2,Maple,Library,30,2\n", "") + "2,Ash,Field,15,1\n", "Deleted ID can be reused uniquely")
scenario("2\n0\n3\n0\n0\n", SEED, "Zero ID cancels mutations", unchanged=True)

with fixture() as folder:
    data = folder / "data" / "trees.csv"
    data.parent.mkdir()
    data.write_text(SEED)
    temporary = data.with_suffix(".csv.tmp")
    temporary.write_text("unowned temporary contents")
    code, output = run(folder, "3\n1\ny\n8\n0\n")
    check(code != 0 and data.read_text() == SEED and temporary.read_text() == "unowned temporary contents",
          "Occupied temporary path preserves old CSV and unowned temporary")
    check(output.count("Cannot create temporary data file") == 2,
          "Failed manual save keeps dirty and retries on exit")
    temporary.unlink()
    code, output = run(folder, "4\n0\n")
    check(code == 0 and rows(output) == [1, 2, 3], "Old CSV reloads after save failure")

with fixture() as folder:
    data = folder / "data" / "trees.csv"
    data.parent.mkdir()
    data.write_text(SEED)
    code, output = run(folder, "2\n1\nBirch\n\n\n\ny\n0\n")
    check(code == 0 and not data.with_suffix(".csv.tmp").exists(), "Successful save leaves no temporary file")
    code, output = run(folder, "4\n0\n")
    check(code == 0 and "Birch" in output and rows(output) == [1, 2, 3], "Edited record reloads in a new process")

if sys.platform == "win32":
    # Real OS replacement failure: permit reads/writes but deny deletion of old file.
    import ctypes
    from ctypes import wintypes
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                  ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
    kernel.CreateFileW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    with fixture() as folder:
        data = folder / "data" / "trees.csv"
        data.parent.mkdir()
        data.write_text(SEED)
        handle = kernel.CreateFileW(str(data), 0x80000000, 3, None, 3, 0, None)
        if handle == ctypes.c_void_p(-1).value:
            raise ctypes.WinError(ctypes.get_last_error())
        try:
            code, output = run(folder, "3\n1\ny\n0\n")
            check(code != 0 and "Cannot replace data file" in output and data.read_text() == SEED,
                  "Real Windows replacement failure preserves original CSV")
            check(not data.with_suffix(".csv.tmp").exists(), "Failed replacement removes owned temporary file")
        finally:
            kernel.CloseHandle(handle)
else:
    print("SKIP: 2 Windows file-sharing checks (Windows only)")

print(f"All {CHECKS} integration checks passed.")

