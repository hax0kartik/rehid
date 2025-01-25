.3ds
.open "code.bin", "code-edited.bin", 0x00100000

patch:
//increase handle count
.org 0x111064
.dcb 4
.org 0x111074
.dcb 4

.org 0x10B634
GetLastPressedPA:
push {lr}
mov r1, #0
ldr r0, =0x113344 // latestkeys
bl svcConvertVAToPA
pop {pc}
nop
.pool

svcConvertVAToPA:
swi 0x90
bx lr

GetStatePA:
.org 0x10B65C
push {lr}
mov r1, #0
ldr r0, =0x1132C9 // initalized
bl svcConvertVAToPA
pop {pc}
.pool

.org 0x1058CC
add r0, sp, #0x60 // rawdata
add r1, sp, #0x68 // v34
bl updatem_latestkeys

.org 0x10AD40
updatem_latestkeys:
push {lr}
sub sp, sp, #0x18
str r0, [sp, #4]
str r1, [sp]

ldrh r3, [r1]
strh r3, [sp, #0xE]
ldrh r3, [r1, #4]
strh r3, [sp, #0x10]
add r2, sp, #0xE
mov r1, r2
mov r0, r2
bl 0x10F30C

ldr r3, =0x113344
mov r1, #0
str r1, [r3]

add r0, sp, #0xE
bl 0x10F208
ldr r3, =0x113344
mov r1, #0xF000000
and r0, r1, r0,LSR#4
str r0, [r3]

ldr r3, [sp, #4]
ldr r3, [r3]
and r3, r3, #0x200
cmp r3, #0
beq nextcheck
ldr r3, =0x113344
ldr r3, [r3]
orr r3, r3, #0x8000
ldr r2, =0x113344
str r3, [r2]

nextcheck:
ldr r3, [sp, #4]
ldr r3, [r3]
and r3, r3, #0x400
cmp r3, #0
beq return
ldr r3, =0x113344
ldr r3, [r3]
orr r3, r3, #0x4000
ldr r2, =0x113344
str r3, [r2]

return:
add sp, sp, #0x18
pop {pc}
.pool

RemoveZLZR:
.org 0x105884
mov r1, #0
stmea sp, {r1, r2}
mov r3, #0

.org 0x1048D0 // Remove ZL/ZR from irrstSamplingThread
orr r1, r1, #0x20000
.org 0x1048E4
orr r0, r0, #0x10000

.close