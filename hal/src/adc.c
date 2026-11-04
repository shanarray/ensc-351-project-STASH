// hal/src/adc.c
#include "hal/adc.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

static bool is_initialized = false;
static int spi_fd = -1;
static const char *SPI_DEV = "/dev/spidev0.0";
static uint32_t SPI_SPEED = 250000;

void Adc_init(void) {
    assert(!is_initialized);
    
    spi_fd = open(SPI_DEV, O_RDWR);
    if (spi_fd < 0) {
        perror("ADC: Failed to open SPI device");
        return;
    }

    uint8_t mode = 0;
    uint8_t bits = 8;

    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &SPI_SPEED);

    is_initialized = true;
}

void Adc_cleanup(void) {
    assert(is_initialized);
    if (spi_fd >= 0) {
        close(spi_fd);
        spi_fd = -1;
    }
    is_initialized = false;
}

int Adc_readChannel(int channel) {
    assert(is_initialized);
    if (channel < 0 || channel > 7) return -1;

    uint8_t tx[3] = {
        (uint8_t)(0x06 | ((channel & 0x04) >> 2)),
        (uint8_t)((channel & 0x03) << 6),
        0x00
    };
    uint8_t rx[3] = {0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = SPI_SPEED,
        .bits_per_word = 8,
        .cs_change = 0
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        return -1;
    }

    return ((rx[1] & 0x0F) << 8) | rx[2];
}