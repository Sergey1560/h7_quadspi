#include "qspi.h"

uint32_t data32;
uint16_t data16;
uint8_t data8;

uint8_t ALGN4 buffer[300];

void qspi_init(void){

    qspi_gpio_init();
    RCC->AHB3ENR |= RCC_AHB3ENR_QSPIEN;

    /* 
    QSPI Settings 
    DFM mode, 2 W25Q16JV
    */
    QUADSPI->CR |=  (QSPI_FIFO << QUADSPI_CR_FTHRES_Pos)|\
                    (QUADSPI_CR_APMS)|\
                    (QUADSPI_CR_DFM)|\
                    ((QSPI_PSC-1) << QUADSPI_CR_PRESCALER_Pos)|\
                    (QSPI_SHIFT << QUADSPI_CR_SSHIFT_Pos);
    
    QUADSPI->DCR |= ((QSPI_FLASH_SIZE_4MB-1) << QUADSPI_DCR_FSIZE_Pos)|
                    (QPSI_DSHIFT << QUADSPI_DCR_CSHT_Pos);

    QUADSPI->CR |=QUADSPI_CR_EN;

    /* Reset */
    qspi_inwrite_cmd(W25Q_RESET_ENABLE);
    for(uint32_t i=0;i<0x1000;i++){__NOP();}
    qspi_inwrite_cmd(W25Q_RESET);

    /* 1. Read SR */
    data16 = qspi_read_sr(W25Q_READ_SR1);
    DEBUG("SR1 %d - %d",(data16 & 0xFF),((data16 >> 8) & 0xFF));
    qspi_inwrite_cmd(W25Q_WRITE_ENABLE);
    data16 = qspi_read_sr(W25Q_READ_SR1);
    DEBUG("SR1 WE %d - %d",(data16 & 0xFF),((data16 >> 8) & 0xFF));
    qspi_inwrite_cmd(W25Q_WRITE_DISABLE);
    data16 = qspi_read_sr(W25Q_READ_SR1);
    DEBUG("SR1 WE off %d - %d",(data16 & 0xFF),((data16 >> 8) & 0xFF));
    data16 = qspi_read_sr(W25Q_READ_SR2);
    DEBUG("SR2 %d - %d",(data16 & 0xFF),((data16 >> 8) & 0xFF));
    data16 = qspi_read_sr(W25Q_READ_SR3);
    DEBUG("SR3 %d - %d",(data16 & 0xFF),((data16 >> 8) & 0xFF));
    qspi_inread_cmd((uint8_t *)&data32, 4, 0, W25Q_READID);
    INFO("Manuf. ID1 %X ID2 %X\nDevice ID1 %X ID2 %X",(data32 & 0xFF), ((data32 >> 8) & 0xFF), ((data32 >> 16) & 0xFF), ((data32 >> 24) & 0xFF));

    for(uint32_t i=0; i<300; i++){
        if(i<256){
            buffer[i]=i;
        }else{
            buffer[i]=i-256;
        }
    }

    DEBUG("Start chip erase");
    qspi_chip_erase();
    DEBUG("End chip erase");
    DEBUG("Start data write");
    qspi_write((uint8_t*)&buffer,300,310);
    DEBUG("End data write");
    
    enable_mem_mapping();
    INFO("QSPI INIT Done");
};


void qspi_chip_erase(void){
    DEBUG("WRITE ENABLE");
    qspi_inwrite_cmd(W25Q_WRITE_ENABLE);
    DEBUG("WAIT FLAG WE");
    qspi_wait_flag(QSPI_WE_FLAG,QSPI_WE_MASK,W25Q_READ_SR1);
    DEBUG("Start chip erase");
    qspi_inwrite_cmd(W25Q_CHIP_ERASE);
    DEBUG("Stop chip erase, wait flag");
    qspi_wait_flag(QSPI_BUSY_FLAG,QSPI_BUSY_MASK,W25Q_READ_SR1);
}


void enable_mem_mapping(void){
    while(QUADSPI->SR & QUADSPI_SR_BUSY){};
    QUADSPI->ABR = 0xF1;
    QUADSPI->CCR =  (W25Q_FAST_READ_4IO << QUADSPI_CCR_INSTRUCTION_Pos)|\
                    (4 << QUADSPI_CCR_DCYC_Pos)|\
                    (MODE_4L << QUADSPI_CCR_ADMODE_Pos)|\
                    (MODE_4L << QUADSPI_CCR_ABMODE_Pos)|\
                    (QSPI_CCR_ADSIZE_24 << QUADSPI_CCR_ADSIZE_Pos)|\
                    (MODE_1L << QUADSPI_CCR_IMODE_Pos)|\
                    (MODE_4L << QUADSPI_CCR_DMODE_Pos)|\
                    (QSPI_MEMMAP << QUADSPI_CCR_FMODE_Pos);
}


