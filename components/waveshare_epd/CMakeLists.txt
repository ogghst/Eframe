idf_component_register(SRCS "epd.cpp"
                           "epdspi.cpp"
                           "epd4spi.cpp"
                           "models/goodisplay/gdey075T7.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES Adafruit-GFX)

