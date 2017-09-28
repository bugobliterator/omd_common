#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

bool flash_erase_page(void* page_addr);
bool flash_write(void* address, size_t write_buf_size, const void* write_buf);
