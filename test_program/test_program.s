CARRY    = %00000001
ZERO     = %00000010
INTMASK  = %00000100
DECIMAL  = %00001000
BRK      = %00100000
OVERFLOW = %01000000
NEGATIVE = %10000000

FINISHED_TRIGGER        = $200
NMI_TRIGGER_COUNT       = $2fa
NMI_TRIGGER_DELAY       = $2fb
RESET_TRIGGER_COUNT     = $2fc
RESET_TRIGGER_DELAY     = $2fd
INT_TRIGGER_COUNT       = $2fe
INT_TRIGGER_DELAY       = $2ff


    .org $00a9
lda_zp_test: .byte $07

    .org $0300

start:
    nop
    nop
    nop
    lda #$03
    lda lda_zp_test     ; Make sure we don't treat the operand as an opcode

    sta FINISHED_TRIGGER

reset_handler:
    ldx #$ff
    txs
    lda #start/256
    pha
    lda #start%256
    pha
    lda #INTMASK+OVERFLOW
    pha
    rti

int_handler:
nmi_handler:
    brk

    .org $fffa
    .word nmi_handler
    .word reset_handler
    .word int_handler
