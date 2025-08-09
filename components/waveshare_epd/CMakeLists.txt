idf_component_register(
    SRCS 
        "epd_driver.c"
        "epd.c"
        "../arduino_esp32/epd_gui.cpp"
        "../arduino_esp32/font8.cpp"
        "../arduino_esp32/font12.cpp"
        "../arduino_esp32/font16.cpp"
        "../arduino_esp32/font20.cpp"
        "../arduino_esp32/font24.cpp"
    INCLUDE_DIRS 
        "."
        "../arduino_esp32"
    REQUIRES 
        driver
        spi_flash
)



