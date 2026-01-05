// Minimal C kernel test - NO C runtime dependencies

void _start(void) {
    // Use inline assembly to avoid ANY C library calls
    __asm__ volatile (
        "mov $0xA0000, %%edi\n"      // VGA memory
        "mov $9, %%al\n"              // Blue color
        "mov $64000, %%ecx\n"         // Screen size
        "rep stosb\n"                 // Fill screen
        "1:\n"
        "hlt\n"
        "jmp 1b\n"
        :
        :
        : "edi", "eax", "ecx"
    );
    
    // Should never reach here
    while(1);
}