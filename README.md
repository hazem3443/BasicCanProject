# Basic Can Project

This project provides a basic CAN (Controller Area Network) communication, intended for embedded systems using an ESP platform (ESP32). The code demonstrates how to initialize the CAN driver, transmit and receive CAN messages, and implement a simple state machine for periodic message sending.

## Features

- Initialization and deinitialization of the CAN driver
- Periodic transmission of a CAN message
- Reception of CAN messages using an interrupt service routine (ISR) hook
- Simple state machine for handling transmit and wait states
- Logging via ESP-IDF's logging macros for debugging and status reporting

## File Overview

- **main/ShiftEV_CanTask.c**: Contains the core logic for CAN task initialization, message transmission, reception, and state handling.
- (Include other important files if necessary, e.g., header files or configuration files.)

## How It Works

1. **CAN Initialization**: The driver is initialized using `can_drv_init()`. If initialization fails, an error is logged and the function returns.
2. **Transmit Timer & ISR Setup**: A timer for periodic transmission is set up (`init_can_transmit_timer()`), and a receive ISR (`RXCAN_Hook`) is registered.
3. **Main Loop**: 
    - The loop checks for incoming CAN messages.
    - If a message is received, it's processed and logged.
    - Otherwise, it operates a simple state machine:
        - `TransmitState`: Sends a CAN message, logs the transmission, sets a delay, and switches to `WaitState`.
        - `WaitState`: Waits for the delay to expire, then returns to `TransmitState`.
        - Any unexpected state resets to `TransmitState`.
4. **Deinitialization**: When the loop exits (though in practice, this is an infinite loop), the CAN driver is deinitialized.

## Example CAN Message

The default transmitted CAN message is:
- **CAN ID**: `CAN_IDENTIFIER`
- **Data**: `{0xBE, 0xEF, 0xAB, 0xAB, 0x00, 0x00, 0x00, 0x00}`
- **Data Length**: `DATA_LENGTH`

## Prerequisites

- ESP-IDF development environment (for ESP32 or compatible device)
- Basic knowledge of CAN bus and embedded C programming

## Building and Running

1. Clone this repository:
    ```sh
    git clone https://github.com/hazem3443/BasicCanProject.git
    ```
2. Set up your ESP-IDF environment (see [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)).
3. Configure the project and hardware-specific settings as needed.
4. Build and flash the firmware to your device:
    ```sh
    idf.py build
    idf.py flash
    idf.py monitor
    ```

## Customization

- Modify the CAN message parameters in `CanTask.c` as needed.
- Extend or integrate the state machine for additional application logic.

## License

(Add your license here, e.g., MIT, Apache 2.0, etc.)

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.
