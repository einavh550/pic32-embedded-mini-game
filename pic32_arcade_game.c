#include <xc.h>
#include <sys/attribs.h>
#include <stdlib.h>
#include <stdio.h>
#include <xc.h>

#pragma config JTAGEN = OFF
#pragma config FWDTEN = OFF
#pragma config FNOSC = FRCPLL
#pragma config FSOSCEN = OFF
#pragma config POSCMOD = EC
#pragma config OSCIOFNC = ON
#pragma config FPBDIV = DIV_1
#pragma config FPLLIDIV = DIV_2
#pragma config FPLLMUL = MUL_20
#pragma config FPLLODIV = DIV_1

#define LCD_CLEAR 0x01
#define LCD_LINE1 0x80
#define LCD_LINE2 0xC0

// RGB LED effect variables
volatile int green_blink_active = 0;
volatile int red_blink_active = 0;
volatile int red_blink_count = 0;

// Seven-segment display variables
volatile int ssd_digit = 0;
volatile int coin_display_value = 0;
unsigned char ssd_segments[16] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9

};

void lcd_cmd(unsigned char cmd);
void lcd_data(unsigned char data);
void init_lcd(void);
void lcd_write_str(const char *str);
void delay_ms(int ms);
void busy(void);
void ADC_Init(void);
unsigned int ADC_AnalogRead(unsigned char analogPIN);
void setup_pins();
int scan_keypad();
void load_custom_char_hands_down(void);
void load_custom_char_hands_up(void);
void load_custom_char_dog(void);
void load_custom_char_coin(void);
void load_custom_char_bomb(void);
void buzz_soft_beep(void);
void delay_us(unsigned int us);
void init_RGB_LED();
void init_timer5(void);
void trigger_green_blink(void);
void trigger_red_blink(void);
void init_ssd(void);
void display_coins(int coin_count);

