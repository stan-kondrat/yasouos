/*
 * Byte Order Conversion Test Suite (Freestanding)
 */

#include "../tests/test-kernel/test_kernel_common.h"
#include "byteorder.h"

void test_htonl(void) {
    test_start("htonl");
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    test_assert_eq_uint32(htonl(0x12345678), 0x12345678, "big-endian");
    test_assert_eq_uint32(htonl(0xAABBCCDD), 0xAABBCCDD, "0xAABBCCDD big-endian");
#else
    test_assert_eq_uint32(htonl(0x12345678), 0x78563412, "little-endian");
    test_assert_eq_uint32(htonl(0xAABBCCDD), 0xDDCCBBAA, "0xAABBCCDD little-endian");
#endif
    test_assert_eq_uint32(htonl(0x00000000), 0x00000000, "zero");
    test_assert_eq_uint32(htonl(0xFFFFFFFF), 0xFFFFFFFF, "all ones");
}

void test_ntohl(void) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    test_assert_eq_uint32(ntohl(0x12345678), 0x12345678, "ntohl on big-endian");
    test_assert_eq_uint32(ntohl(0xAABBCCDD), 0xAABBCCDD, "ntohl 0xAABBCCDD on big-endian");
#else
    test_assert_eq_uint32(ntohl(0x12345678), 0x78563412, "ntohl on little-endian");
    test_assert_eq_uint32(ntohl(0xAABBCCDD), 0xDDCCBBAA, "ntohl 0xAABBCCDD on little-endian");
#endif
    test_assert_eq_uint32(ntohl(0x00000000), 0x00000000, "ntohl zero");
    test_assert_eq_uint32(ntohl(0xFFFFFFFF), 0xFFFFFFFF, "ntohl all ones");
}

void test_htons(void) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    test_assert_eq_uint16(htons(0x1234), 0x1234, "htons on big-endian");
    test_assert_eq_uint16(htons(0xAABB), 0xAABB, "htons 0xAABB on big-endian");
#else
    test_assert_eq_uint16(htons(0x1234), 0x3412, "htons on little-endian");
    test_assert_eq_uint16(htons(0xAABB), 0xBBAA, "htons 0xAABB on little-endian");
#endif
    test_assert_eq_uint16(htons(0x0000), 0x0000, "htons zero");
    test_assert_eq_uint16(htons(0xFFFF), 0xFFFF, "htons all ones");
}

void test_ntohs(void) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    test_assert_eq_uint16(ntohs(0x1234), 0x1234, "ntohs on big-endian");
    test_assert_eq_uint16(ntohs(0xAABB), 0xAABB, "ntohs 0xAABB on big-endian");
#else
    test_assert_eq_uint16(ntohs(0x1234), 0x3412, "ntohs on little-endian");
    test_assert_eq_uint16(ntohs(0xAABB), 0xBBAA, "ntohs 0xAABB on little-endian");
#endif
    test_assert_eq_uint16(ntohs(0x0000), 0x0000, "ntohs zero");
    test_assert_eq_uint16(ntohs(0xFFFF), 0xFFFF, "ntohs all ones");
}

void test_ntohl_unaligned(void) {
    uint8_t aligned_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    test_assert_eq_uint32(ntohl_unaligned(aligned_data), 0xAABBCCDD, "ntohl_unaligned aligned");

    uint8_t unaligned_buffer[8] = {0xFF, 0x12, 0x34, 0x56, 0x78, 0xFF, 0xFF, 0xFF};
    test_assert_eq_uint32(ntohl_unaligned(&unaligned_buffer[1]), 0x12345678, "ntohl_unaligned offset +1");

    uint8_t unaligned_buffer2[8] = {0xFF, 0xFF, 0xDE, 0xAD, 0xBE, 0xEF, 0xFF, 0xFF};
    test_assert_eq_uint32(ntohl_unaligned(&unaligned_buffer2[2]), 0xDEADBEEF, "ntohl_unaligned offset +2");

    uint8_t unaligned_buffer3[8] = {0xFF, 0xFF, 0xFF, 0xCA, 0xFE, 0xBA, 0xBE, 0xFF};
    test_assert_eq_uint32(ntohl_unaligned(&unaligned_buffer3[3]), 0xCAFEBABE, "ntohl_unaligned offset +3");

    uint8_t zeros[] = {0x00, 0x00, 0x00, 0x00};
    test_assert_eq_uint32(ntohl_unaligned(zeros), 0x00000000, "ntohl_unaligned zeros");

    uint8_t ones[] = {0xFF, 0xFF, 0xFF, 0xFF};
    test_assert_eq_uint32(ntohl_unaligned(ones), 0xFFFFFFFF, "ntohl_unaligned all ones");
}

