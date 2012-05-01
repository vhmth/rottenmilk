##############################################################
# Main SPIMbot Code
#
# Taylor Fairbank | Vinay Hiremath
##############################################################



#============================================================
# Main Functionality
#============================================================

.data
    boards:         .space 648                   # the boards' storage space: 324 bytes per board; 324 = 4 * 9 * 9 = word size * # sudoku spaces on 1 board = memory for 1 board
    boards_cur:     .word 0                      # the current board to solve (rotates between 0 and boards_max)
    boards_pending: .word 0                      # the number of boards being generated
    boards_max:     .word 2                      # the max number of boards
    newline_str:    .asciiz "\n"

    # test: print_board
    newline:.asciiz "\n"        # useful for printing commands
    star:    .asciiz "*"

.text
main:

    # set our interrupts
    li   $t4, 0x4001                             # enable suduko and kick
    mtc0 $t4, $12

    # set up our stack variables
    sub  $sp, $sp, 4
    sw   $ra, 0($sp)

main_request_boards:
    # request initial boards
    #  - note: only request more when one's been solved (completion based rather than checking every loop)
    la   $t0, boards
    li   $t1, 0
    lw   $t2, boards_max
    sw   $t2, boards_pending
main_init_boards:
    sw   $t0, 0xffff00e8($0)                     # SUDOKU_GET- writes 324 bytes to the address passed in
    addi $t0, $t0, 324
    addi $t1, $t1, 1
    blt  $t1, $t2, main_init_boards

main_while:

main_sudoku:
    # 3.) solve the sudoku puzzel
    #
    # if (boards_pending != boards_max) {        // we have boards to solve
    #     Pick 1:
    #         rule 1 until no changes happen
    #         rule 2 until no changes happen
    #         brute force
    # }

    lw   $t0, boards_max
    lw   $t1, boards_pending
    beq  $t0, $t1, main_sudoku_end               # no valid boards to solve, all pending
main_sudoku_solve:
    # TODO: handle rule2 and brute force
    lw   $t0, boards_cur
    mul  $t0, $t0, 324
    la   $s0, boards
    add  $s0, $s0, $t0                           # choose the current board
    move $a0, $s0

    # test: print board
#    jal print_board
#    move $a0, $s0

    jal  rule1                              # limit on jumps, jump to a halfway point first

    # test: print rule1 result
#    move $s4, $v0
#    move $a0, $v0
#    li   $v0, 1
#    syscall
#    la   $a0, newline_str
#    li   $v0, 4
#    syscall
#    move $v0, $s4

    bne  $v0, 0, main_sudoku_end                # if we made a successful pass with our rules, go back to main loop and wait for next cycle

    # TODO: bruteforce here

    sw   $s0, 0xffff00ec($0)                     # SUDOKU_DONE- solved board pointer
    lw   $t0, boards_cur
    addi $t0, $t0, 1
    lw   $t1, boards_max
    bne  $t0, $t1, main_sudoku_getnext
    li   $t0, 0                                  # wrap around

main_sudoku_getnext:                             # solved a board, request another
    sw   $t0, boards_cur
    sw   $s0, 0xffff00e8($0)                     # SUDOKU_GET- writes 324 bytes to the address passed in
    lw   $t0, boards_pending                     # increment boards_pending
    addi $t0, $t0, 1
    sw   $t0, boards_pending

main_sudoku_end:

main_while_end:
    # 4.) jump to top of loop
    j main_while

main_return:
    # free stack space
    li   $t0, 324
    lw   $t3, boards_max
    mul  $t0, $t0, $t3
    add  $sp, $sp, $t0

    lw   $ra, 0($sp)
    addi $sp, $sp, 4

    jr   $ra

#################### SINGLETON ####################     
is_singleton:
    li    $v0, 0
    beq    $a0, 0, is_singleton_done        # return 0 if value == 0
    sub    $a1, $a0, 1
    and    $a1, $a0, $a1
    bne    $a1, 0, is_singleton_done        # return 0 if (value & (value - 1)) == 0
    li    $v0, 1
is_singleton_done:
    jr    $ra

#################### GET_SINGLETON ####################     
get_singleton:
    li    $v0, 0            # i
    li    $t1, 1
gs_loop:sll    $t2, $t1, $v0        # (1<<i)
    beq    $t2, $a0, get_singleton_done
    add    $v0, $v0, 1
    blt    $v0, 9, gs_loop        # repeat if (i < 9)
