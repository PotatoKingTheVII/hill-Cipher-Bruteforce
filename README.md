## Summary
Very quick and dirty Python implementation of Alex Barter's method outlined at https://alexbarter.com/cryptanalysis/breaking-hill-cipher/ full credit to him for the cryptanalysis

The code is generalised for any NxN matrix but instead of optimizing I slapped multithreading in and called it a day so 4x4 is the practical limit. User inputs are found at the bottom of the file and it'll output the most likely matrix row values and their Chi-squared score (Note these will give the ciphertext after encoding not decoding, to get the original matrix back you can invert it) and then try all permutations of the top N values and output trial plaintexts for each with their corresponding matrix