// Timer5 ISR for RGB LED effects and Seven-segment display
void __ISR(_TIMER_5_VECTOR, ipl4auto) Timer5ISR(void)
{
    static int green_timer_count = 0;
    static int red_timer_count = 0;
    static int led_timer_count = 0;
    
    // LED effects (run at slower rate)
    led_timer_count++;
    if (led_timer_count >= 100) {  // Every 200ms for LED effects
        if (green_blink_active) {
            green_timer_count++;
            if (green_timer_count >= 3) {  // 3 x 200ms = 600ms (about half second)
                LATDbits.LATD12 = 0;       // Turn off green (RD12)
                green_blink_active = 0;
                green_timer_count = 0;
            }
        }
        
        if (red_blink_active) {
            red_timer_count++;
            if (red_timer_count >= 1) {    // 200ms intervals for fast but visible blinks
                LATDbits.LATD2 ^= 1;       // Toggle red (RD2)
                red_blink_count++;
                red_timer_count = 0;
                
                if (red_blink_count >= 6) {  // 3 complete blinks (6 toggles)
                    LATDbits.LATD2 = 0;      // Turn off red (RD2)
                    red_blink_active = 0;
                    red_blink_count = 0;
                }
            }
        }
        led_timer_count = 0;
    }
    
    // Seven-segment display multiplexing (every 2ms for smooth display)
    // Turn off all digits first
    LATBbits.LATB12 = 1;  // AN0
    LATBbits.LATB13 = 1;  // AN1
    LATAbits.LATA9 = 1;   // AN2
    LATAbits.LATA10 = 1;  // AN3
    
    // Extract digits from coin_display_value
    int thousands = (coin_display_value / 1000) % 10;
    int hundreds = (coin_display_value / 100) % 10;
    int tens = (coin_display_value / 10) % 10;
    int ones = coin_display_value % 10;
    
    // Display current digit
    switch(ssd_digit) {
        case 0:  // Display thousands (leftmost digit)
            // Set cathode segments for thousands digit
            LATGbits.LATG12 = !(ssd_segments[thousands] & 0x01);  // CA
            LATAbits.LATA14 = !(ssd_segments[thousands] & 0x02);  // CB
            LATDbits.LATD6 = !(ssd_segments[thousands] & 0x04);   // CC
            LATGbits.LATG13 = !(ssd_segments[thousands] & 0x08);  // CD
            LATGbits.LATG15 = !(ssd_segments[thousands] & 0x10);  // CE
            LATDbits.LATD7 = !(ssd_segments[thousands] & 0x20);   // CF
            LATDbits.LATD13 = !(ssd_segments[thousands] & 0x40);  // CG
            LATAbits.LATA10 = 0;  // Enable AN3 (leftmost)
            break;
        case 1:  // Display hundreds
            LATGbits.LATG12 = !(ssd_segments[hundreds] & 0x01);
            LATAbits.LATA14 = !(ssd_segments[hundreds] & 0x02);
            LATDbits.LATD6 = !(ssd_segments[hundreds] & 0x04);
            LATGbits.LATG13 = !(ssd_segments[hundreds] & 0x08);
            LATGbits.LATG15 = !(ssd_segments[hundreds] & 0x10);
            LATDbits.LATD7 = !(ssd_segments[hundreds] & 0x20);
            LATDbits.LATD13 = !(ssd_segments[hundreds] & 0x40);
            LATAbits.LATA9 = 0;   // Enable AN2
            break;
        case 2:  // Display tens
            LATGbits.LATG12 = !(ssd_segments[tens] & 0x01);
            LATAbits.LATA14 = !(ssd_segments[tens] & 0x02);
            LATDbits.LATD6 = !(ssd_segments[tens] & 0x04);
            LATGbits.LATG13 = !(ssd_segments[tens] & 0x08);
            LATGbits.LATG15 = !(ssd_segments[tens] & 0x10);
            LATDbits.LATD7 = !(ssd_segments[tens] & 0x20);
            LATDbits.LATD13 = !(ssd_segments[tens] & 0x40);
            LATBbits.LATB13 = 0;  // Enable AN1
            break;
        case 3:  // Display ones (rightmost digit)
            LATGbits.LATG12 = !(ssd_segments[ones] & 0x01);
            LATAbits.LATA14 = !(ssd_segments[ones] & 0x02);
            LATDbits.LATD6 = !(ssd_segments[ones] & 0x04);
            LATGbits.LATG13 = !(ssd_segments[ones] & 0x08);
            LATGbits.LATG15 = !(ssd_segments[ones] & 0x10);
            LATDbits.LATD7 = !(ssd_segments[ones] & 0x20);
            LATDbits.LATD13 = !(ssd_segments[ones] & 0x40);
            LATBbits.LATB12 = 0;  // Enable AN0 (rightmost)
            break;
    }
    
    ssd_digit = (ssd_digit + 1) % 4;
    
    IFS0bits.T5IF = 0;
}

void setup_pins() {
    // Configure keypad row pins as OUTPUTS
    TRISCbits.TRISC2 = 0; // x0 - RC2 is output
    TRISCbits.TRISC1 = 0; // x1 - RC1 is output
    TRISCbits.TRISC4 = 0; // x2 - RC4 is output
    TRISGbits.TRISG6 = 0; // x3 - RG6 is output
    
    // Initially set all rows HIGH
    LATCbits.LATC2 = 1;
    LATCbits.LATC1 = 1;
    LATCbits.LATC4 = 1;
    LATGbits.LATG6 = 1;

    // Configure keypad column pins as INPUTS with pull-ups
    TRISCbits.TRISC3 = 1;  // y0 - RC3 is input
    CNPUCbits.CNPUC3 = 1;  // Enable pull-up for RC3

    TRISGbits.TRISG7 = 1;  // y1 - RG7 is input
    CNPUGbits.CNPUG7 = 1;  // Enable pull-up for RG7

    TRISGbits.TRISG8 = 1;  // y2 - RG8 is input
    CNPUGbits.CNPUG8 = 1;  // Enable pull-up for RG8

    TRISGbits.TRISG9 = 1;  // y3 - RG9 is input
    CNPUGbits.CNPUG9 = 1;  // Enable pull-up for RG9

    
    ANSELGbits.ANSG6 = 0;  
    ANSELGbits.ANSG7 = 0;  
    ANSELGbits.ANSG8 = 0;  
    ANSELGbits.ANSG9 = 0;  
    
    TRISBbits.TRISB15 = 0; // RS
    TRISDbits.TRISD5 = 0;  // RW
    TRISDbits.TRISD4 = 0;  // EN
    TRISE = 0x00; // Data bus (all pins as output)
    ANSELE = 0x00;
    ANSELBbits.ANSB15 = 0;
    
    TRISBbits.TRISB14 = 0;
    ANSELBbits.ANSB14 = 0;

    
    
    ANSELEbits.ANSE2 = 0;
    ANSELEbits.ANSE4 = 0;
    ANSELEbits.ANSE5 = 0;
    ANSELEbits.ANSE6 = 0;
    
    TRISA &= 0XFF00;
    TRISFbits.TRISF3 = 1; // SW0 (RF3) for HEX counter mode
    TRISFbits.TRISF5 = 1; // SW1 (RF5) for SHIFT mode
    TRISFbits.TRISF4 = 1; // SW2 (RF4) for Fan mode
    TRISDbits.TRISD15 = 1; // SW3 (D15) for reverse mode
    
    

}