get_singleton_done:
    jr    $ra

#################### Sudoku Helpers ####################
get_square_begin:
    div $v0, $a0, 3
    mul $v0, $v0, 3
    jr  $ra

board_address:
    mul    $v0, $a1, 9        # i*9
    add    $v0, $v0, $a2        # (i*9)+j
    sll    $v0, $v0, 2        # ((i*9)+j)*4
    add    $v0, $a0, $v0
    jr    $ra

#################### RULE 1 ####################
rule1:
    sub    $sp, $sp, 32         
    sw    $ra, 0($sp)        # save $ra and free up 7 $s registers for
    sw    $s0, 4($sp)        # i
    sw    $s1, 8($sp)        # j
    sw    $s2, 12($sp)        # board
    sw    $s3, 16($sp)        # value
    sw    $s4, 20($sp)        # k
    sw    $s5, 24($sp)        # changed
    sw    $s6, 28($sp)        # temp
    move    $s2, $a0
    li    $s5, 0            # changed = false

    li    $s0, 0            # i = 0
r1_loop1:
    li    $s1, 0            # j = 0
r1_loop2:
    move    $a0, $s2        # board
    move     $a1, $s0        # i
    move    $a2, $s1        # j
    jal    board_address
    lw    $s3, 0($v0)        # value = board[i][j]
    move    $a0, $s3        
    jal    is_singleton
    beq    $v0, 0, r1_loop2_bot    # if not a singleton, we can go onto the next iteration

    li    $s4, 0            # k = 0
r1_loop3:
    beq    $s4, $s1, r1_skip_row    # skip if (k == j)
    move    $a0, $s2        # board
    move     $a1, $s0        # i
    move    $a2, $s4        # k
    jal    board_address
    lw    $t0, 0($v0)        # board[i][k]
    and    $t1, $t0, $s3        
    beq    $t1, 0, r1_skip_row
    not    $t1, $s3
    and    $t1, $t0, $t1        
    sw    $t1, 0($v0)        # board[i][k] = board[i][k] & ~value
    li    $s5, 1            # changed = true
    
r1_skip_row:
    beq    $s4, $s0, r1_skip_col    # skip if (k == i)
    move    $a0, $s2        # board
    move     $a1, $s4        # k
    move    $a2, $s1        # j
    jal    board_address
    lw    $t0, 0($v0)        # board[k][j]
    and    $t1, $t0, $s3        
    beq    $t1, 0, r1_skip_col
    not    $t1, $s3
    and    $t1, $t0, $t1        
    sw    $t1, 0($v0)        # board[k][j] = board[k][j] & ~value
    li    $s5, 1            # changed = true

r1_skip_col:    
    add    $s4, $s4, 1        # k++
    blt    $s4, 9, r1_loop3

## doubly nested loop
    move    $a0, $s0        # i
    jal    get_square_begin
    move    $s6, $v0        # ii
    move    $a0, $s1        # j
    jal    get_square_begin    # jj
    move     $t0, $s6        # k = ii
    add     $s6, $v0, 3        # jj + GRIDSIZE
    add    $t1, $t0, 3        # ii + GRIDSIZE

r1_loop4_outer:
    sub    $t2, $s6, 3        # l = jj

r1_loop4_inner:
    bne    $t0, $s0, r1_loop4_1
    beq    $t2, $s1, r1_loop4_bot

r1_loop4_1:    
    mul    $v0, $t0, 9        # k*9
    add    $v0, $v0, $t2        # (k*9)+l
    sll    $v0, $v0, 2        # ((k*9)+l)*4
    add    $v0, $s2, $v0        # &board[k][l]
    lw    $v1, 0($v0)        # board[k][l]
       and    $t3, $v1, $s3        # board[k][l] & value
    beq    $t3, 0, r1_loop4_bot

    not    $t3, $s3
    and    $v1, $v1, $t3        
    sw    $v1, 0($v0)        # board[k][l] = board[k][l] & ~value
    li    $s5, 1            # changed = true

r1_loop4_bot:    
    add    $t2, $t2, 1        # l++
    blt    $t2, $s6, r1_loop4_inner

    add    $t0, $t0, 1        # k++
    blt    $t0, $t1, r1_loop4_outer

