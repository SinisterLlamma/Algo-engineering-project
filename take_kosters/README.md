# Algo-engineering-project


## How to run takes_kosters.cpp
``` bash
g++ -O2 -std=c++17 takes_kosters.cpp -o bounding
./bounding --strategy 2 smallworld.mtx > results.csv
```

After running your C++ code, split its output into two files:

# assuming ./bounding printed both blocks to all_results.csv
awk '/^#/ {exit} {print}' results.csv > summary.csv
awk 'f;/^#/{f=1}' results.csv > iters.csv
Install dependencies (if you don’t already have them):

``` bash
pip install pandas matplotlib
```
Run the plotting script:

``` bash
python plot_diameter_results.py --summary summary.csv --iters iters.csv
```

# generateing master CSV

Make sure your C++ binary is built and executable, e.g.:

``` bash
g++ -O2 -std=c++17 main.cpp -o bounding
```


```Bash
chmod +x generate_master_csv.py
```
Run it:
``` bash
python generate_master_csv.py --binary ./bounding --graph smallworld.mtx
```