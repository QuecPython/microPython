enum {
    PORT_A,
    PORT_B,
    PORT_C,
    PORT_D,
    PORT_E,
    PORT_Z,
};

enum {
    AF_FN_FTM,
    AF_FN_I2C,
    AF_FN_UART,
    AF_FN_SPI
};

enum {
    AF_PIN_TYPE_FTM_CH0 = 0,
    AF_PIN_TYPE_FTM_CH1,
    AF_PIN_TYPE_FTM_CH2,
    AF_PIN_TYPE_FTM_CH3,
    AF_PIN_TYPE_FTM_CH4,
    AF_PIN_TYPE_FTM_CH5,
    AF_PIN_TYPE_FTM_CH6,
    AF_PIN_TYPE_FTM_CH7,
    AF_PIN_TYPE_FTM_QD_PHA,
    AF_PIN_TYPE_FTM_QD_PHB,

    AF_PIN_TYPE_I2C_SDA = 0,
    AF_PIN_TYPE_I2C_SCL,

    AF_PIN_TYPE_SPI_MOSI = 0,
    AF_PIN_TYPE_SPI_MISO,
    AF_PIN_TYPE_SPI_SCK,
    AF_PIN_TYPE_SPI_NSS,

    AF_PIN_TYPE_UART_TX = 0,
    AF_PIN_TYPE_UART_RX,
    AF_PIN_TYPE_UART_CTS,
    AF_PIN_TYPE_UART_RTS,
};

typedef GPIO_TypeDef pin_gpio_t;
