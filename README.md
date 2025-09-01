# csapplabs

## Overview

This repository contains my solutions and implementation code for the labs from *Computer Systems: A Programmer's Perspective (CS:APP)*, 3rd Edition. These hands-on projects help deepen the understanding of fundamental computer systems concepts through practical implementation.

---

## Lab List

### 1. Data Lab
- **Description**: Solve a series of puzzles involving bit-level manipulations under strict coding constraints
- **Topics**: Two's complement, floating-point representation, bitwise operations
- **Key Skills**: Bit manipulation, integer and floating-point representations

### 2. Bomb Lab
- **Description**: Reverse-engineer a "binary bomb" executable using GDB debugging to defuse six phases
- **Topics**: Assembly code analysis, debugging techniques, memory layout
- **Key Skills**: GDB usage, assembly understanding, reverse engineering

### 3. Attack Lab
- **Description**: Exploit buffer overflow vulnerabilities through code injection and Return-Oriented Programming (ROP) attacks across five phases
- **Topics**: Stack smashing, code injection, ROP techniques
- **Key Skills**: Vulnerability exploitation, stack manipulation

### 4. Cache Lab
- **Description**: Implement a configurable cache simulator in C with LRU replacement policy
- **Topics**: Cache memory hierarchy, replacement policies, simulation
- **Key Skills**: Cache design, performance optimization

### 5. Shell Lab (shlab)
- **Description**: Build a Unix shell supporting job control, signal handling, and built-in commands
- **Topics**: Process control, signal handling, job management
- **Key Skills**: Process creation, signal handling, shell implementation

### 6. Malloc Lab
- **Description**: Implement a dynamic memory allocator using explicit free lists with LIFO policy
- **Topics**: Memory allocation, fragmentation, free list management
- **Key Skills**: Memory management, pointer manipulation

### 7. Proxy Lab
- **Description**: Create a concurrent HTTP proxy with caching capabilities
- **Topics**: Network programming, concurrency, caching
- **Key Skills**: Socket programming, thread synchronization

### 8. Arch Lab (Optional)
- **Description**: Not implemented in this repository
- **Status**: Optional architecture-related lab not included

---

## Resources

- [CS:APP Official Website](http://csapp.cs.cmu.edu/)
- [CS:APP Labs Assignment](http://csapp.cs.cmu.edu/3e/labs.html)
- Computer Systems: A Programmer's Perspective, 3rd Edition

---

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Chinese introduction-中文简介

1.datalab：完成一系列的二进制相关的位操作。

2.bomblab：利用反编译手段查看一个“炸弹”程序的汇编代码，GDB调试断点，查看寄存器内容，通过六个阶段的从而拆除“炸弹”程序。

3.attacklab：利用堆栈溢出的漏洞，向目标代码插入特定字符，从而使目标程序跳转到自己期望的地方，一共5个阶段，攻击技术包括注入式代码攻击和面向返回攻击。

4.cachelab：用c语言实现一个cache的软件模拟，可传参控制缓存的大小，组数量，块数量等，采用LRU替换策略。 

5.shlab：用c语言实现一个简单的shell，功能包括&，ctrl-c，ctrl-z，quit，jobs，但是不要求实现管道和重定向。

6.malloclab：用c语言软件模拟基于显式链表的内存分配器，采用LIFO策略管理块。

7.proxylab：用线程方式实现能够处理并发请求且拥有一个固定大小缓存器的代理。

8.archlab：没做。
