 .ORIG x3000
        LEA R1, ONE
        LDW R1, R1, #0
        LEA R2, TWO
        LDW R2, R2, #0
        LEA R3, THREE
        LDW R3, R3, #0
        LEA R6, SIX
        LDW R6, R6, #0
        LEA R7, SEVEN
        LDW R7, R7, #0

        XOR R3, R3, R3 ;//R3 -> x0000
        XOR R3, R1, R2 ;//R3 -> xFF02
        XOR R3, R2, #12 ;//R3 -> x000D
        XOR R3, R6, #-1 ;//R3 -> x000E
        XOR R3, R7, #-16 ;//R3 -> xFFF0


        LSHF R3, R3, #0 ;//R3 -> xFFF0
        LSHF R5, R3, #8 ;//R5 -> xF000
        LSHF R7, R2, #1 ;//R7 -> x0002

        RSHFL R7, R0, #1 ;//R7 -> x0000
        RSHFL R4, R4, #3 ;//R4 -> x0000

        RSHFA R1, R3, #0 ;//R1 ->ff00
        RSHFA R4, R5, #6 ;//R4 -> xFFC0

        HALT

        ONE .FILL xFF03 ;//R1
        TWO .FILL x0001 ;//R2
        THREE .FILL x0003 ;//R3
        FOUR .FILL x0008 ;//R4
        FIVE .FILL xFFF8 ;//R5
        SIX .FILL xFFF1 ;//R6
        SEVEN .FILL x0000 ;//R7
        .END