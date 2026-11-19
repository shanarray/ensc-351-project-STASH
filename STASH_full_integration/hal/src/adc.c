
#include "adc.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

static int spi_fd = -1;
static const uint32_t spi_speed = 250000;

int Adc_init(void)
{
    if (spi_fd >= 0) return 0;

    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        perror("ADC: open SPI");
        return -1;
    }

    uint8_t mode = SPI_MODE_0, bits = 8;

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0) {
        perror("ADC: configure SPI");
        Adc_cleanup();
        return -1;
    }

    return 0;
}

void Adc_cleanup(void)
{
    if (spi_fd >= 0) close(spi_fd);
    spi_fd = -1;
}

int Adc_readChannel(int channel)
{
    if (spi_fd < 0 || channel < 0 || channel > 7)
        return -1;

    uint8_t tx[3] = {
        (uint8_t)(0x06 | ((channel & 0x04) >> 2)),
        (uint8_t)((channel & 0x03) << 6),
        0
    };

    uint8_t rx[3] = {0, 0, 0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (uintptr_t)tx,
        .rx_buf = (uintptr_t)rx,
        .len = sizeof(tx),
        .speed_hz = spi_speed,
        .bits_per_word = 8
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) != 3) {
        perror("ADC: SPI transfer");
        return -1;
    }

    return ((rx[1] & 0x0F) << 8) | rx[2];
}


