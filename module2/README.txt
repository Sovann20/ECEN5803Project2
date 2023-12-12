#README

## How to Build
Run make to build convert program. 


## How to Run
The program looks for file extensions, so if your input file file has a .wav extension, it assumes
there is a wav header and parses it. If your output file has a .wav extension, it will generate a 
wav header. Directions to use the program, it expects 3 arguments --

	Argument 1: path to input file (.wav or .PCM)
	Argument 2: path to output file (.wav or .PCM)
	Argument 3:
		1 - u-Law encode
		2 - u-Law decode
		3 - A-Law encode
		4 - A-Law decode

Some examples:

 * Generate a ulaw wav file from a PCM wav file: ./convert input_file.wav output_file.wav 1 

 * Generate a raw ulaw encoded file from a PCM wav file: ./convert input_file.wav output_file 1 

There are many combinations and they should all work. Included in the directory is CantinaBand3.wav
which is the file that I used during my testing.
