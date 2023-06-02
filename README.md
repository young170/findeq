# OS_HW4
* Seongbin Kim 22100113
* 
* 

## Introduction
This is an implementation of **findeq**.
**findeq** is a multithreaded search of files with equal data.

Using findeq, the users can count how much memory space is 
wasted by redundant files and identify chances of saving up 
storage spaces by removing redundant files.

## Instructions
### Build
```
$ make
gcc -pthread -c findeq.c -o findeq.o
gcc -pthread findeq.o -o findeq
```
* For debugging purposes 
```
$ make -B DEBUG=1
gcc -pthread -DDEBUG -c findeq.c -o findeq.o
gcc -pthread -DDEBUG findeq.o -o findeq
```
### Run
```
$ ./findeq <option> dir
```
### Options
```
options:
    -t=NUM      creates upto NUM threads addition to the main thread.
                    The given number is no more than 64.
    -n=NUM      ignores all files whose size is less than NUM bytes
                    from the search. The default value is 1024.
    -o=FILE     produces to the output to FILE. By default, the output
                    must be printed to the standard output. 
```

## Testing
### Types of files
* .zip
* .jpg
* .gif
* .pdf
* .txt
* links

## Demonstration
### YouTube
[YouTube_Link](https://www.youtube.com/watch?v=Och1-LHc7sQ)