void init_RGB_LED(void)
{
    TRISDbits.TRISD2 = 0;   // Red LED
    TRISDbits.TRISD12 = 0;  // Green LED
    
    LATDbits.LATD2 = 0;     // Initially off
    LATDbits.LATD12 = 0;    // Initially off
}

void init_timer5(void)
{
    T5CONbits.ON = 0;
    T5CONbits.TGATE = 0;
    T5CONbits.TCS = 0;
    T5CONbits.TCKPS0 = 1;
    T5CONbits.TCKPS1 = 1;
    T5CONbits.TCKPS2 = 0;  // 1:16 prescaler for faster display refresh
    
    TMR5 = 0;
    PR5 = 1250;  // ~2ms intervals for smooth seven-segment display
    
    IPC5bits.T5IP = 4;
    IPC5bits.T5IS = 0;
    IFS0bits.T5IF = 0;
    IEC0bits.T5IE = 1;
    
    T5CONbits.ON = 1;
}

void trigger_green_blink(void)
{
    LATDbits.LATD12 = 1;     // Turn on green (RD12 is green)
    green_blink_active = 1;  // Will be turned off by timer
}

void trigger_red_blink(void)
{
    LATDbits.LATD2 = 1;      // Turn on red (RD2 is red)
    red_blink_active = 1;    // Will blink 3 times via timer
    red_blink_count = 0;
}

char scan_key[] = {
    0x44, '1',  0x34, '2',  0x24, '3',  0x14, 'A',
    0x43, '4',  0x33, '5',  0x23, '6',  0x13, 'B',
    0x42, '7',  0x32, '8',  0x22, '9',  0x12, 'C',
    0x41, '0',  0x31, 'F',  0x21, 'E',  0x11, 'D'
};


