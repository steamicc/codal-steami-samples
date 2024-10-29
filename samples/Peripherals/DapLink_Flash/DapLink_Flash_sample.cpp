#include "DapLink_Flash_sample.h"

#include <cctype>
#include <cstdio>

constexpr uint8_t ADDRESS               = 0x76;
constexpr uint8_t FLASH_WHO_AM_I        = 0x01;
constexpr uint8_t FLASH_APPEND_FILE     = 0x11;
constexpr uint8_t FLASH_CLEAR_FILE      = 0x10;
constexpr uint8_t FLASH_SET_FILENAME    = 0x03;
constexpr uint8_t FLASH_GET_FILENAME    = 0x04;
constexpr uint8_t FLASH_READ_STATUS_REG = 0x80;
constexpr uint8_t FLASH_READ_ERROR_REG  = 0x81;

bool state_led                          = false;

void DAPLINK_FLASH_Sample(codal::STeaMi& steami)
{
    steami.init();
    steami.serial.init(115'200);

    steami.io.speaker.setDigitalValue(0);  // Force HP low state...

    auto i2c = steami.i2cInt;

    printf("Ready !\r\n");
    codal::fiber_sleep(1'000);

    char c;
    while (1) {
        steami.sleep(100);

        c = static_cast<char>(steami.serial.getChar(codal::ASYNC));

        if ((steami.serial.getChar(codal::ASYNC) != DEVICE_NO_DATA)) {
            // flush rx buffer
            target_wait(10);
            while (steami.serial.getChar(codal::ASYNC) != DEVICE_NO_DATA) target_wait(10);

            if (c < 0x20) continue;

            printf("--- Recv '%c'\n", toupper(c));

            switch ((char)toupper(c)) {
                case 'A': {
                    printf("Data to append (max 256 char): \n");
                    codal::ManagedString str = steami.serial.readUntil('\n', codal::SYNC_SPINWAIT);

                    i2c.beginTransmission(ADDRESS);
                    i2c.write(FLASH_APPEND_FILE);
                    for (int16_t i = 0; i < str.length() && i < 256; ++i) {
                        i2c.write(uint8_t(str.charAt(i)));
                    }
                    i2c.endTransmission();
                    break;
                }

                    // Read a sector (256 bytes) seems to break the I2C bus. This is not an issue, because there is no
                    // reason to read a secto for the moment.
                    // case 'R': {
                    //     i2c.beginTransmission(ADDRESS);
                    //     i2c.write(0x20);
                    //     i2c.write(0x00);
                    //     i2c.endTransmission();

                    //     auto result = i2c.read(ADDRESS, 256);
                    //     printf("Sector 0 data (%d):\n'%.256s'\n", result.size(), result.data());

                    //     break;
                    // }

                case 'W': {
                    i2c.beginTransmission(ADDRESS);
                    i2c.write(FLASH_WHO_AM_I);
                    i2c.endTransmission();

                    auto result = i2c.read(ADDRESS, 1);
                    printf("Who Am I = 0x%02X\n", result[0]);
                    break;
                }

                case 'C':
                    i2c.beginTransmission(ADDRESS);
                    i2c.write(FLASH_CLEAR_FILE);
                    i2c.endTransmission();
                    break;

                case 'F': {
                    printf(
                        "Set the filename (format: FFFFFFFFEEE (F: filename, E: ext) use ' ' (space) to fill unsused \
                        characters)): \n");
                    codal::ManagedString str = steami.serial.readUntil('\n', codal::SYNC_SPINWAIT);

                    printf("--- filename: (size: %d) '%s' ", str.length(), str.toCharArray());

                    for (int i = 0; i < str.length(); ++i) {
                        printf("0x%02X ", unsigned(str.charAt(i)));
                    }
                    printf("\n");

                    i2c.beginTransmission(ADDRESS);
                    i2c.write(FLASH_SET_FILENAME);
                    for (uint16_t i = 0; i < str.length(); ++i) {
                        i2c.write(uint8_t(str.charAt(i)));
                    }
                    i2c.endTransmission();
                    break;
                }

                case 'G': {
                    i2c.beginTransmission(ADDRESS);
                    i2c.write(FLASH_GET_FILENAME);
                    i2c.endTransmission();

                    auto result = i2c.read(ADDRESS, 11);
                    printf("filename = '%.11s'\n", result.data());
                    break;
                }

                case 'S': {
                    i2c.beginTransmission(ADDRESS);
                    i2c.write(FLASH_READ_STATUS_REG);
                    i2c.endTransmission();

                    auto status_1 = i2c.read(ADDRESS, 1);
                    printf("Status = 0x%02X\n", status_1[0]);

                    i2c.beginTransmission(ADDRESS);
                    i2c.write(FLASH_READ_ERROR_REG);
                    i2c.endTransmission();

                    auto status_err = i2c.read(ADDRESS, 1);
                    printf("Error status = 0x%02X\n", status_err[0]);
                    break;
                }

                case 'L': {
                    uint16_t total   = 200;
                    uint16_t success = 0;
                    uint16_t failure = 0;

                    printf("Start Flash/I2C stress test (reading the filename %d times)\n", total);

                    for (uint16_t i = 0; i < total; i++) {
                        i2c.beginTransmission(ADDRESS);
                        i2c.write(FLASH_GET_FILENAME);
                        i2c.endTransmission();

                        auto result = i2c.read(ADDRESS, 11);

                        if (result[0] == 0x00) {
                            failure++;
                        }
                        else {
                            success++;
                        }

                        steami.sleep(10);
                    }

                    printf("Test done (success: %d/%d (%d %%) -- failure: %d/%d (%d %%)\n", success, total,
                           success * 100 / total, failure, total, failure * 100 / total);
                    break;
                }

                default:
                    printf(
                        "USAGE:\n\tW: Who Am I\n\
                        \tF: Set filename\n\
                        \tG: Get filename\n\n\
                        \tA: Apped data to file\n\
                        \tC: Clear data file\n\n\
                        \tS: Read Status & Error register\n\n\
                        \tL: Start stress test\n");
                    break;
            }
        }
    }
}