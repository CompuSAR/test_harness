    .org $0300

    .list
    nop
    .ifdef CPU_WDC
    nop
    .fi
    nop
    lda #$03
    lda #$05
    lda $a9
    nop
    lda $8623
    nop

    .org $00
    .ascii "Hello, world",10,"How are you?",0
    .blk 12, 1