int scan_keypad() {
    int row, col;
    int flag = 0;

    // Row 1
    LATCbits.LATC2 = 0; // Activate row 1 (set LOW)
    delay_ms(1); // Short delay for signal to stabilize
    
    if (!PORTCbits.RC3) { col = 1; row = 1; flag = 1; }
    else if (!PORTGbits.RG7) { col = 2; row = 1; flag = 1; }
    else if (!PORTGbits.RG8) { col = 3; row = 1; flag = 1; }
    else if (!PORTGbits.RG9) { col = 4; row = 1; flag = 1; }
    
    LATCbits.LATC2 = 1; // Deactivate row (set HIGH)

    // Row 2
    if (!flag) {
        LATCbits.LATC1 = 0; // Activate row 2
        delay_ms(1);
        
        if (!PORTCbits.RC3) { col = 1; row = 2; flag = 1; }
        else if (!PORTGbits.RG7) { col = 2; row = 2; flag = 1; }
        else if (!PORTGbits.RG8) { col = 3; row = 2; flag = 1; }
        else if (!PORTGbits.RG9) { col = 4; row = 2; flag = 1; }
        
        LATCbits.LATC1 = 1; // Deactivate row
    }

    // Row 3
    if (!flag) {
        LATCbits.LATC4 = 0; // Activate row 3
        delay_ms(1);
        
        if (!PORTCbits.RC3) { col = 1; row = 3; flag = 1; }
        else if (!PORTGbits.RG7) { col = 2; row = 3; flag = 1; }
        else if (!PORTGbits.RG8) { col = 3; row = 3; flag = 1; }
        else if (!PORTGbits.RG9) { col = 4; row = 3; flag = 1; }
        
        LATCbits.LATC4 = 1; // Deactivate row
    }

    // Row 4
    if (!flag) {
        LATGbits.LATG6 = 0; // Activate row 4
        delay_ms(1);
        
        if (!PORTCbits.RC3) { col = 1; row = 4; flag = 1; }
        else if (!PORTGbits.RG7) { col = 2; row = 4; flag = 1; }
        else if (!PORTGbits.RG8) { col = 3; row = 4; flag = 1; }
        else if (!PORTGbits.RG9) { col = 4; row = 4; flag = 1; }
        
        LATGbits.LATG6 = 1; // Deactivate row
    }
    
    if (!flag) return 0; // No key detected
    
    else{ // key Detected
        
        // Wait for key release (debounce)
        while (!PORTCbits.RC3 || !PORTGbits.RG7 || !PORTGbits.RG8 || !PORTGbits.RG9) {
            delay_ms(10); // Check every 10ms
        }
        
        delay_ms(50); // Additional debounce time after release
        
        return (row << 4)|col;
    }
}