void qspi_write(uint8_t *data,uint32_t size,uint32_t addr){
    uint16_t check_erase;
    uint32_t align_count;
    
    qspi_inread_cmd((uint8_t *)&check_erase, 2, addr, W25Q_READ);
    if(check_erase != 0xFFFF){
        ERROR("Write to not erased area at addr 0x%X",addr);
        return;
    } 

    if((addr % 256) != 0){
        align_count = ((addr/256)+1)*256 - addr;
        qspi_write_data(data, align_count, addr, W25Q_PAGE_PROGRAM);  
        size-=align_count;
        data+=align_count;
        addr+=align_count;
    }

    while (size)
    {
      if(size > 256){
          qspi_write_data(data, 256, addr, W25Q_PAGE_PROGRAM);  
          size-=256;
          data+=256;
          addr+=256;
        }else{
          qspi_write_data(data, size, addr, W25Q_PAGE_PROGRAM);
          size=0;
        }
   
        qspi_wait_flag(QSPI_BUSY_FLAG,QSPI_BUSY_MASK,W25Q_READ_SR1);
    }
}


void qspi_write_data(uint8_t *data, uint32_t size, uint32_t addr, uint8_t cmd){

    qspi_inwrite_cmd(W25Q_WRITE_ENABLE);
    qspi_wait_flag(QSPI_WE_FLAG,QSPI_WE_MASK,W25Q_READ_SR1);
    
    QUADSPI->DLR = size-1;
    QUADSPI->CCR =  (cmd << QUADSPI_CCR_INSTRUCTION_Pos)|\
                    (0 << QUADSPI_CCR_DCYC_Pos)|\
                    (MODE_1L << QUADSPI_CCR_ADMODE_Pos)|\
                    (QSPI_CCR_ADSIZE_24 << QUADSPI_CCR_ADSIZE_Pos)|\
                    (MODE_1L << QUADSPI_CCR_IMODE_Pos)|\
                    (MODE_1L << QUADSPI_CCR_DMODE_Pos);
    QUADSPI->AR = addr;

    for(uint32_t k=0;k<size;k++){
        QUADSPI_DR_8b=*data;
        data++;
    }

    qspi_wait_flag(QSPI_BUSY_FLAG,QSPI_BUSY_MASK,W25Q_READ_SR1);
};

void qspi_inread_cmd(uint8_t *data, uint32_t size, uint32_t addr, uint8_t cmd){
   
    while(QUADSPI->SR & QUADSPI_SR_BUSY){};

    QUADSPI->DLR = size-1;
    QUADSPI->CCR =  (cmd << QUADSPI_CCR_INSTRUCTION_Pos)|\
                    (0 << QUADSPI_CCR_DCYC_Pos)|\
                    (MODE_1L << QUADSPI_CCR_ADMODE_Pos)|\
                    (QSPI_CCR_ADSIZE_24 << QUADSPI_CCR_ADSIZE_Pos)|\
                    (MODE_1L << QUADSPI_CCR_IMODE_Pos)|\
                    (MODE_1L << QUADSPI_CCR_DMODE_Pos)|\
                    (QSPI_IND_READ << QUADSPI_CCR_FMODE_Pos);
    QUADSPI->AR = addr;

    for(uint32_t k=0;k<size;k++){
        *data=QUADSPI_DR_8b;
        data++;
    }

}


void qspi_wait_flag(uint32_t flag, uint32_t mask, uint8_t cmd){

    while(QUADSPI->SR & QUADSPI_SR_BUSY){};

    QUADSPI->DLR = 1;

    QUADSPI->PSMKR = mask;
    QUADSPI->PSMAR = flag;

    QUADSPI->CCR =  (cmd << QUADSPI_CCR_INSTRUCTION_Pos)|\
                    (MODE_1L << QUADSPI_CCR_DMODE_Pos)|\
                    (MODE_1L << QUADSPI_CCR_IMODE_Pos)|\
                    (QSPI_POLL << QUADSPI_CCR_FMODE_Pos);  //Старт записью инструкции в режиме READ
  
    while(!(QUADSPI->SR & QUADSPI_SR_SMF)){};
    QUADSPI->FCR = (QUADSPI_FCR_CTCF|QUADSPI_FCR_CSMF);
    DEBUG("Flag get");
}


