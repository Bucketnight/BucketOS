#include "bucketos/gdt.h"
#include "bucketos/paging.h"
#include "bucketos/string.h"
#include "bucketos/syscall.h"
#include "bucketos/usertest.h"

enum {
    USER_STUB_MOV_EAX_IMM = 0xB8,
    USER_STUB_MOV_EBX_IMM = 0xBB,
    USER_STUB_MOV_ECX_IMM = 0xB9,
    USER_STUB_INT = 0xCD,
    USER_STUB_XOR_EBX = 0x31,
    USER_STUB_INT_VECTOR = 0x80,
    USER_STUB_XOR_EBX_MODRM = 0xDB
};

static bool g_usertest_loaded;

static void encode_u32(uint8_t *buffer, uint32_t value) {
    buffer[0] = (uint8_t)(value & 0xFFu);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFu);
    buffer[2] = (uint8_t)((value >> 16) & 0xFFu);
    buffer[3] = (uint8_t)((value >> 24) & 0xFFu);
}

void usertest_initialize(void) {
    static const char message[] = "hello from ring 3 via int 0x80\n";
    static const uint8_t binary_image[] = {
        USER_STUB_MOV_EAX_IMM, 0, 0, 0, 0,
        USER_STUB_MOV_EBX_IMM, 0, 0, 0, 0,
        USER_STUB_MOV_ECX_IMM, 0, 0, 0, 0,
        USER_STUB_INT, USER_STUB_INT_VECTOR,
        USER_STUB_MOV_EAX_IMM, 0, 0, 0, 0,
        USER_STUB_XOR_EBX, USER_STUB_XOR_EBX_MODRM,
        USER_STUB_INT, USER_STUB_INT_VECTOR
    };

    const user_space_mapping_t *const user_space = paging_user_space();
    uint8_t *const code = (uint8_t *)user_space->code_physical;
    const uintptr_t message_virtual = user_space->code_virtual + sizeof(binary_image);

    memset(code, 0, 4096u);
    memcpy(code, binary_image, sizeof(binary_image));
    encode_u32(&code[1], SYSCALL_WRITE);
    encode_u32(&code[6], (uint32_t)message_virtual);
    encode_u32(&code[11], (uint32_t)(sizeof(message) - 1u));
    encode_u32(&code[18], SYSCALL_EXIT);
    memcpy(code + sizeof(binary_image), message, sizeof(message));

    g_usertest_loaded = true;
}

uint32_t usertest_run(void) {
    const user_space_mapping_t *const user_space = paging_user_space();

    if (!g_usertest_loaded) {
        usertest_initialize();
    }

    return gdt_enter_user_mode(user_space->code_virtual, user_space->stack_top_virtual);
}
