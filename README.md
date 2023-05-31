# OS_HW4
* Seongbin Kim 22100113
* 
* 
# Instructions
### Build
```
$ make
gcc -pthread -c findeq.c -o findeq.o
gcc -pthread findeq.o -o findeq
```
#### Debug
```
$ make -B DEBUG=1
gcc -pthread -DDEBUG -c findeq.c -o findeq.o
gcc -pthread -DDEBUG findeq.o -o findeq
```
### Run
```
$ findeq <option> dir
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
