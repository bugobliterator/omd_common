#include <common/flash.h>

#include <ch.h>
#include <hal.h>

#define FLASH_WORD_SIZE sizeof(flash_word_t)
typedef uint16_t flash_word_t;

static void flash_wait_until_ready(void) {
    while(FLASH->SR & FLASH_SR_BSY);
}

static bool flash_unlock(void) {
    if (!(FLASH->CR & FLASH_CR_LOCK)) {
        return true;
    }

    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    return !(FLASH->CR & FLASH_CR_LOCK);
}

static void flash_lock(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}

static bool flash_write_word(volatile flash_word_t* target, flash_word_t data) {
    if (((size_t)target % FLASH_WORD_SIZE) != 0) {
        return false;
    }

    // 1. Check that no main Flash memory operation is ongoing by checking the
    //    BSY bit in the FLASH_SR register.
    flash_wait_until_ready();
    // 2. Set the PG bit in the FLASH_CR register.
    FLASH->CR |= FLASH_CR_PG;
    // 3. Perform the data write (half-word) at the desired address.
    *target = data;
    // 4. Wait until the BSY bit is reset in the FLASH_SR register.
    flash_wait_until_ready();
    // 5. Check the EOP flag in the FLASH_SR register (it is set when the
    //    programming operation has succeeded), and then clear it by software.
    bool success = FLASH->SR & FLASH_SR_EOP;
    FLASH->SR |= FLASH_SR_EOP;

    return success;
}

bool flash_write(void* address, size_t write_buf_size, const void* write_buf) {
    if (!flash_unlock()) {
        return false;
    }

    bool success = true;

    while (write_buf_size > 0) {
        size_t align_ofs = (size_t)address % FLASH_WORD_SIZE;

        size_t chunk_size = FLASH_WORD_SIZE-align_ofs;
        if (chunk_size > write_buf_size) {
            chunk_size = write_buf_size;
        }

        volatile flash_word_t* aligned_word_ptr = (volatile flash_word_t*)((uint8_t*)address-align_ofs);
        flash_word_t aligned_word_val = *aligned_word_ptr;

        memcpy((uint8_t*)&aligned_word_val + align_ofs, write_buf, chunk_size);
        if (!flash_write_word(aligned_word_ptr, aligned_word_val)) {
            success = false;
        }

        address = (uint8_t*)address + chunk_size;
        write_buf = (uint8_t*)write_buf + chunk_size;
        write_buf_size -= chunk_size;
    }

    flash_lock();

    return success;
}

bool flash_erase_page(void* page_addr) {
    if (!flash_unlock()) {
        return false;
    }

    // 1. Check that no Flash memory operation is ongoing by checking the BSY
    //    bit in the FLASH_CR register
    flash_wait_until_ready();

    // 2. Set the PER bit in the FLASH_CR register
    FLASH->CR |= FLASH_CR_PER;

    // 3. Program the FLASH_AR register to select a page to erase
    FLASH->AR = (uint32_t)page_addr;

    // 4. Set the STRT bit in the FLASH_CR register
    FLASH->CR |= FLASH_CR_STRT;

    // NOTE: The software should start checking if the BSY bit equals ‘0’ at
    //       least one CPU cycle after setting the STRT bit.
    __asm__("nop");

    // 5. Wait for the BSY bit to be reset
    flash_wait_until_ready();

    // 6. Check the EOP flag in the FLASH_SR register (it is set when the erase
    //    operation has succeeded), and then clear it by software.
    bool success = FLASH->SR & FLASH_SR_EOP;
    FLASH->SR |= FLASH_SR_EOP;
    flash_lock();
    return success;
}
