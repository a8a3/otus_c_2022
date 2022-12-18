bits 64
extern malloc, puts, printf, fflush, abort
global main

section .data
empty_str: db 0x0             ;db 'b' means byte
int_format: db "%ld ", 0x0    ;db 'b' means byte
data: dq 4, 8, 15, 16, 23, 42 ;dq 'q' means quadword(8 bytes) -> long values list
data_length: equ ($-data) / 8 ;equ means constant -> list length

section .text

;print_int proc
print_int:
    mov rsi, rdi        ;1st input parameter contains list node value
    mov rdi, int_format ;int_format will be passed as 2nd parameter to printf
    xor rax, rax
    call printf         ;call printf func

    xor rdi, rdi ;to pass a NULL to fflush
    call fflush  ;flushes all open output streams

    ret

;p proc
p:
    mov rax, rdi ;1st input parameter -> rax
    and rax, 1   ;if rax is even, reset rax
    ret

;add_element proc
add_element:
    push rbp
    push rbx

    mov rbp, rdi ;put 1st input parameter into rbp
    mov rbx, rsi ;put 2nd input parameter into rbx

    mov rdi, 16   ;16 -> rdi
    call malloc   ;malloc 16 bytes
    test rax, rax ;rax contains result of malloc call 
    jz abort      ;if malloc returns NULL, abort

    mov [rax], rbp     ;by address in rax put rbp val (i.e 1st input)
    mov [rax + 8], rbx ;by address in rax + 8 put rbx val (i.e 2nd input)

    pop rbx
    pop rbp

    ret

;m proc(map) (applies passed function to each element in the list)
m:
    test rdi, rdi ;if rdi contains a null, return
    jz outm

    push rbp
    push rbx

    mov rbx, rdi  ;1st input parameter(pointer to the list node) -> rbx 
    mov rbp, rsi  ;2nd input parameter(pointer to print_int func) -> rbp

    mov rdi, [rdi] ;dereferenced list val -> rdi
    call rsi       ;call print_int func with value from the list node

    mov rdi, [rbx + 8] ;move next list node addr to rdi
    mov rsi, rbp       ;move func pointer to rsi 
    call m             ;call m recursively 

    pop rbx
    pop rbp

outm:
    ret

;f proc(filter) (uses list as a source and creates a new one with elements satisfying passed filter func)
f:
    mov rax, rsi  ;first rsi val is 0

    test rdi, rdi ;rdi contains list node
    jz outf

    push rbx
    push r12
    push r13

    mov rbx, rdi ;rbx -> 1st input parameter (src list node)
    mov r12, rsi ;r12 -> 2nd input parameter (dst list node)
    mov r13, rdx ;r13 -> p proc addr

    mov rdi, [rdi]
    call rdx       ;call p proc with list node val
    test rax, rax
    jz z           ;if p proc returned 0, go to z label

    mov rdi, [rbx]
    mov rsi, r12
    call add_element
    mov rsi, rax
    jmp ff

z:
    mov rsi, r12

ff:
    mov rdi, [rbx + 8]
    mov rdx, r13
    call f ;call f recursively

    pop r13
    pop r12
    pop rbx

outf:
    ret

;main proc
main:
    push rbx

    xor rax, rax         ;reset rax
    mov rbx, data_length ;data_length val goes to rbx register, rbx will be the loop counter

    adding_loop:
        mov rdi, [data - 8 + rbx * 8]  ;rdi contains 1st arg of add_element
        mov rsi, rax                   ;rsi contains the 2nd one 
        call add_element               ;call add_element
        dec rbx                        ;decrease loop counter by one
    jnz adding_loop ;jump to addong_loop label if 'zero flag' in rflags is not set

    mov rbx, rax ;save list head to rbx

    mov rdi, rax       ;pass list head as 1st parameter to m proc
    mov rsi, print_int ;pass print_list addr as 2nd parameter to m proc
    call m             ;m proc call

    mov rdi, empty_str
    call puts ;writes empty_str to stdout

    mov rdx, p   ;p proc addr -> rdx
    xor rsi, rsi ;reset rsi
    mov rdi, rbx ;rbx(list head) -> rdi
    call f       ;f proc call

    mov rdi, rax
    mov rsi, print_int
    call m

    mov rdi, empty_str
    call puts ;writes empty_str to stdout

    pop rbx

    xor rax, rax ;reset rax
    ret
