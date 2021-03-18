
.section .rodata.ir_hook, "a", %progbits
.balign 4
.arm

@@@@@ IR hook @@@@@
.global IRCodePatchFunc
.type   IRCodePatchFunc, %function
IRCodePatchFunc:
add lr, lr, #4 @ Skip the address in the hook when we return to the original function
b skip_vars

i2c_readdeviceraw_addr:
    .word 0
redirected_input_addr:
    .word 0

skip_vars:
stmfd sp!, {r4-r5, lr}
ir_check_i2c:
@ Original code. Does an i2c read at device 17 func 5.
@ r0 and r1 were set up by the hook

mov r4, r1                  @ save r1
mov r2, #17
mov r3, #5

adr r12, i2c_readdeviceraw_addr
ldr r12, [r12]
blx r12

adr r5, redirected_input_addr
ldr r5, [r5]

ldr r1, =0x80800081         @ no c-stick activity / zlzr not pressed
cmp r0, #0                  @ check if the i2c read succeeded completely
ldreq r2, [r4]
subeq r3, r2, r1
andeq r2, r2, #0xFFFFF0FF
streq r2, [r4]
ldr r0, [r5]
mov r1, #0
str r1, [r5]
ldr r1, =0x20081
cmp r2, r1
ldreq r1, =0x80800081
streq r1, [r4]

checkZR:
tst r3, #0x200
beq checkZL
ldr r0, [r5]
orr r0, r0, #0x8000
str r0, [r5]

checkZL:
tst r3, #0x400
beq return
ldr r0, [r5]
orr r0, r0, #0x4000
str r0, [r5]

@ Return!
return:

mov r0, #0                  @ For ir:user.
ldmfd sp!, {r4-r5, pc}
.pool