r1_loop2_bot:    
    add    $s1, $s1, 1        # j++
    blt    $s1, 9, r1_loop2

    add    $s0, $s0, 1        # i++
    blt    $s0, 9, r1_loop1

    move    $v0, $s5        # return changed
    lw    $ra, 0($sp)        # restore registers and return
    lw    $s0, 4($sp)
    lw    $s1, 8($sp)
    lw    $s2, 12($sp)
    lw    $s3, 16($sp)
    lw    $s4, 20($sp)
    lw    $s5, 24($sp)
    lw    $s6, 28($sp)
    add    $sp, $sp, 32
    jr    $ra

#################### PRINT_NEWLINE ####################     
print_newline:
    lb       $a0, newline($0)            # read the newline char
    li       $v0, 11            # load the syscall option for printing chars
    syscall                  # print the char
    
    jr      $ra              # return to the calling procedure


#################### PRINT_INT_AND_SPACE ####################     
print_int_and_space:
    li       $v0, 1             # load the syscall option for printing ints
    syscall                  # print the element

    li       $a0, 32            # print a black space (ASCII 32)
    li       $v0, 11            # load the syscall option for printing chars
    syscall                  # print the char
    
    jr      $ra              # return to the calling procedure

#################### PRINT BOARD ####################     
print_board:
    sub    $sp, $sp, 20
    sw    $ra, 0($sp)        # save $ra and free up 4 $s registers for
    sw    $s0, 4($sp)        # i
    sw    $s1, 8($sp)        # j
    sw    $s2, 12($sp)        # the function argument
    sw    $s3, 16($sp)        # the computed pointer (which is used for 2 calls)
    move    $s2, $a0

    li    $s0, 0            # i
pb_loop1:
    li    $s1, 0            # j
pb_loop2:
    mul    $t0, $s0, 9        # i*9
    add    $t0, $t0, $s1        # (i*9)+j
    sll    $t0, $t0, 2        # ((i*9)+j)*4
    add    $s3, $s2, $t0
    lw    $a0, 0($s3)
    jal    is_singleton        
    beq    $v0, 0, pb_star        # if it was not a singleton, jump
    lw    $a0, 0($s3)
    jal    get_singleton        
    add    $a0, $v0, 1        # print the value + 1
    li    $v0, 1
    syscall
    j    pb_cont

pb_star:        
    li    $v0, 4            # print a "*"
    la    $a0, star
    syscall

pb_cont:    
    add    $s1, $s1, 1        # j++
    blt    $s1, 9, pb_loop2

    li    $v0, 4            # at the end of a line, print a newline char.
    la    $a0, newline
    syscall    
    
    add    $s0, $s0, 1        # i++
    blt    $s0, 9, pb_loop1

    lw    $ra, 0($sp)        # restore registers and return
    lw    $s0, 4($sp)
    lw    $s1, 8($sp)
    lw    $s2, 12($sp)
    lw    $s3, 16($sp)
    add    $sp, $sp, 20
    jr    $ra

#============================================================
# Interrupt Handler
#============================================================

.kdata
    chunkIH:            .space 8
    non_intrpt_str:     .asciiz "Non-interrupt exception\n"
    unhandled_str:      .asciiz "Unhandled interrupt type\n"

.ktext 0x80000080
interrupt_handler:
.set noat
    move $k1, $at
.set at

    la   $k0, chunkIH
    sw   $a0, 0($k0)
    sw   $a1, 4($k0)

    mfc0 $k0, $13
    srl  $a0, $k0, 2
    and  $a0, $a0, 0xf
    bne  $a0, 0  , non_intrpt

interrupt_dispatch:
    mfc0 $k0, $13
    beq  $k0, $zero, interrupt_done

    and  $a0, $k0, 0x4000
    bne  $a0, 0, puzzle_interrupt

    li   $v0, 4
    la   $a0, unhandled_str
    syscall

    j    interrupt_done

puzzle_interrupt:
    # board is stored in passed in heap address
    lw   $a0, boards_pending                     # decrement boards_pending
    sub  $a0, $a0, 1
    sw   $a0, boards_pending

    sw   $a0, 0xffff0068($0)                     # acknowledge interrupt: can write any value
    j    interrupt_dispatch

non_intrpt:

    li   $v0, 4
    la   $a0, non_intrpt_str
    syscall
    #test
    j    infinite
    j    interrupt_done

interrupt_done:

    la   $k0, chunkIH
    lw   $a0, 0($k0)
    lw   $a1, 4($k0)
    mfc0 $k0, $14

    .set noat

    move $at  $k1

    .set at

    rfe
    jr   $k0
    nop

infinite:
    j infinite
