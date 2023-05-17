# OS_HW4
* Seongbin Kim 22100113
* 
* 
# Instructions
### Build
```
$ make
gcc -o findeq findeq.c
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