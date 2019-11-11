#ifndef QSPI_H
#define QPI_H
#include "common_defs.h"
#include <stdio.h>
#include <string.h>

#define QSPI_DUAL_MODE 1

//Для доступа по 8бит
#define QUADSPI_DR_8b          (*(__IO uint8_t *)((uint32_t)QUADSPI+0x020))
//Для доступа по 16бит
#define QUADSPI_DR_16b         (*(__IO uint16_t *)((uint32_t)QUADSPI+0x020))


/* QSPI_CR settings */
#define QSPI_PSC 2
#define QSPI_FIFO 2
#define QSPI_SHIFT 0

/*
Размер памяти
В режиме DUAL суммарный размер
*/
#define QSPI_FLASH_SIZE_2MB 21
#define QSPI_FLASH_SIZE_4MB 22
#define QSPI_FLASH_SIZE_8MB 23
#define QSPI_FLASH_SIZE_16MB 24
#define QSPI_FLASH_SIZE_32MB 25
#define QSPI_FLASH_SIZE_64MB 26
#define QSPI_FLASH_SIZE_128MB 27
#define QSPI_FLASH_SIZE_256MB 28

#define QPSI_DSHIFT 0

#define MODE_NO  0
#define MODE_1L  1 
#define MODE_2L  2 
#define MODE_4L  3 

#define QSPI_CCR_ADSIZE_24 2

#define QSPI_IND_READ  1
#define QSPI_IND_WRITE 0
#define QSPI_MEMMAP    3
#define QSPI_POLL      2


#define QSPI_BUSY_FLAG 0
#define QSPI_BUSY_MASK (uint32_t)(1 << 0)|(1 << 8)

#define QSPI_WE_FLAG   (uint32_t)(1 << 1)|(1 << 9)
#define QSPI_WE_MASK (uint32_t)(1 << 1)|(1 << 9)

/********* W25Qxxx command ***********/
/* write enable/disable cmd */
#define W25Q_WRITE_ENABLE     0x06
#define W25Q_WRITE_ENVSR      0x50//volatile status register
#define W25Q_WRITE_DISABLE    0x04
/* read status register */
#define W25Q_READ_SR1         0x05
#define W25Q_READ_SR2         0x35
#define W25Q_READ_SR3         0x15
/* write status register */
#define W25Q_WRITE_SR1        0x01
#define W25Q_WRITE_SR2        0x31
#define W25Q_WRITE_SR3        0x11
/* read id */
#define W25Q_READID           0x90
#define W25Q_READID_DSPI      0x92
#define W25Q_READID_QSPI      0x94
/* QSPI Enable &Disable */
#define W25Q_QSPI_ENABLE      0x38
#define W25Q_QSPI_DISABLE     0xff
/* address mode */
#define W25Q_ADDR_3BYTE       0xe9
#define W25Q_ADDR_4BYTE       0xb7
/* fast read */
#define W25Q_READ             0x03
#define W25Q_FAST_READ_4OUT   0x6B
#define W25Q_FAST_READ_4IO    0xEB
#define W25Q_FAST_READ4B      0x0c
/* page program */
#define W25Q_PAGE_PROGRAM     0x02
/* sector erase */
#define W25Q_SECTOR_ERASE_4K  0x20
#define W25Q_SECTOR_ERASE_32K 0x52
#define W25Q_SECTOR_ERASE_64K 0xD8
#define W25Q_CHIP_ERASE       0xC7
/* set read parameter */
#define W25Q_READ_PARA        0xc0
/* reset */
#define W25Q_RESET_ENABLE	  0x66
#define W25Q_RESET			  0x99


#define QUADSPI_WAIT_BUSY while (QUADSPI->SR & QUADSPI_SR_BUSY){__NOP();}

void qspi_init(void);
void qspi_gpio_init(void);

void qspi_inwrite_cmd(uint8_t cmd);
void qspi_inread_cmd(uint8_t *data, uint32_t size, uint32_t addr, uint8_t cmd);

void qspi_write_data(uint8_t *data, uint32_t size, uint32_t addr, uint8_t cmd);
void qspi_write(uint8_t *data,uint32_t size,uint32_t addr);

uint16_t qspi_read_sr(uint8_t cmd);
void enable_mem_mapping(void);

void qspi_chip_erase(void);

void qspi_wait_flag(uint32_t flag, uint32_t mask, uint8_t cmd);

#endif