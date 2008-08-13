#include "regs.h"
#include "wikireader.h"

enum {
	SPI_WRITE = 0,
	SPI_READ = 1,
	SPI_CS_HI = 2,
	SPI_CS_LO = 3
};

int main(void)
{
	unsigned char cmd, dat, len;

	init_pins();
	init_rs232();
	SDCARD_CS_HI();
	EEPROM_CS_HI();

	/* EEPROM WP: off */
	REG_P2_IOC2 = (1 << 6);
	REG_P2_P2D  = (1 << 6);


	/* enable SPI: master mode, no DMA, 8 bit transfers */
	REG_SPI_CTL1 = 0x73 | (7 << 10) | (7 << 4);

	for (;;) {
		do {} while (!(REG_EFSIF0_STATUS & 0x1));
		cmd = REG_EFSIF0_RXD;

		switch (cmd) {
			case SPI_CS_HI:
				EEPROM_CS_HI();
				break;
			case SPI_CS_LO:
				EEPROM_CS_LO();
				break;
			case SPI_WRITE:
				do {} while (!(REG_EFSIF0_STATUS & 0x1));
				len = REG_EFSIF0_RXD;

				while (len--) {
					do {} while (!(REG_EFSIF0_STATUS & 0x1));
					dat = REG_EFSIF0_RXD;
					REG_SPI_TXD = dat;
					do {} while (REG_SPI_STAT & (1 << 6));
				}
				break;
			case SPI_READ:
				do {} while (!(REG_EFSIF0_STATUS & 0x1));
				len = REG_EFSIF0_RXD;
				
				while (len--) {
					REG_SPI_TXD = 0x00;
					do {} while (REG_SPI_STAT & (1 << 6));
					dat = REG_SPI_RXD;
					REG_EFSIF0_TXD = dat;
					do {} while (REG_EFSIF0_STATUS & (1 << 5));
				}
				break;
		}
	}
}

