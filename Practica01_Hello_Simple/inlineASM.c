// Power2_inline_asm.c
// compile with: /EHsc
// processor: x86
#include <stdio.h>
int power2(int num, int power);
int main(void)
{
    printf_s("3 times 2 to the power of 5 is %d\n", \
        power2(3, 5));      //(2^5)*3 = 96
    
}
int power2(int num, int power)
{
    __asm
    {
        mov eax, num; Get first argument
        mov ecx, power; Get second argument
        shl eax, cl; EAX = EAX * (2 to the power of CL)
    }
    // Return with result in EAX
}