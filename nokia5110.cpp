#include "nokia5110.h"

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include <avr/pgmspace.h>


#define nokia5110_CMD  LOW
#define nokia5110_DATA HIGH


/*
 * If this was a ".h", it would get added to sketches when using
 * the "Sketch -> Import Library..." menu on the Arduino IDE...
 */
#include "charset.cpp"


nokia5110::nokia5110(unsigned char sclk, unsigned char sdin,
                 unsigned char dc, unsigned char reset,
                 unsigned char sce):
    pin_sclk(sclk),
    pin_sdin(sdin),
    pin_dc(dc),
    pin_reset(reset),
    pin_sce(sce)
{}


void nokia5110::begin(unsigned char width, unsigned char height, unsigned char model)
{
    this->width = width;
    this->height = height;

    this->column = 0;
    this->line = 0;

    // Sanitize the custom glyphs...
    memset(this->custom, 0, sizeof(this->custom));

    // All pins are outputs (these displays cannot be read)...
    pinMode(this->pin_sclk, OUTPUT);
    pinMode(this->pin_sdin, OUTPUT);
    pinMode(this->pin_dc, OUTPUT);
    pinMode(this->pin_reset, OUTPUT);
    pinMode(this->pin_sce, OUTPUT);

    // Reset the controller state...
    digitalWrite(this->pin_reset, LOW);
    delayMicroseconds(1);
    digitalWrite(this->pin_reset, HIGH);
    
    digitalWrite(this->pin_sce, LOW);
    delayMicroseconds(1);
    digitalWrite(this->pin_reset, HIGH);
    delayMicroseconds(1); 
 
   // Set the LCD parameters...
    this->send(nokia5110_CMD, 0x21);  // extended instruction set control (H=1)
    this->send(nokia5110_CMD, 0xc8);  // bias system (1:48)
    this->send(nokia5110_CMD, 0x06);  
    this->send(nokia5110_CMD, 0x13);  
    this->send(nokia5110_CMD, 0x20);  
    // Clear RAM contents...
    this->clear();
    this->send(nokia5110_CMD, 0x0c);  
    digitalWrite(this->pin_sce, LOW);
}


void nokia5110::stop()
{
    this->clear();
    this->setPower(false);
}


void nokia5110::clear()
{
    this->setCursor(0, 0);

    for (unsigned short i = 0; i < this->width * (this->height/8); i++) {
        this->send(nokia5110_DATA, 0x00);
    }

    this->setCursor(0, 0);
}


void nokia5110::clearLine()
{
    this->setCursor(0, this->line);

    for (unsigned char i = 0; i < this->width; i++) {
        this->send(nokia5110_DATA, 0x00);
    }

    this->setCursor(0, this->line);
}


void nokia5110::setPower(bool on)
{
    this->send(nokia5110_CMD, on ? 0x20 : 0x24);
}


inline void nokia5110::display()
{
    this->setPower(true);
}


inline void nokia5110::noDisplay()
{
    this->setPower(false);
}


void nokia5110::setInverse(bool inverse)
{
    this->send(nokia5110_CMD, inverse ? 0x0d : 0x0c);
}


void nokia5110::home()
{
    this->setCursor(0, this->line);
}


void nokia5110::setCursor(unsigned char column, unsigned char line)
{
    this->column = (column % this->width);
    this->line = (line % (this->height/9 + 1));

    this->send(nokia5110_CMD, 0x80 | this->column);
    this->send(nokia5110_CMD, 0x40 | this->line); 
}


void nokia5110::createChar(unsigned char chr, const unsigned char *glyph)
{
    // ASCII 0-31 only...
    if (chr >= ' ') {
        return;
    }
    
    this->custom[chr] = glyph;
}


#if ARDUINO < 100
void nokia5110::write(uint8_t chr)
#else
size_t nokia5110::write(uint8_t chr)
#endif
{
    // ASCII 7-bit only...
    if (chr >= 0x80) {
#if ARDUINO < 100
        return;
#else
        return 0;
#endif
    }

    const unsigned char *glyph;
    unsigned char pgm_buffer[5];

    if (chr >= ' ') {
        // Regular ASCII characters are kept in flash to save RAM...
        memcpy_P(pgm_buffer, &charset[chr - ' '], sizeof(pgm_buffer));
        glyph = pgm_buffer;
    } else {
        // Custom glyphs, on the other hand, are stored in RAM...
        if (this->custom[chr]) {
            glyph = this->custom[chr];
        } else {
            // Default to a space character if unset...
            memcpy_P(pgm_buffer, &charset[0], sizeof(pgm_buffer));
            glyph = pgm_buffer;
        }
    }

    // Output one column at a time...
    for (unsigned char i = 0; i <5; i++) {
        this->send(nokia5110_DATA, glyph[i]);
    }

    // One column between characters...
    this->send(nokia5110_DATA, 0x00);

    // Update the cursor position...
    this->column = (this->column + 6) % this->width;

    if (this->column == 0) {
        this->line = (this->line + 1) % (this->height/9 + 1);
    }

#if ARDUINO >= 100
    return 1;
#endif
}


void nokia5110::drawBitmap(const unsigned char *data, unsigned char columns, unsigned char lines)
{
    unsigned char scolumn = this->column;
    unsigned char sline = this->line;

    // The bitmap will be clipped at the right/bottom edge of the display...
    unsigned char mx = (scolumn + columns > this->width) ? (this->width - scolumn) : columns;
    unsigned char my = (sline + lines > this->height/8) ? (this->height/8 - sline) : lines;

    for (unsigned char y = 0; y < my; y++) {
        this->setCursor(scolumn, sline + y);

        for (unsigned char x = 0; x < mx; x++) {
            this->send(nokia5110_DATA, data[y * columns + x]);
        }
    }

    // Leave the cursor in a consistent position...
    this->setCursor(scolumn + columns, sline);
}


void nokia5110::drawColumn(unsigned char lines, unsigned char value)
{
    unsigned char scolumn = this->column;
    unsigned char sline = this->line;

    // Keep "value" within range...
    if (value > lines*8) {
        value = lines*8;
    }

    // Find the line where "value" resides...
    unsigned char mark = (lines*8 - 1 - value)/8;
    
    // Clear the lines above the mark...
    for (unsigned char line = 0; line < mark; line++) {
        this->setCursor(scolumn, sline + line);
        this->send(nokia5110_DATA, 0x00);
    }

    // Compute the byte to draw at the "mark" line...
    unsigned char b = 0xff;
    for (unsigned char i = 0; i < lines*8 - mark*8 - value; i++) {
        b <<= 1;
    }

    this->setCursor(scolumn, sline + mark);
    this->send(nokia5110_DATA, b);

    // Fill the lines below the mark...
    for (unsigned char line = mark + 1; line < lines; line++) {
        this->setCursor(scolumn, sline + line);
        this->send(nokia5110_DATA, 0xff);
    }
  
    // Leave the cursor in a consistent position...
    this->setCursor(scolumn + 1, sline); 
}


void nokia5110::send(unsigned char type, unsigned char data)
{
    digitalWrite(this->pin_dc, type);
  
    digitalWrite(this->pin_sce, LOW);
    shiftOut(this->pin_sdin, this->pin_sclk, MSBFIRST, data);
    digitalWrite(this->pin_sce, HIGH);
}
