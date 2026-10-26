"""
Usage (run it from the project root):
    python3 scripts/plot_bench.py pthread=bench_spawnJoin.csv -o costo_per_thread
    python3 scripts/plot_bench.py cache_40MiB=bench_spawnJoin_dense.csv cache_1GiB=bench_spawnJoin_dense_bigcache.csv cache_0=bench_spawnJoin_dense_nocache.csv -o costo_per_thread_cache

Every argument is LABEL=FILE.csv, where FILE.csv is looked up in benchmarks/results/.
"""


# Read the command-line arguments (pthread=bench_spawnJoin.csv -o costo_per_thread).
import argparse
import csv
import re
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import NullLocator, ScalarFormatter


# Folders are relative to the current directory: run the script from the project root.
RESULTS_DIR = Path("benchmarks/results")    # The CSV files are read from here.
FIGURES_DIR = Path("docs/images/spawnJoin") # The plot (pdf and png) is written here.
TABLES_DIR = Path("docs/tables")            # The LaTeX tables are written here.

TITLE      = "Ciclo Create-join dei thread"
SUBTITLE   = "Esecuzione su singolo core"
LABEL_N    = "Numero dei thread"
LABEL_COST = "µs/thread"





# Pattern of a row name, e.g. "BM_pthread_spawnJoinBatch/8/real_time_median":
#   .*/                 any text up to the last "/" before the number
#   (\d+)               N, the number of threads (8): first group
#   /real_time_         fixed text
#   (median|stddev|cv)  the statistic: second group
#   $                   the name ends here (so "real_time_mean" does not match)
ROW_NAME = re.compile(r".*/(\d+)/real_time_(median|stddev|cv)$")






# Read one benchmark CSV file and return {N: {"median": us, "stddev": us, "cv": fraction}}.
# "path" is the path of the CSV file.
#
# Steps:
#  1. Read the whole file as text and split it into a list of lines.
#  2. find the index of the first line that starts with "name, which is the CSV header.
#  3. Create the empty dictionary that will be returned.
#  4. Read the CSV from the header on; every row becomes a dictionary (row["name"], row["real_time"]).
#  5. Check whether the row name looks like ".../8/real_time_median"; 
#  6. From the name take N (as a number) and the statistic ("median", "stddev" or "cv").
#  7. If this N is not in the dictionary yet, create an empty entry for it (each N has three rows).
#  8. Save the value of the row: times are in us (Unit(kMicrosecond)), the cv is a fraction.
#  9. Return the dictionary sorted by N.
def load(path):
    lines = Path(path).read_text().splitlines()
    start = next(i for i, line in enumerate(lines) if line.startswith("name,"))
    data = {}
    for row in csv.DictReader(lines[start:]):
        match = ROW_NAME.match(row["name"])
        if not match:
            continue
        n, stat = int(match[1]), match[2]
        data[n] = data.get(n, {})
        data[n][stat] = float(row["real_time"])
    return dict(sorted(data.items()))






# Draw the cost per thread (median batch time / N), one curve per label, and save it as out.pdf and out.png.
# The error bars are +/- 1 standard deviation (also divided by N).
#
# Steps:
#  1. Create the figure and its axes.
#  2. For every series draw one curve: x = N, y = median / N, error bar = stddev / N.
#  3. Take the N values of the first series: they decide the ticks of the x axis.
#  4. If N is 1, 2, 3, ... (steps of 1) use a linear x axis; otherwise (1, 2, 4, 8, ...)
#     use a log2 axis, so the ticks are equally spaced in both cases.
#  5. Put one tick on the x axis for every measured N.
#  6. Write the ticks as plain numbers (1024, not 2^10) and remove the small ticks between them.
#  7. Make the tick labels smaller so they do not overlap.
#  8. Start the y axis from zero, so the differences are not exaggerated.
#  9. Write the axis labels, the title and the gray subtitle under the title.
# 10. Draw a light grid.
# 11. Show the legend only when there is more than one curve.
# 12. Adjust the margins so nothing is cut off.
# 13. Save the figure twice: pdf (vector, for the final thesis) and png (for Google Docs).
def plot_cost_per_thread(series, out):
    fig, ax = plt.subplots(figsize=(6, 4))
    for label, data in series.items():
        ns = list(data)
        ax.errorbar(ns, [data[n]["median"] / n for n in ns], yerr=[data[n]["stddev"] / n for n in ns],
                    marker="o", ms=4, capsize=2, lw=1.2, label=label)
 
    ns = list(next(iter(series.values())))
    if all(b - a == 1 for a, b in zip(ns, ns[1:])):
        ax.set_xlim(ns[0] - 0.5, ns[-1] + 0.5)
    else:
        ax.set_xscale("log", base=2)
    ax.set_xticks(ns)
    ax.xaxis.set_major_formatter(ScalarFormatter())
    ax.xaxis.set_minor_locator(NullLocator())
    ax.tick_params(axis="x", labelsize=8)
 
    ax.set_ylim(bottom=0)
    ax.set_xlabel(LABEL_N)
    ax.set_ylabel(LABEL_COST)
    ax.set_title(TITLE, pad=18)
    ax.text(0.5, 1.02, SUBTITLE, transform=ax.transAxes, ha="center", va="bottom", fontsize=9, color="gray")
    ax.grid(True, alpha=0.3)
    if len(series) > 1:
        ax.legend()
 
    fig.tight_layout()
    for ext in ("pdf", "png"):
        fig.savefig(f"{out}.{ext}", dpi=200)
 
 


