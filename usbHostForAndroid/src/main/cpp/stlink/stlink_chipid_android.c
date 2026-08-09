#include <stdint.h>
#include <string.h>

#include <chipid.h>

static char device_type[] = "STM32G0Bx_G0Cx";
static char reference_manual[] = "0444";

static struct stlink_chipid_params stm32g0bc = {
    .dev_type = device_type,
    .ref_manual_id = reference_manual,
    .chip_id = 0x467u,
    .flash_type = STM32_FLASH_TYPE_G0,
    .flash_size_reg = 0x1fff75e0u,
    .flash_pagesize = 0x800u,
    .sram_size = 0x24000u,
    .bootrom_base = 0x1fff0000u,
    .bootrom_size = 0x7000u,
    .option_base = 0x1fff7800u,
    .option_size = 0x80u,
    .flags = CHIP_F_HAS_DUAL_BANK,
    .otp_base = 0u,
    .otp_size = 0u,
    .next = NULL
};

struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chip_id) {
    return chip_id == stm32g0bc.chip_id ? &stm32g0bc : NULL;
}

void dump_a_chip(struct stlink_chipid_params *device) {
    (void)device;
}

void process_chipfile(char *filename) {
    (void)filename;
}

void init_chipids(char *directory) {
    (void)directory;
}
