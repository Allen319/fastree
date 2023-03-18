# A Root-based SiPM signal extractor and fitter
## Usage
```
git clone https://github.com/Allen319/fastree.git
cd fastree
. env_lcg.sh
mkdir build
cd build
cmake ..
make -j$(nproc)
cd -
```
## A simple test
The binary file is currently named as `test`. 
- `-c` option is followed with the path of the configuration YAML file (This one is mandatory).
- `-o` option should be followed with the name (or path and name) of the output file.
- `-i` option should specify the path of the CSV (comma-separated values) file, in which each line represents the information of a single SiPM test.
- `-L` option is a simplified version of option `-i` and it asks for a single string "\*,\*,\*" just like a line in the CSV file.
- Either use `-i` or `-L` option in the test, otherwise the program will be confused.
```
./bin/test -c config/config.yaml -L "11683,50V,/junofs/users/qumanhao/Ref_SiPM_PDE1/2022_9_30_145541-11683-50V/ch0.txt"
```
Or use
```
./bin/test -c config/config.yaml -i datasets.csv"
```
