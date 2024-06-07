import subprocess

# Define the dictionary with sets of filenames
files_to_sum = {
    "connector": ["SVFModule.java", "SVFJava.java", "CppReference.java", "SVFAnalysisListener.java", "connector.cpp","svfjava_interface.cpp", "NativeAnalysis.scala", "SVFConnector.scala"],
    "analysis": ["analysis.cpp"],
    "lattice": ["extendlattice.cpp"],
    "detector":["detectJNICalls.cpp",  "detector2.cpp"],
    "solver":["solver.cpp"],
    #"test":["countLOC.py", ],
}
import re
import sys
if len(sys.argv) == 1:
    print("usage: " + sys.argv[0] + " dir1 dir2 ...")
paths = sys.argv[1:]

output = ""
for p in paths:
    command = ["tokei", p, "-f"]
    result = subprocess.run(command, capture_output=True, text=True)
    output += result.stdout + "\n"
print(output)
# Step 2: Parse the output to extract LOC for each file
file_locs = {}
pattern = re.compile(r"^\s*(.+?)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)$")
lines = output.split("\n")
for line in lines:
    match = pattern.match(line)
    if match:
        filename = match.group(1).strip()
        loc = int(match.group(3))
        file_locs[filename] = loc


# Step 3: Sum the LOCs for the files specified in the dictionary
result_sums = {}
total = 0
for key, filenames in files_to_sum.items():
    total_loc = 0
    for filename in filenames:
        found = False
        for file_path, loc in file_locs.items():
            if filename in file_path:
                total_loc += loc
                found = True
        if not found:
            print("couldn't find " + filename)
            1 / 0
    result_sums[key] = total_loc
    total += total_loc

# Step 4: Output the results
for key, total_loc in result_sums.items():
    print("\\newcommand{\\svf" + key + "}{\\tnum{" + str(total_loc) + "}}")
print("\\newcommand{\\svftotal}{\\tnum{" + str(total) + "}}")