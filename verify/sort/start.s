.section .text.init
.globl _start
_start:
    li sp,0x1000
    jal ra,main

_hlt:
    j _hlt
