# Caesar_Cipher_OpenCL_Multithreading
This repository contains code that encrypts text using the Caesar or Substitution Cipher.  
OpenCL and POSIX threads are used to achieve this.  

A Caesar cipher is a simple encryption cipher in which each letter
in the plaintext is replaced with another letter that is located a certain positive number, n, positions
away in the alphabet. If it reaches the end of the alphabets it wrap around to the start.  

For example, if n = 2, each character will be replaced by the character which is located 2 positions
after it. If you want to encrypt the message: Hello, the output will be: Jgnnq  

I have coded 3 different implementations of the Caesar Cipher.  
The first method uses POSIX threads only.  
The second method uses OpenCL only.  
The third method uses a combination of POSIX and OpenCL threads (OpenCL threads are started before the POSIX threads).
