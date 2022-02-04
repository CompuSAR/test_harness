    .org $00a9
lda_zp_test: .byte $07

    .org $0300

start:
    nop
    nop
    nop
    lda #$03
    lda lda_zp_test     ; Make sure we don't treat the operand as an opcode

reset_handler:
    php
    ldx #$ff
    txs
    lda #start/256
    pha
    lda #start%256
    pha
    lda #0
    pha
    rti

int_handler:
nmi_handler:

    .org $fffa
    .word nmi_handler
    .word reset_handler
    .word int_handler
