#include <stdint.h>

#define sys_freq 16000000

#define RCC_Base 0x40023800
#define RCC_AHB1ENR *((volatile uint32_t*)(RCC_Base + 0x30))
// I2C lives in APB1
#define RCC_APB1ENR *((volatile uint32_t*)(RCC_Base + 0x40))
// reset APB1
#define RCC_APB1RSTR *((volatile uint32_t*)(RCC_Base + 0x20))

#define GPIOB_Base 0x40020400
#define GPIOB_MODER *((volatile uint32_t*)(GPIOB_Base + 0x00))
#define GPIOB_OTYPER *((volatile uint32_t*)(GPIOB_Base + 0x04))
#define GPIOB_OSPEEDR *((volatile uint32_t*)(GPIOB_Base + 0x08))
#define GPIOB_PUPDR *((volatile uint32_t*)(GPIOB_Base + 0x0C))
// says what kind of alternate function (H means pins 8-15)
#define GPIOB_AFRH *((volatile uint32_t*)(GPIOB_Base + 0x24))

#define I2C1_Base 0x40005400
#define I2C1_CR1 *((volatile uint32_t*)(I2C1_Base + 0x00))
#define I2C1_CCR *((volatile uint32_t*)(I2C1_Base + 0x1C))
#define I2C1_SR1 *((volatile uint32_t*)(I2C1_Base + 0x14))
#define I2C1_SR2 *((volatile uint32_t*)(I2C1_Base + 0x18))
#define I2C1_DR *((volatile uint32_t*)(I2C1_Base + 0x10))

void delay_SysTick(uint32_t delay_ms){
    // SysTick Reload Value Register (specifies value to count down from)
    volatile uint32_t* pSYST_RVR = (volatile uint32_t*)(0xE000E014);
    // beats every 1 ms
    *pSYST_RVR = (sys_freq/1000) - 1;

    volatile uint32_t* pSYST_CSR = (volatile uint32_t*)(0xE000E010);
    // tells that we're using proccessor clock
    *pSYST_CSR |= (1<<2);
    // TICKINT, I dont really understand, if bit 1 then it goes to cpu 
    // interupt handler, otherwise just deal with it as software
    *pSYST_CSR &= ~(1<<1);
    // Enables the counter
    *pSYST_CSR |= (1<<0);

    for (int i = 0; i < delay_ms; i++){
        // COUNTFLAG Returns 1 if timer counted to 0 since last time this was read.
        while((*pSYST_CSR & (1<<16)) == 0){}
    } 

    // Disables the counter
    *pSYST_CSR &= ~(1<<0);
}

void I2C1_init(){
    // enable clocks
    RCC_AHB1ENR |= (1<<1);
    RCC_APB1ENR |= (1<<21);

    // clears I2C
    RCC_APB1RSTR |= (1<<21);
    RCC_APB1RSTR &= ~(1<<21);
    

    // initailize PB8/PB9 in alternate mode
    GPIOB_MODER |= (1<<17) | (1<<19);
    GPIOB_AFRH |= (1<<2) | (1<<6);

    // set both pins to be open drain
    GPIOB_OTYPER |= (1<<8) | (1<<9);

    // set to fastest speed
    GPIOB_OSPEEDR |= (3<<16) | (3<<18);
    
    // pull up resistors
    GPIOB_PUPDR |= (1<<16) | (1<<18);
    
    // initialize I2C clock
    I2C1_CR1 &= ~(1<<0); // turn off I2C1 so it doesnt geek while configuring
    I2C1_CCR &= ~(1<<15); // standard mode
    I2C1_CCR = 80; // CCR = PCLK/(2*SCL_Freq)
    I2C1_CR1 |= (1<<0); // turn back on

}

void I2C1_send(uint8_t address, uint8_t data_byte){
    // 1-1. Set start bit
    I2C1_CR1 |= (1<<8);
    // 1-2. Wait for start bit
    while(!(I2C1_SR1 & (1<<0))){}

    // 2-1. Send address
    I2C1_DR = (address<<1); // has to be shifted over for r/w bit (w=0, r=1)
    // 2-2. Wait for address to send
    while(!(I2C1_SR1 & (1<<1))){}
        // read I2C_SR1/2 to clear ADDR bit
        int temp = I2C1_SR1;
        temp = I2C1_SR2;

    // 3-1. Wait empty data register
    while(!(I2C1_SR1 & (1<<7))){}
    // 3-2. Write byte
    I2C1_DR = data_byte;
    // 3-3. Wait for byte transfer finish
    while(!(I2C1_SR1 & (1<<2))){}

    // 4. Stop condition
    I2C1_CR1 |= (1<<9);
}

// send the 4 bit version, with the enable(E) acting as a shutter
void nybble(uint8_t bin_num, uint8_t RS_bit){
    //                      light   E-rising  write   cmd/data
    I2C1_send(0x27, bin_num|=(1<<3)|(1<<2)|(0<<1)|(RS_bit<<0));
    delay_SysTick(1);
    I2C1_send(0x27, bin_num &= ~(1<<2));
}

// breaks the 8 bits into 4 packages
void lcd_send_cmd(uint8_t cmd){
    uint8_t high = (cmd & 0xF0);//0xF0 = 11110000
    uint8_t low = (cmd << 4);

    nybble(high, 0);
    nybble(low, 0);
}

// breaks the 8 bits into 4 packages
void lcd_send_data(uint8_t data){
    uint8_t high = (data & 0xF0);//0xF0 = 11110000
    uint8_t low = (data << 4);

    nybble(high, 1);
    nybble(low, 1);
}

// based off psudeocode found in random datasheet
void LCD_4bit_init(){
    delay_SysTick(41);
    nybble(0x30, 0);
    delay_SysTick(10);
    nybble(0x30, 0);
    delay_SysTick(10);
    nybble(0x30, 0); 
    delay_SysTick(10);// finish wake up commands

    nybble(0x20, 0);// enter 4 bit mode
    lcd_send_cmd(0x28);
    lcd_send_cmd(0x10);
    lcd_send_cmd(0x0F);
    lcd_send_cmd(0x06);
}

void lcd_print_string(char* str){

    for(int i = 0; str[i] != '\0'; i++){
        lcd_send_data(str[i]);
        delay_SysTick(10);
    }
}

int main(){
    I2C1_init();
    LCD_4bit_init();

    // clear lcd with (delay very important)
    lcd_send_cmd(0b00000001);
    delay_SysTick(2);

    lcd_print_string("Hello World!");

    while(1){}
    return 0;
}