void main(void)
{
    unsigned int adc_val, display_val;
    int correct_counter = 0;

    TRISBbits.TRISB14 = 0;
    ANSELBbits.ANSB14 = 0;
    TRISBbits.TRISB15 = 0; // RS
    TRISDbits.TRISD5  = 0; // RW
    TRISDbits.TRISD4  = 0; // EN
    TRISE = 0x00;
    ANSELE = 0x00;
    ANSELBbits.ANSB15 = 0;

    TRISBbits.TRISB2 = 1;
    ANSELBbits.ANSB2 = 1;
    TRISA &= 0xFF00;
    LATA = 0;

    TRISFbits.TRISF3 = 1; // SW0

    init_lcd();
    ADC_Init();
    setup_pins();
    init_RGB_LED();
    init_ssd();
    init_timer5();
    
    // Enable interrupts
    INTCONbits.MVEC = 1;
    asm("ei");
   
    lcd_cmd(LCD_CLEAR);
    lcd_cmd(LCD_LINE1);
    lcd_write_str("Answer the hint");
    lcd_cmd(LCD_LINE2);
    lcd_write_str("101 in binary is");

    while (1)
    {
        adc_val = ADC_AnalogRead(2);
        display_val = adc_val / 4;
        PORTA = display_val;

        if (display_val >= 100 && display_val <= 102)
            correct_counter++;
        else
            correct_counter = 0;

        if (correct_counter >= 20)
        {
            lcd_cmd(LCD_CLEAR);
            lcd_cmd(LCD_LINE1);
            lcd_write_str("Correct!");
            break;
        }

        delay_ms(100);
    }

    delay_ms(3000);
    int exitGame = 1;
    int character = 0;
    int coins = 10;
    int speed;
    PORTA = 0;
    while(exitGame){
        int gameOver = 1;
        int key = 0;
        
        // Initialize display for menu
        display_coins(coins);
        
        while(key != 0x34 && key != 0x44 && key != 0x24 ){
            lcd_cmd(LCD_CLEAR);
            lcd_cmd(LCD_LINE1);
            lcd_write_str("MENU:   Play-1");
            lcd_cmd(LCD_LINE2);
            lcd_write_str("Exit-2  Store-3 ");
            
            // Keep display updated during menu
            display_coins(coins);
            delay_ms(50);  // Short delay to reduce LCD refresh rate
            
            key = scan_keypad();
        }
        if (key == 0x44){
            key = 0;
            lcd_cmd(LCD_CLEAR);
            lcd_cmd(LCD_LINE1);
            lcd_write_str("Easy press - 1");
            lcd_cmd(LCD_LINE2);
            lcd_write_str("Hard press - 2");
            delay_ms(1000);
            while (1)
            {
                key = scan_keypad();
                if (key == 0x44)
                {
                    lcd_cmd(LCD_CLEAR);
                    lcd_cmd(LCD_LINE1);
                    lcd_write_str("Easy mode selected");
                    speed = 0;
                    break;
                }
                else if (key == 0x34)
                {
                    lcd_cmd(LCD_CLEAR);
                    lcd_cmd(LCD_LINE1);
                    lcd_write_str("Hard mode selected");
                    speed = 1;
                    break;
                }
                delay_ms(100);
            }

            delay_ms(2000);
            int Score = 0;
            int prevSW0 = 0;
            int currentSW0 = 0;
            unsigned char player_row = 0xC0;
            unsigned char player_col = 0;
            unsigned char coin_col = 15;
            unsigned char coin_row = 0x80;
            unsigned char bomb_col = 10;
            unsigned char bomb_row = 0xC0;
            int toggle = 0;
            int bomb_toggle = 1;
            int ch;

            // Initialize coin display
            display_coins(coins);

            if(character == 0){ load_custom_char_hands_down(); ch = 0; }
            else if(character == 1){ load_custom_char_hands_up(); ch = 1; }
            else if(character == 2){ load_custom_char_dog(); ch = 2; }

            load_custom_char_coin();  
            load_custom_char_bomb();  

            lcd_cmd(player_row + player_col); lcd_data(ch);
            lcd_cmd(coin_row + coin_col);     lcd_data(3);
            lcd_cmd(bomb_row + bomb_col);     lcd_data(4);

            while (gameOver)
            {
                    currentSW0 = PORTFbits.RF3;

                    if (currentSW0 && !prevSW0) {
                        player_row = 0x80;
                    } else if (!currentSW0 && prevSW0) {
                        player_row = 0xC0;
                    }
                    prevSW0 = currentSW0;

                    lcd_cmd(coin_row + coin_col); lcd_data(' ');
                    lcd_cmd(bomb_row + bomb_col); lcd_data(' ');
                    lcd_cmd(player_row == 0x80 ? 0xC0 : 0x80); lcd_cmd(player_col); lcd_data(' ');

                    if (coin_col > 0) {
                        coin_col--;
                    } else {
                        coin_col = 15;
                        if (toggle == 0) { coin_row = 0x80; toggle = 1; }
                        else { coin_row = 0xC0; toggle = 0; }
                    }

                    if (bomb_col > 0) {
                        bomb_col--;
                    } else {
                        bomb_col = 15;
                        if (bomb_toggle == 0) { bomb_row = 0xC0; bomb_toggle = 1; }
                        else { bomb_row = 0x80; bomb_toggle = 0; }
                    }

                    
                    lcd_cmd(player_row + player_col); lcd_data(ch);
                    lcd_cmd(coin_row + coin_col);     lcd_data(3);
                    lcd_cmd(bomb_row + bomb_col);     lcd_data(4);

                    
                    if (coin_col == player_col && coin_row == player_row) {
                        coins++;
                        display_coins(coins);  // Update seven-segment display
                        buzz_soft_beep();
                        trigger_green_blink();  // Trigger green blink for coin collection
                    }

                    
                    if (bomb_col == player_col && bomb_row == player_row) {
                        buzz_soft_beep();
                        trigger_red_blink();    // Trigger red triple blink for bomb hit
                        lcd_cmd(LCD_CLEAR);
                        lcd_write_str("BOOM! Game Over");
                        delay_ms(2000);
                        gameOver = 0;
                        break;
                    }

                    if(speed == 1){
                        delay_ms(180);
                    }
                    else{
                        delay_ms(400);
                    }
            }

        }
        else if (key == 0x34){
            exitGame = 0;
            lcd_cmd(LCD_CLEAR);
            lcd_cmd(LCD_LINE1);
            lcd_write_str("Good_Bye ");
            delay_ms(2000);
        }
        else if(key == 0x24){
            lcd_cmd(LCD_CLEAR);
            
            key = 0;

            load_custom_char_hands_down(); // 0 
            load_custom_char_hands_up();   // 1
            load_custom_char_dog();        // 2

            lcd_cmd(LCD_LINE1 + 0);
            lcd_write_str("0C");

            lcd_cmd(LCD_LINE1 + 6);
            lcd_write_str("5C");

            lcd_cmd(LCD_LINE1 + 12);
            lcd_write_str("4C");

            lcd_cmd(0xC0 + 0);
            lcd_data(0);

            lcd_cmd(0xC0 + 6);
            lcd_data(1);

            lcd_cmd(0xC0 + 12);
            lcd_data(2);

            while(1)
            {
                // Keep display stable during store browsing
                display_coins(coins);
                delay_ms(10);
                
                key = scan_keypad();
                if(key == 0x42 && coins >= 2){
                    character = 0;
                    display_coins(coins);  // Update display immediately
                    lcd_cmd(LCD_CLEAR);
                    lcd_write_str("chosen Char 1!");
                    break;
                } else if(key == 0x43 && coins >= 5){
                    coins -= 5;
                    display_coins(coins);  // Update display immediately after purchase
                    character = 1;
                    lcd_cmd(LCD_CLEAR);
                    lcd_write_str("Bought Char 2!");
                    break;
                } else if(key == 0x44 && coins >= 4){
                    coins -= 4;
                    display_coins(coins);  // Update display immediately after purchase
                    character = 2;
                    lcd_cmd(LCD_CLEAR);
                    lcd_write_str("Bought Char 3!");
                    break;
                } else if(key == 0x44 || key == 0x43 || key == 0x42){
                    lcd_cmd(LCD_CLEAR);
                    lcd_write_str("Not enough coins");
                    delay_ms(1500);
                    break;
                }
            }
            delay_ms(2000);
        }
    }
}

