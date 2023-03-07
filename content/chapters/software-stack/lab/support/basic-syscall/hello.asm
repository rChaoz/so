; SPDX-License-Identifier: BSD-3-Clause

section .rodata

hello db "Hello, world!", 10, 0
hello_len equ $ - hello
bye db "Bye, world! *commits sudoku*", 10, 0
bye_len equ $ - bye

section .text

global main

main:
    push rbp
    mov rbp, rsp

    ; Call write(1, "Hello, world!\n", 14).
    ; rax <- __NR_write (index of write syscall: 1)
    ; rdi <- first syscall argument (fd: 1)
    ; rsi <- second syscall argument (buffer: hello)
    ; rdx <- third syscall argument (lenght: 14)
    mov rdi, 1
    mov rsi, hello
    mov rdx, hello_len
    mov rax, 1
    syscall
	
    mov rdi, 1
    mov rsi, bye
    mov rdx, bye_len
    mov rax, 1
    syscall

    ; Call exit(0).
    ; rax <- __NR_exit (index of exit syscall: 60)
    ; rdi <- first syscall argument (error_code: 0)
    mov rdi, 0
    mov rax, 60
    syscall

    xor rax, rax
    leave
    ret
