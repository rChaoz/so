import os
import subprocess

grade = 0


def check(testname):
    proc_serial_res = subprocess.run(
        ["./serial", testname], capture_output=True, text=True
    ).stdout.strip("\n")
    for i in range(0, 100):
        proc_parallel_res = subprocess.run(
            ["./parallel", testname], capture_output=True, text=True
        ).stdout.strip("\n")
        if proc_serial_res != proc_parallel_res:
            return False

    return True


lst = os.listdir("tests")
lst.sort(key=lambda s: (len(s), s))
for filename in lst:
    f = os.path.join("tests", filename)
    if check(f):
        print("Test: " + f + " ........................ passed (5/5)")
        grade += 5
    else:
        print("Test: " + f + " ........................ failed (0/5)")

print("Total: " + str(grade))