# Format a number for LaTeX: decimal point, thin space between thousands.
# "x" is the number, "decimals" is how many digits to keep after the point.
#
# Steps:
#  1. Write the number with a comma between thousands and the requested decimals (37181 -> "37,181").
#  2. Replace every comma with \, (the LaTeX thin space), so it becomes "37\,181".
def fmt(x, decimals):
    return f"{x:,.{decimals}f}".replace(",", r"\,")
 


 
# Write the LaTeX table of one CSV file to docs/tables/<name>.tex.
# One row for every N, with four columns: N, cost per thread [us], throughput [thread/s] and cv [%].
# "name" is the CSV file name without extension, "data" is the dictionary returned by load().
#
# Steps:
#  1. Open docs/tables/<name>.tex for writing ("w"); "with" closes the file automatically at the end.
#  2. Start the LaTeX table with 4 columns aligned to the right (rrrr); \toprule draws the top line.
#  3. Write the header row: & separates the columns, \\ ends the row, \midrule draws a line below it.
#  4. Repeat for every N in the data: n is the number of threads, d holds its median, stddev and cv.
#  5. Cost per thread in us: median time of the whole batch divided by the number of threads.
#  6. Write one table row: N, cost per thread (1 decimal), throughput (no decimals), cv in percent (2 decimals).
#     1e6 us = 1 second, so 1e6 / cost = threads per second; the cv is a fraction, so it is multiplied by 100.
#  7. Draw the bottom line and close the table.
#
# In the code every backslash is doubled (\\begin gives \begin in the file, \\\\ gives \\),
# because Python reads a single backslash as a special character.
def write_table(name, data):
    with open(TABLES_DIR / f"{name}.tex", "w") as f:
        f.write("\\begin{tabular}{rrrr}\n\\toprule\n")
        f.write("$N$ & \\textmu s/thread & Throughput [thread/s] & cv [\\%] \\\\\n\\midrule\n")
        for n, d in data.items():
            per_thread = d["median"] / n
            f.write(f"{n} & {fmt(per_thread, 1)} & {fmt(1e6 / per_thread, 0)} & {fmt(d['cv'] * 100, 2)} \\\\\n")
        f.write("\\bottomrule\n\\end{tabular}\n")
 
 


# Read the command-line arguments, load every CSV, write its LaTeX table and draw the plot.
#
# Steps:
#  1. Define the accepted arguments: one or more LABEL=FILE.csv and the required -o with the plot name.
#  2. Read the arguments typed in the terminal.
#  3. Create the dictionary {label: data} that will hold one entry per CSV file.
#  4. For every LABEL=FILE.csv: split it at the first "=" into label and file name.
#  5. Load the CSV from benchmarks/results and store it under its label.
#  6. Write the LaTeX table of that CSV (named like the CSV, without ".csv").
#  7. Draw all the curves in one plot and save it in docs/images/spawnJoin/<name given with -o>.
def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("inputs", nargs="+", metavar="LABEL=FILE.csv")
    ap.add_argument("-o", "--output", required=True, help="file name of the plot, without extension")
    args = ap.parse_args()
 
    series = {}
    for item in args.inputs:
        label, file = item.split("=", 1)
        series[label] = load(RESULTS_DIR / file)
        write_table(Path(file).stem, series[label])
 
    plot_cost_per_thread(series, FIGURES_DIR / args.output)
 
 



if __name__ == "__main__":
    main()