void qspi_inwrite_cmd(uint8_t cmd){

    DEBUG("WRITE CMD");
    
    while(QUADSPI->SR & QUADSPI_SR_BUSY){};

    qspi_wait_flag(QSPI_BUSY_FLAG,QSPI_BUSY_MASK,W25Q_READ_SR1);
    QUADSPI->CCR =  (cmd << QUADSPI_CCR_INSTRUCTION_Pos)|\
                    (MODE_1L << QUADSPI_CCR_IMODE_Pos);
    
    while (!(QUADSPI->SR & QUADSPI_SR_TCF)){__NOP();}
    QUADSPI->FCR = QUADSPI_FCR_CTCF;
}


uint16_t qspi_read_sr(uint8_t cmd){
    uint16_t data;

    while(QUADSPI->SR & QUADSPI_SR_BUSY){};

    QUADSPI->DLR = 1;
    QUADSPI->CCR =  (cmd << QUADSPI_CCR_INSTRUCTION_Pos)|\
                    (MODE_1L << QUADSPI_CCR_DMODE_Pos)|\
                    (MODE_1L << QUADSPI_CCR_IMODE_Pos)|\
                    (QSPI_IND_READ << QUADSPI_CCR_FMODE_Pos);  //Старт записью инструкции в режиме READ
  
    while (!(QUADSPI->SR & QUADSPI_SR_TCF)){__NOP();}
    QUADSPI->FCR = QUADSPI_FCR_CTCF;
    data = QUADSPI_DR_16b;
    return data;
}



void qspi_gpio_init(void){

/*  
PB2  CLK      AF9
PB10 BK1_CS   AF9

PF6  BK1_IO3  AF9
PF7  BK1_IO2  AF9
PF8  BK1_IO0  AF10
PF9  BK1_IO1  AF10

PG9  BK2_IO2  AF9
PG14 BK2_IO3  AF9

PH2  BK2_IO0  AF9
PH3  BK2_IO1  AF9

AF 9

*/

    GPIO_TypeDef *PORT;

    RCC->AHB4ENR|=( RCC_AHB4ENR_GPIOBEN|\
                    RCC_AHB4ENR_GPIOFEN|\
                    RCC_AHB4ENR_GPIOGEN|\
                    RCC_AHB4ENR_GPIOHEN);
                    

    /****************************************** GPIOB **************************************/
    /* 2,10 */
    PORT=GPIOB;
    //Сброс бит
    PORT->MODER &= ~(GPIO_MODER_MODER2|\
                     GPIO_MODER_MODER10);
    //Режим AF
    PORT->MODER |= (GPIO_MODER_MODER2_1|\
                    GPIO_MODER_MODER10_1);
    
    //Сброс бит в режим OUT_PP
    PORT->OTYPER &= ~(GPIO_OTYPER_OT_2|\
                      GPIO_OTYPER_OT_10);
    
    //Сброс бит
    PORT->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR2|\
                       GPIO_OSPEEDER_OSPEEDR10);

    //Скорость 
    PORT->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR2|\
                       GPIO_OSPEEDER_OSPEEDR10);

    PORT->OSPEEDR |= (S_VH << GPIO_OSPEEDER_OSPEEDR2_Pos)|\
                     (S_VH << GPIO_OSPEEDER_OSPEEDR10_Pos);

    //Подтяжки отключены
    PORT->PUPDR &= ~(GPIO_PUPDR_PUPDR2|\
                     GPIO_PUPDR_PUPDR10);

    //AF 9
    PORT->AFR[0] &= ~(GPIO_AFRL_AFRL2);
    PORT->AFR[1] &= ~(GPIO_AFRH_AFRH10);

    PORT->AFR[0] |= (9 << GPIO_AFRL_AFRL2_Pos);
    PORT->AFR[1] |= (9 << GPIO_AFRH_AFRH10_Pos);

    /****************************************** GPIOF **************************************/
    /* 6,7,8,9 */
    PORT=GPIOF;
    //Сброс бит
    PORT->MODER &= ~(GPIO_MODER_MODER6|\
                     GPIO_MODER_MODER7|\
                     GPIO_MODER_MODER8|\
                     GPIO_MODER_MODER9);
    //Режим AF
    PORT->MODER |= (GPIO_MODER_MODER6_1|\
                    GPIO_MODER_MODER7_1|\
                    GPIO_MODER_MODER8_1|\
                    GPIO_MODER_MODER9_1);
    
    //Сброс бит в режим OUT_PP
    PORT->OTYPER &= ~(GPIO_OTYPER_OT_6|\
                      GPIO_OTYPER_OT_7|\
                      GPIO_OTYPER_OT_8|\
                      GPIO_OTYPER_OT_9);
    
    //Сброс бит
    PORT->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR6|\
                       GPIO_OSPEEDER_OSPEEDR7|\
                       GPIO_OSPEEDER_OSPEEDR8|\
                       GPIO_OSPEEDER_OSPEEDR9);

    //Скорость 
    PORT->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR6|\
                       GPIO_OSPEEDER_OSPEEDR7|\
                       GPIO_OSPEEDER_OSPEEDR8|\
                       GPIO_OSPEEDER_OSPEEDR9);

    PORT->OSPEEDR |= (S_VH << GPIO_OSPEEDER_OSPEEDR6_Pos)|\
                     (S_VH << GPIO_OSPEEDER_OSPEEDR7_Pos)|\
                     (S_VH << GPIO_OSPEEDER_OSPEEDR8_Pos)|\
                     (S_VH << GPIO_OSPEEDER_OSPEEDR9_Pos);

    //Подтяжки отключены
    PORT->PUPDR &= ~(GPIO_PUPDR_PUPDR6|\
                     GPIO_PUPDR_PUPDR7|\
                     GPIO_PUPDR_PUPDR8|\
                     GPIO_PUPDR_PUPDR9);

    //AF 9
    PORT->AFR[0] &= ~(GPIO_AFRL_AFRL6|GPIO_AFRL_AFRL7);
    PORT->AFR[1] &= ~(GPIO_AFRH_AFRH8|GPIO_AFRH_AFRH9);

    PORT->AFR[0] |= (9 << GPIO_AFRL_AFRL6_Pos)|\
                    (9 << GPIO_AFRL_AFRL7_Pos);

    //AF10
    PORT->AFR[1] |= (10 << GPIO_AFRH_AFRH8_Pos)|\
                    (10 << GPIO_AFRH_AFRH9_Pos);

