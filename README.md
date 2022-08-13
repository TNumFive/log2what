# log2what
### build.sh for test_and_bench.cpp
```bash
# put this script under ${repo}/build/
mkdir -p ./temp/
g++ -c ../file_writer/file_writer.cpp -o ./temp/file_writer.o &&
g++ -c ../db_writer/db_writer.cpp -o ./temp/db_writer.o &&
g++ -c ../test_and_bench/test_and_bench.cpp -o ./temp/main.o &&
g++ ./temp/main.o ./temp/file_writer.o ./temp/db_writer.o -g -l sqlite3 -lpthread
```