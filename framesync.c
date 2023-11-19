#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// CRC16 CCITT calculation
uint16_t crc16_ccitt(const uint8_t *data, size_t size) {
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)data[i] << 8;

        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

// Frame Synchronizer class
typedef struct {
    uint8_t syncWord[4];  // Change the sync word size as needed
    uint8_t *buffer;
    size_t bufferSize;
    size_t bytesRead;
} FrameSynchronizer;

// Initialize the Frame Synchronizer
void initFrameSynchronizer(FrameSynchronizer *synchronizer, const uint8_t *syncWord, size_t bufferSize) {
    // Assuming sync word size is 4 bytes
    for (size_t i = 0; i < 4; ++i) {
        synchronizer->syncWord[i] = syncWord[i];
    }

    synchronizer->buffer = (uint8_t *)malloc(bufferSize);
    synchronizer->bufferSize = bufferSize;
    synchronizer->bytesRead = 0;
}

// Process a byte in the input stream
void processByte(FrameSynchronizer *synchronizer, uint8_t byte) {
    if (synchronizer->bytesRead < 4) {
        // Check for sync word
        printf("Check for sync word: %x\n", byte);
        if (byte == synchronizer->syncWord[synchronizer->bytesRead]) {
            synchronizer->bytesRead++;
        } else {
            synchronizer->bytesRead = 0;
        }
    } else {
        // Add byte to buffer
        if (synchronizer->bytesRead - 4 < synchronizer->bufferSize) {
            synchronizer->buffer[synchronizer->bytesRead - 4] = byte;
            synchronizer->bytesRead++;
            printf("Add byte to buffer %d\n", byte);
        } else {
            // Buffer full, handle the frame
            uint16_t calculatedCRC = crc16_ccitt(synchronizer->buffer, synchronizer->bytesRead - 4);
            // Assuming CRC is a 2-byte field at the end of the frame
            uint16_t receivedCRC = (uint16_t)(byte << 8);
            synchronizer->bytesRead = 0;  // Reset for the next frame

            if (calculatedCRC == receivedCRC) {
                // Process the frame (you can do something with the frame data here)
                printf("Frame processed successfully!\n");
            } else {
                // Handle CRC error
                printf("CRC error!\n");
            }
        }
    }
}

// Clean up resources
void cleanupFrameSynchronizer(FrameSynchronizer *synchronizer) {
    free(synchronizer->buffer);
}

int main() {
    // Example usage
    FrameSynchronizer synchronizer;
    const uint8_t syncWord[] = {0xAA, 0xBB, 0xCC, 0xDD};  // Change the sync word as needed
    const size_t bufferSize = 128;  // Change the buffer size as needed

    initFrameSynchronizer(&synchronizer, syncWord, bufferSize);

    // Simulate receiving a byte stream
    uint8_t byteStream[] = {0x11, 0x22, 0x33, 0x44, 0xAA, 0xBB, 0xCC, 0xDD, 0x01, 0x02, 0x03, 0x04};
    size_t streamSize = sizeof(byteStream) / sizeof(byteStream[0]);

    for (size_t i = 0; i < streamSize; ++i) {
        processByte(&synchronizer, byteStream[i]);
    }

    printf("Success\n");

    cleanupFrameSynchronizer(&synchronizer);

    return 0;
}