#ifdef QSPI_DUAL_MODE
    /****************************************** GPIOG **************************************/
    /* 9,14 */

    PORT=GPIOG;
    //Сброс бит
    PORT->MODER &= ~(GPIO_MODER_MODER9|\
                     GPIO_MODER_MODER14);
    //Режим AF
    PORT->MODER |= (GPIO_MODER_MODER9_1|\
                    GPIO_MODER_MODER14_1);
    
    //Сброс бит в режим OUT_PP
    PORT->OTYPER &= ~(GPIO_OTYPER_OT_9|\
                      GPIO_OTYPER_OT_14);
    
    //Сброс бит
    PORT->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR9|\
                       GPIO_OSPEEDER_OSPEEDR14);

    //Скорость 
    PORT->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR9|\
                       GPIO_OSPEEDER_OSPEEDR14);

    PORT->OSPEEDR |= (S_VH << GPIO_OSPEEDER_OSPEEDR9_Pos)|\
                     (S_VH << GPIO_OSPEEDER_OSPEEDR14_Pos);

    //Подтяжки отключены
    PORT->PUPDR &= ~(GPIO_PUPDR_PUPDR9|\
                     GPIO_PUPDR_PUPDR14);

    //AF 9
    
    PORT->AFR[1] &= ~(GPIO_AFRH_AFRH9|GPIO_AFRH_AFRH14);
    PORT->AFR[1] |= (9 << GPIO_AFRH_AFRH9_Pos)|(9 << GPIO_AFRH_AFRH14_Pos);


    /****************************************** GPIOH **************************************/
    /* 2,3 */

    PORT=GPIOH;
    //Сброс бит
    PORT->MODER &= ~(GPIO_MODER_MODER2|\
                     GPIO_MODER_MODER3);
    //Режим AF
    PORT->MODER |= (GPIO_MODER_MODER2_1|\
                    GPIO_MODER_MODER3_1);
    
    //Сброс бит в режим OUT_PP
    PORT->OTYPER &= ~(GPIO_OTYPER_OT_2|\
                      GPIO_OTYPER_OT_3);
    
    //Сброс бит
    PORT->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR2|\
                       GPIO_OSPEEDER_OSPEEDR3);

    //Скорость 
    PORT->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR2|\
                       GPIO_OSPEEDER_OSPEEDR3);

    PORT->OSPEEDR |= (S_VH << GPIO_OSPEEDER_OSPEEDR2_Pos)|\
                     (S_VH << GPIO_OSPEEDER_OSPEEDR3_Pos);

    //Подтяжки отключены
    PORT->PUPDR &= ~(GPIO_PUPDR_PUPDR2|\
                     GPIO_PUPDR_PUPDR3);

    //AF 9
    PORT->AFR[0] &= ~(GPIO_AFRL_AFRL2|GPIO_AFRL_AFRL3);
    PORT->AFR[0] |= (9 << GPIO_AFRL_AFRL2_Pos)|(9 << GPIO_AFRL_AFRL3_Pos);
    #endif 
}