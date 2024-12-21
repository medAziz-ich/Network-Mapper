/*Network Mapper*/

This is a simple network scanner written in C that can be used to scan a range of IP addresses on a specified port.
Features

    Scans a range of IP addresses or a single IP address on a given port.
    Performs a basic TCP connection attempt to the target IP addresses.
    Displays the status of the connection for each IP address.

Requirements

    C Compiler (GCC or compatible)
    Linux-based Operating System (or any OS with network socket support)
    Basic knowledge of networking and TCP/IP.

Files

    netmap.c: Main source file with the code for the network mapper.
    Makefile: Build script to compile the program.
    README.md: Documentation for the project.



Compilation

    To compile the program, simply run the following command:
            (make)
    This will generate an executable file named netmap.

Usage

    After compiling the program, you can run it with the following syntax:
      ./netmap <port> [start_ip] [end_ip]


License

    This project is open source and can be used freely under the MIT license.