void test_htonl_unaligned(void) {
    uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    test_assert_eq_uint32(htonl_unaligned(data), 0xAABBCCDD, "htonl_unaligned");

    uint8_t unaligned_buffer[7] = {0xFF, 0x11, 0x22, 0x33, 0x44, 0xFF, 0xFF};
    test_assert_eq_uint32(htonl_unaligned(&unaligned_buffer[1]), 0x11223344, "htonl_unaligned offset +1");
}

void test_write_htonl_unaligned(void) {
    uint8_t buffer1[4] = {0};
    write_htonl_unaligned(buffer1, 0xAABBCCDD);
    uint8_t expected1[] = {0xAA, 0xBB, 0xCC, 0xDD};
    test_assert_mem_eq(buffer1, expected1, 4, "write_htonl_unaligned aligned");

    uint8_t buffer2[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    write_htonl_unaligned(&buffer2[1], 0x12345678);
    uint8_t expected2[] = {0xFF, 0x12, 0x34, 0x56, 0x78, 0xFF, 0xFF, 0xFF};
    test_assert_mem_eq(buffer2, expected2, 8, "write_htonl_unaligned offset +1");

    uint8_t buffer3[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    write_htonl_unaligned(&buffer3[3], 0xDEADBEEF);
    uint8_t expected3[] = {0xFF, 0xFF, 0xFF, 0xDE, 0xAD, 0xBE, 0xEF, 0xFF};
    test_assert_mem_eq(buffer3, expected3, 8, "write_htonl_unaligned offset +3");

    uint8_t buffer4[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    write_htonl_unaligned(buffer4, 0x00000000);
    uint8_t expected4[] = {0x00, 0x00, 0x00, 0x00};
    test_assert_mem_eq(buffer4, expected4, 4, "write_htonl_unaligned zeros");

    uint8_t buffer5[4] = {0x00, 0x00, 0x00, 0x00};
    write_htonl_unaligned(buffer5, 0xFFFFFFFF);
    uint8_t expected5[] = {0xFF, 0xFF, 0xFF, 0xFF};
    test_assert_mem_eq(buffer5, expected5, 4, "write_htonl_unaligned all ones");
}

void test_write_ntohl_unaligned(void) {
    uint8_t buffer[4] = {0};
    write_ntohl_unaligned(buffer, 0xAABBCCDD);
    uint8_t expected[] = {0xAA, 0xBB, 0xCC, 0xDD};
    test_assert_mem_eq(buffer, expected, 4, "write_ntohl_unaligned");
}

void test_ntohs_unaligned(void) {
    uint8_t aligned_data[] = {0xAA, 0xBB};
    test_assert_eq_uint16(ntohs_unaligned(aligned_data), 0xAABB, "ntohs_unaligned aligned");

    uint8_t unaligned_buffer[5] = {0xFF, 0x12, 0x34, 0xFF, 0xFF};
    test_assert_eq_uint16(ntohs_unaligned(&unaligned_buffer[1]), 0x1234, "ntohs_unaligned offset +1");

    uint8_t unaligned_buffer2[6] = {0xFF, 0xFF, 0xFF, 0xCA, 0xFE, 0xFF};
    test_assert_eq_uint16(ntohs_unaligned(&unaligned_buffer2[3]), 0xCAFE, "ntohs_unaligned offset +3");

    uint8_t zeros[] = {0x00, 0x00};
    test_assert_eq_uint16(ntohs_unaligned(zeros), 0x0000, "ntohs_unaligned zeros");

    uint8_t ones[] = {0xFF, 0xFF};
    test_assert_eq_uint16(ntohs_unaligned(ones), 0xFFFF, "ntohs_unaligned all ones");
}

void test_htons_unaligned(void) {
    uint8_t data[] = {0xAA, 0xBB};
    test_assert_eq_uint16(htons_unaligned(data), 0xAABB, "htons_unaligned");

    uint8_t unaligned_buffer[5] = {0xFF, 0x56, 0x78, 0xFF, 0xFF};
    test_assert_eq_uint16(htons_unaligned(&unaligned_buffer[1]), 0x5678, "htons_unaligned offset +1");
}

void test_write_htons_unaligned(void) {
    uint8_t buffer1[2] = {0};
    write_htons_unaligned(buffer1, 0xAABB);
    uint8_t expected1[] = {0xAA, 0xBB};
    test_assert_mem_eq(buffer1, expected1, 2, "write_htons_unaligned aligned");

    uint8_t buffer2[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    write_htons_unaligned(&buffer2[1], 0x1234);
    uint8_t expected2[] = {0xFF, 0x12, 0x34, 0xFF, 0xFF};
    test_assert_mem_eq(buffer2, expected2, 5, "write_htons_unaligned offset +1");

    uint8_t buffer3[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    write_htons_unaligned(&buffer3[3], 0xCAFE);
    uint8_t expected3[] = {0xFF, 0xFF, 0xFF, 0xCA, 0xFE, 0xFF};
    test_assert_mem_eq(buffer3, expected3, 6, "write_htons_unaligned offset +3");

    uint8_t buffer4[2] = {0xFF, 0xFF};
    write_htons_unaligned(buffer4, 0x0000);
    uint8_t expected4[] = {0x00, 0x00};
    test_assert_mem_eq(buffer4, expected4, 2, "write_htons_unaligned zeros");

    uint8_t buffer5[2] = {0x00, 0x00};
    write_htons_unaligned(buffer5, 0xFFFF);
    uint8_t expected5[] = {0xFF, 0xFF};
    test_assert_mem_eq(buffer5, expected5, 2, "write_htons_unaligned all ones");
}

void test_write_ntohs_unaligned(void) {
    uint8_t buffer[2] = {0};
    write_ntohs_unaligned(buffer, 0xAABB);
    uint8_t expected[] = {0xAA, 0xBB};
    test_assert_mem_eq(buffer, expected, 2, "write_ntohs_unaligned");
}

void test_roundtrip_32bit(void) {
    uint32_t original = 0xDEADBEEF;
    uint8_t buffer[4];
    write_htonl_unaligned(buffer, original);
    uint32_t result = ntohl_unaligned(buffer);
    test_assert_eq_uint32(result, original, "32-bit write->read roundtrip");
}

void test_roundtrip_16bit(void) {
    uint16_t original = 0xCAFE;
    uint8_t buffer[2];
    write_htons_unaligned(buffer, original);
    uint16_t result = ntohs_unaligned(buffer);
    test_assert_eq_uint16(result, original, "16-bit write->read roundtrip");
}

void test_symmetry(void) {
    uint32_t val32 = 0x12345678;
    test_assert_eq_uint32(ntohl(htonl(val32)), val32, "htonl/ntohl symmetry");

    uint16_t val16 = 0xABCD;
    test_assert_eq_uint16(ntohs(htons(val16)), val16, "htons/ntohs symmetry");
}

// Entry point for tests
void test_kernel_main(void) {
    test_suite_start("Byte Order");

    test_htonl();
    test_ntohl();
    test_htons();
    test_ntohs();
    test_ntohl_unaligned();
    test_htonl_unaligned();
    test_write_htonl_unaligned();
    test_write_ntohl_unaligned();
    test_ntohs_unaligned();
    test_htons_unaligned();
    test_write_htons_unaligned();
    test_write_ntohs_unaligned();
    test_roundtrip_32bit();
    test_roundtrip_16bit();
    test_symmetry();

    test_suite_end();
}
