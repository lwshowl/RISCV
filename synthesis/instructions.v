`ifndef INSTRUCTIONS_V
`define INSTRUCTIONS_V

//instructions
`define   i_lui     0
`define   i_auipc   1
`define   i_jal     2
`define   i_jalr    3
`define   i_beq     4
`define   i_bne     5
`define   i_blt     6
`define   i_bge     7
`define   i_bltu    8
`define   i_bgeu    9
`define   i_lb      10
`define   i_lh      11
`define   i_lw      12
`define   i_lbu     13
`define   i_lhu     14
`define   i_sb      15
`define   i_sh      16
`define   i_sw      17
`define   i_addi    18
`define   i_slti    19
`define   i_sltiu   20
`define   i_xori    21
`define   i_ori     22
`define   i_andi    23
`define   i_slli    24
`define   i_srli    25
`define   i_srai    26
`define   i_add     27
`define   i_sub     28
`define   i_sll     29
`define   i_slt     30
`define   i_sltu    31
`define   i_xor     32
`define   i_srl     33
`define   i_sra     34
`define   i_or      35
`define   i_and     36
`define   i_ecall   37
`define   i_ebreak  38
`define   i_csrrw   39
`define   i_csrrs   40
`define   i_csrrc   41
`define   i_csrrwi  42
`define   i_csrrsi  43
`define   i_csrrci  44
`define   i_invalid 63 
`endif