void load_custom_char_bomb(void)
{
    unsigned char bomb[8] = {
        0x04,
        0x0A,
        0x15,
        0x0E,
        0x0E,
        0x1F,
        0x04,
        0x0A
    };

    lcd_cmd(0x50 + 16);
    for (int i = 0; i < 8; i++) {
        lcd_data(bomb[i]);
    }
}

void load_custom_char_dog(void)
{
    unsigned char dog[8] = {
        0x00,
        0x0A,
        0x1F,
        0x15,
        0x1F,
        0x04,
        0x0A,
        0x11
    };

    lcd_cmd(0x50);
    for (int i = 0; i < 8; i++) {
        lcd_data(dog[i]);
    }
}

void load_custom_char_coin(void) {
    unsigned char coin[8] = {
        0b00000,
        0b00110,
        0b01111,
        0b01111,
        0b01111,
        0b01111,
        0b00110,
        0b00000
    };
    
    lcd_cmd(0x50 + 8);
    for (int i = 0; i < 8; i++) {
        lcd_data(coin[i]);
    }
}

void load_custom_char_hands_down(void)
{
    unsigned char man[8] = {
        0x0E,
        0x0E,
        0x04,
        0x04,
        0x0E,
        0x15,
        0x04,
        0x0A
    };

    lcd_cmd(0x40); 
    for (int i = 0; i < 8; i++) {
        lcd_data(man[i]);
    }
}

void load_custom_char_hands_up(void)
{
    unsigned char man[8] = {
        0x0E,   
        0x04,    
        0x15,   
        0x0E,   
        0x04, 
        0x04,  
        0x0A,    
        0x11      
    };

    lcd_cmd(0x40 + 8);
    for (int i = 0; i < 8; i++) {
        lcd_data(man[i]);
    }
}

void buzz_soft_beep(void)
{
    for (int i = 0; i < 100; i++) {
        PORTBbits.RB14 = 1;
        delay_us(300); 
        PORTBbits.RB14 = 0;
        delay_us(300);
    }
}

void delay_us(unsigned int us)
{
    for (int i = 0; i < us; i++) {
        for (volatile int j = 0; j < 2; j++); 
        
    }
}

