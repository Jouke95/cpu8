; count down from 10 to 0, and keep a value in memory
        LOAD  A 10
        LOAD  B 1
loop:   SUB   A B
        PRINT A
        JZ    end
        JMP   loop
end:    LOAD  A 100
        STORE A 142         ; data belongs above 128
        LOAD  B #142        ; and read it back
        PRINT B
        HALT