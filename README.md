## Versions
The Python version came first while the c++ was a port to get more familiar with the language. It's around 60x faster and is the preferred version, managing a 5x5 in around 3 minutes with 5 threads. 6x6 would take over an hour so I haven't bothered optimising the memory usage for that and as such won't be possible. Command usage is given when running the program, compiled fine under C++ 14 standard on VS 19


## Summary
Very quick and dirty Python implementation of Alex Barter's method outlined at https://alexbarter.com/cryptanalysis/breaking-hill-cipher/ full credit to him for the cryptanalysis

The code is generalised for any NxN matrix but instead of optimizing I slapped multithreading in and called it a day so 4x4 is the practical limit. User inputs are found at the bottom of the file and it'll output the most likely matrix row values and their Chi-squared score (Note these will give the ciphertext after encoding not decoding, to get the original matrix back you can invert it) and then try all permutations of the top N values and output trial plaintexts for each with their corresponding matrix. *Make sure to pad ciphertext to a multiple of N*