void init_ssd(void)
{
    // Configure anode pins as outputs
    TRISBbits.TRISB12 = 0;  // AN0
    TRISBbits.TRISB13 = 0;  // AN1
    TRISAbits.TRISA9 = 0;   // AN2
    TRISAbits.TRISA10 = 0;  // AN3
    
    // Configure cathode pins as outputs
    TRISGbits.TRISG12 = 0;  // CA
    TRISAbits.TRISA14 = 0;  // CB
    TRISDbits.TRISD6 = 0;   // CC
    TRISGbits.TRISG13 = 0;  // CD
    TRISGbits.TRISG15 = 0;  // CE
    TRISDbits.TRISD7 = 0;   // CF
    TRISDbits.TRISD13 = 0;  // CG
    
    // Disable analog functionality for AN0 and AN1
    ANSELBbits.ANSB12 = 0;
    ANSELBbits.ANSB13 = 0;
    
    // Initialize all digits off (anodes high)
    LATBbits.LATB12 = 1;   // AN0
    LATBbits.LATB13 = 1;   // AN1
    LATAbits.LATA9 = 1;    // AN2
    LATAbits.LATA10 = 1;   // AN3
    
    // Initialize all segments off (cathodes high for common anode)
    LATGbits.LATG12 = 1;   // CA
    LATAbits.LATA14 = 1;   // CB
    LATDbits.LATD6 = 1;    // CC
    LATGbits.LATG13 = 1;   // CD
    LATGbits.LATG15 = 1;   // CE
    LATDbits.LATD7 = 1;    // CF
    LATDbits.LATD13 = 1;   // CG
}

void display_coins(int coin_count)
{
    coin_display_value = coin_count;
}


void init_lcd(void)
{
    lcd_cmd(0x38); 
    lcd_cmd(0x0C); 
    lcd_cmd(0x06); 
    lcd_cmd(0x01); 
    delay_ms(250);
}

void lcd_write_str(const char *str)
{
    while (*str)
        lcd_data(*str++);
}

void lcd_cmd(unsigned char cmd)
{
    PORTBbits.RB15 = 0; // RS = 0
    PORTDbits.RD5 = 0;  // RW = 0
    PORTE = cmd;
    PORTDbits.RD4 = 1;
    PORTDbits.RD4 = 0;
    busy();
}

void lcd_data(unsigned char data)
{
    PORTBbits.RB15 = 1; // RS = 1
    PORTDbits.RD5 = 0;  // RW = 0
    PORTE = data;
    PORTDbits.RD4 = 1;
    PORTDbits.RD4 = 0;
    busy();
}



void delay_ms(int ms)
{
    int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 800; j++);
}



void busy(void)
{
    char RD, RS;
    int STATUS_TRISE;
    int portMap;

    RD = PORTDbits.RD5;
    RS = PORTBbits.RB15;
    STATUS_TRISE = TRISE;
    PORTDbits.RD5 = 1; // RW = 1
    PORTBbits.RB15 = 0; // RS = 0
    portMap = TRISE;
    portMap |= 0x80; // RE7
    TRISE = portMap;

    do {
        PORTDbits.RD4 = 1;
        _asm_ volatile("nop");
        PORTDbits.RD4 = 0;
    } while (PORTEbits.RE7);

    PORTDbits.RD5 = RD;
    PORTBbits.RB15 = RS;
    TRISE = STATUS_TRISE;
}

// === ADC ===
void ADC_Init()
{
    AD1CON1 = 0;
    AD1CON1bits.SSRC = 7;
    AD1CON1bits.FORM = 0;
    AD1CSSL = 0;
    AD1CON3 = 0x0002;
    AD1CON2 = 0;
    AD1CON2bits.VCFG = 0;
    AD1CON1bits.ON = 1;
}

unsigned int ADC_AnalogRead(unsigned char analogPIN)
{
    int adc_val = 0;
    AD1CHS = analogPIN << 16;
    AD1CON1bits.SAMP = 1;
    while (AD1CON1bits.SAMP);
    while (!AD1CON1bits.DONE);
    adc_val = ADC1BUF0;
    return adc_val;
}
