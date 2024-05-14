.arm

.global CDCHID_GetDataAsm
.type CDCHID_GetDataAsm, % function
CDCHID_GetDataAsm:
push {r4 - r6, lr}
mov r5, r1
mrc p15, 0, r4, c13, c0, 3
mov r1, #0x10000
str r1, [r4, #0x80]!
ldr r0, [r0]
svc 0x32
ands r1, r0, #0x80000000
bmi ret
ldrd r0, r1, [r4, #8]
strd r0, r1, [r5]
ldr r0, [r4, #4]
ret:
pop {r4 - r6, pc}