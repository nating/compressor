# compressor
A program to compress files using Huffman Coding.

## About
This program takes a training file to learn the frequency of different characters in the language of your text files.

It uses the frequency of characters in your training file to create a huffman tree which acts as a cipher when encoding or decoding your text files.

The example files given show how the test_source.txt can be compressed with the program from 440KB to 246KB.

The training_file.txt given is a free Frankenstein ebook from http://www.gutenberg.org. The frequency of characters in this text file will make a huffman tree that will compress any English text file quite efficiently.

## Usage
To encode a file:  
`./compressor huffcode <training filename> <input filename> <output filename>`

To decode a file:  
`./compressor huffdecode <training filename> <input filename> <output filename>`
