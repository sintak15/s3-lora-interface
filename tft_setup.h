#define USER_SETUP_LOADED
#define USER_SETUP_INFO "s3-lora-interface ILI9341"

#define ILI9341_DRIVER
#define TFT_INVERSION_ON
#define USE_HSPI_PORT

#define TFT_CS   10
#define TFT_DC   46
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_MISO 13
#define TFT_RST  -1
#define TFT_BL   45
#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SMOOTH_FONT

#define SPI_FREQUENCY       27000000
#define SPI_READ_FREQUENCY  20000000
