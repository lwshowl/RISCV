OUTPUT_ARCH( "riscv" )

_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x01000;

/* Specify the default entry point to the program */

MEMORY {
  com : ORIGIN = 0x00000000, LENGTH = 0x00001000
  mem : ORIGIN = 0x00001000, LENGTH = 0x00001000
  mio : ORIGIN = 0x00003000, LENGTH = 0x00001000
}

ENTRY(_start)

SECTIONS
{
  . = 0x00000000;
  .text.init : { *(.text.init) } > com
  . = ALIGN(0x0100);

  .text : { *(.text) }  > com
  . = ALIGN(0x0100);

  .data : { *(.data) } > mem
  .data.string : { *(.data.string)} > mem
  .bss : { *(.bss) } > mem
  
  .memoryio : {*(.mappedio)} > mio
  . = ALIGN(0x0100);

  _end = .;
  
}

