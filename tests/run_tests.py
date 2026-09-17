"""Optional integration checks: python tests/run_tests.py <executable> [args...]."""
import pathlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMAND = sys.argv[1:]
if not COMMAND:
    raise SystemExit("Usage: python tests/run_tests.py <absolute executable> [args...]")


def run(folder, text):
    result = subprocess.run(COMMAND, input=text, text=True, capture_output=True,
                            cwd=folder, timeout=20)
    return result.returncode, result.stdout + result.stderr


def check(condition, label):
    if not condition:
        raise AssertionError(label)
    print("PASS:", label)


def rows(output):
    return [int(match) for match in re.findall(r"(?m)^(\d+)\s+\|", output)]


(ROOT / "work").mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix="integration-", dir=ROOT / "work") as temp:
    folder = pathlib.Path(temp)
    data = folder / "data" / "trees.csv"
    code, output = run(folder, "2\n5\n0\n")
    check(code == 0 and "Total trees: 0" in output and data.exists(),
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
    code, output = run(folder, "2\n0\n")
    check(code == 0 and "Loaded 5 tree(s)." in output and rows(output) == [3, 1, 5, 2, 4],
          "Reload saved records and list order")
    code, output = run(folder, "3\n1\n2\n0\n0\n")
    check(code == 0 and rows(output) == [2] and "Teaching Building" in output,
          "Exact ID search preserves full record after sort")
    code, output = run(folder, "3\n2\nGinkgo\n0\n0\n")
    check(code == 0 and rows(output) == [3, 1], "Species search returns all matches")
    code, output = run(folder, "3\n1\n999\n2\nginkgo\n0\n0\n")
    check(code == 0 and output.count("No matching trees.") == 2, "No result and case sensitivity")
    code, output = run(folder, "5\n0\n")
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
          len(data.read_text().splitlines()) == 1, "EOF cancels partial record and saves")
    data.write_text("".join(f"{i},Oak,Gate,{i},1\n" for i in range(1, 102)))
    code, output = run(folder, "1\n0\n")
    check(code == 0 and "capacity limit" in output and "Storage full" in output and
          len(data.read_text().splitlines()) == 100, "CSV and interactive capacity limit")
    data.write_text("1,Oak,Gate,20,1\n2,Maple,Library,20,2\n")
    code, output = run(folder, "4\n5\n0\n")
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
print("All integration checks passed.")
