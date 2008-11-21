/*
    e07 bootloader suite
    Copyright (c) 2008 Daniel Mack <daniel@caiaq.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "regs.h"
#include "types.h"
#include "wikireader.h"
#include "eeprom.h"
#include "misc.h"
#include "elf32.h"
#include "lcd.h"

#define KERNEL "/KERNEL"

__attribute__((noreturn))
int main(void)
{
	init_pins();
	init_rs232();
	init_ram();

	EEPROM_CS_HI();
	SDCARD_CS_HI();
	
	/* value of default data area is hard-coded in this case */
	asm("xld.w   %r15, 0x1500");

	print("Bootloader starting\n");

	/* enable SPI: master mode, no DMA, 8 bit transfers */
	REG_SPI_CTL1 = 0x03 | (7 << 10) | (1 << 4);

	print_u32(elf_exec(KERNEL));
	
	/* load the 'could not boot from SD card' image */
	eeprom_load(0x10000, (u8 *) 0x10000000, (320 * 240) / 2);
#if BOARD_PROTO1
	{
		int i;
		for (i = 0x10000000; i < 0x10000000 + (320 * 240) / 2; i++)
			*(char *) i ^= 0xff;
	}
#endif
	init_lcd();

	/* if we get here, boot_from_sdcard() failed to find a kernel on the
	 * inserted media or there is no media. Thus, we register an
	 * interrupt handler for the SD card insert switch and try again as
	 * soon as a media switch is detected. */

	/* TODO */

	asm("slp");

	for(;;);
}

