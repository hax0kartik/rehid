
.section .rodata.ir_hook, "a", %progbits
.balign 4
.arm

@@@@@ IR hook @@@@@
.global IRCodePatchFunc
.type   IRCodePatchFunc, %function
IRCodePatchFunc:
add lr, lr, #0xC
ldr r4, [r3]
ldr r1, [sp, #0xC]

checkZL:
tst r0, #2
beq checkZR
orr r1, r1, #0x8000
str r1, [sp, #0xC]

checkZR:
tst r0, #4
beq remap
orr r1, r1, #0x4000
str r1, [sp, #0xC]

remap:
ldrb r12, [r2, #0x50]
cmp r12, #0
beq exit
mov r3, r2
mov r0, r1
push {lr}
add r12, r3, r12, LSL#3

loop:
ldr r2, [r3]
add r3, r3, #8
bics lr, r2, r1
ldreq lr, [r3,#-4]
eoreq r2, r2, lr
eoreq r0, r0, r2
cmp r3, r12
bne loop
pop {lr}
str r0, [sp, #0xC]
exit:
bx lr

.pool