#ifndef nokia5110_H
#define nokia5110_H


#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif


// Chip variants supported...
#define CHIP_nokia5110 0
#define CHIP_ST7576  1


class nokia5110: public Print {
    public:
        // All the pins can be changed from the default values...
        nokia5110(unsigned char sclk  = 0,   /* clock       (display pin 2) */
                unsigned char sdin  = 1,   /* data-in     (display pin 3) */
                unsigned char dc    = 2,   /* data select (display pin 4) */
                unsigned char reset = 3,   /* reset       (display pin 8) */
                unsigned char sce   = 4);  /* enable      (display pin 5) */

        // Display initialization (dimensions in pixels)...
        void begin(unsigned char width=84, unsigned char height=48, unsigned char model=CHIP_nokia5110);
        void stop();

        // Erase everything on the display...
        void clear();
        void clearLine();  // ...or just the current line
        
        // Control the display's power state...
        void setPower(bool on);

        // For compatibility with the LiquidCrystal library...
        void display();
        void noDisplay();

        // Activate white-on-black mode (whole display)...
        void setInverse(bool inverse);

        // Place the cursor at the start of the current line...
        void home();

        // Place the cursor at position (column, line)...
        void setCursor(unsigned char column, unsigned char line);

        // Assign a user-defined glyph (5x8) to an ASCII character (0-31)...
        void createChar(unsigned char chr, const unsigned char *glyph);

        // Write an ASCII character at the current cursor position (7-bit)...
#if ARDUINO < 100
        virtual void write(uint8_t chr);
#else        
        virtual size_t write(uint8_t chr);
#endif

        // Draw a bitmap at the current cursor position...
        void drawBitmap(const unsigned char *data, unsigned char columns, unsigned char lines);

        // Draw a chart element at the current cursor position...
        void drawColumn(unsigned char lines, unsigned char value);

    private:
        unsigned char pin_sclk;
        unsigned char pin_sdin;
        unsigned char pin_dc;
        unsigned char pin_reset;
        unsigned char pin_sce;

        // The size of the display, in pixels...
        unsigned char width;
        unsigned char height;

        // Current cursor position...
        unsigned char column;
        unsigned char line;

        // User-defined glyphs (below the ASCII space character)...
        const unsigned char *custom[' '];

        // Send a command or data to the display...
        void send(unsigned char type, unsigned char data);
};


#endif
