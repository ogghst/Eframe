#ifndef EPD_DRIVER_H
#define EPD_DRIVER_H

// Display resolution
#define EPD_WIDTH       800
#define EPD_HEIGHT      480

void EPD_Init(void);
void EPD_Clear(void);
void EPD_Display(const unsigned char* frame_buffer);
void EPD_Sleep(void);


#endif // EPD_DRIVER_H
