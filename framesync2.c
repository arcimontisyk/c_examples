#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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

// Queue Node
typedef struct Node {
    uint8_t *data;
    size_t size;
    struct Node *next;
} Node;

// Input Queue
typedef struct {
    Node *front;
    Node *rear;
} InputQueue;

// Output Queue
typedef struct {
    Node *front;
    Node *rear;
} OutputQueue;

// Frame Synchronizer class
typedef struct {
    uint8_t syncWord[4];  // Change the sync word size as needed
    uint8_t *buffer;
    size_t bufferSize;
    size_t bytesRead;
    InputQueue inputQueue;
    OutputQueue outputQueue;
} FrameSynchronizer;

// Initialize the Input Queue
void initInputQueue(InputQueue *queue) {
    queue->front = queue->rear = NULL;
}

// Enqueue a byte array to the Input Queue
void enqueueInputQueue(InputQueue *queue, const uint8_t *data, size_t size) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = (uint8_t *)malloc(size);
    newNode->size = size;
    newNode->next = NULL;

    for (size_t i = 0; i < size; ++i) {
        newNode->data[i] = data[i];
    }

    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

// Dequeue a byte array from the Input Queue
bool dequeueInputQueue(InputQueue *queue, uint8_t **data, size_t *size) {
    if (queue->front == NULL) {
        return false; // Queue is empty
    }

    Node *temp = queue->front;
    *data = temp->data;
    *size = temp->size;

    queue->front = temp->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(temp);

    return true;
}

// Clean up the Input Queue
void cleanupInputQueue(InputQueue *queue) {
    while (queue->front != NULL) {
        Node *temp = queue->front;
        queue->front = temp->next;
        free(temp->data);
        free(temp);
    }
}

// Initialize the Output Queue
void initOutputQueue(OutputQueue *queue) {
    queue->front = queue->rear = NULL;
}

// Enqueue a byte array to the Output Queue
void enqueueOutputQueue(OutputQueue *queue, const uint8_t *data, size_t size) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = (uint8_t *)malloc(size);
    newNode->size = size;
    newNode->next = NULL;

    for (size_t i = 0; i < size; ++i) {
        newNode->data[i] = data[i];
    }

    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

// Dequeue a byte array from the Output Queue
bool dequeueOutputQueue(OutputQueue *queue, uint8_t **data, size_t *size) {
    if (queue->front == NULL) {
        return false; // Queue is empty
    }

    Node *temp = queue->front;
    *data = temp->data;
    *size = temp->size;

    queue->front = temp->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(temp);

    return true;
}

// Clean up the Output Queue
void cleanupOutputQueue(OutputQueue *queue) {
    while (queue->front != NULL) {
        Node *temp = queue->front;
        queue->front = temp->next;
        free(temp->data);
        free(temp);
    }
}

// Initialize the Frame Synchronizer
void initFrameSynchronizer(FrameSynchronizer *synchronizer, const uint8_t *syncWord, size_t bufferSize) {
    // Assuming sync word size is 4 bytes
    for (size_t i = 0; i < 4; ++i) {
        synchronizer->syncWord[i] = syncWord[i];
    }

    synchronizer->buffer = (uint8_t *)malloc(bufferSize);
    synchronizer->bufferSize = bufferSize;
    synchronizer->bytesRead = 0;
    initInputQueue(&synchronizer->inputQueue);
    initOutputQueue(&synchronizer->outputQueue);
}

// Process a byte in the input stream
void processByte(FrameSynchronizer *synchronizer, uint8_t byte) {
    if (synchronizer->bytesRead < 4) {
        // Check for sync word
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
        } else {
            // Buffer full, handle the frame
            uint16_t calculatedCRC = crc16_ccitt(synchronizer->buffer, synchronizer->bytesRead - 4);
            // Assuming CRC is a 2-byte field at the end of the frame
            uint16_t receivedCRC = (uint16_t)(byte << 8);
            synchronizer->bytesRead = 0;  // Reset for the next frame

            if (calculatedCRC == receivedCRC) {
                // Process the frame (you can do something with the frame data here)
                enqueueOutputQueue(&synchronizer->outputQueue, synchronizer->buffer, synchronizer->bytesRead - 4);
            } else {
                // Handle CRC error
                printf("CRC error!\n");
            }
        }
    }
}

// Process the input queue
void processInputQueue(FrameSynchronizer *synchronizer) {
    uint8_t *data;
    size_t size;

    while (dequeueInputQueue(&synchronizer->inputQueue, &data, &size)) {
        for (size_t i = 0; i < size; ++i) {
            processByte(synchronizer, data[i]);
        }
        free(data);  // Free the memory allocated for the input data
    }
}

// Clean up resources
void cleanupFrameSynchronizer(FrameSynchronizer *synchronizer) {
    cleanupInputQueue(&synchronizer->inputQueue);
    cleanupOutputQueue(&synchronizer->outputQueue);
    free(synchronizer->buffer);
}

int main() {
    // Example usage
    FrameSynchronizer synchronizer;
    const uint8_t syncWord[] = {0xAA, 0xBB, 0xCC, 0xDD};  // Change the sync word as needed
    const size_t bufferSize = 128;  // Change the buffer size as needed

    initFrameSynchronizer(&synchronizer, syncWord, bufferSize);

    // Simulate receiving byte streams of different lengths
    uint8_t byteStream1[] = {0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0x01, 0x02, 0x03, 0x04};
    uint8_t byteStream2[] = {0xAA, 0xBB, 0xCC, 0xDD, 0x05, 0x06, 0x07, 0x08};
    size_t streamSize1 = sizeof(byteStream1) / sizeof(byteStream1[0]);
    size_t streamSize2 = sizeof(byteStream2) / sizeof(byteStream2[0]);

    // Enqueue byte streams to the input queue
    enqueueInputQueue(&synchronizer.inputQueue, byteStream1, streamSize1);
    enqueueInputQueue(&synchronizer.inputQueue, byteStream2, streamSize2);

    // Process the input queue
    processInputQueue(&synchronizer);

    // Dequeue processed frames from the output queue
    uint8_t *outputData;
    size_t outputSize;

    printf("Get frames now\n");
    while (dequeueOutputQueue(&synchronizer.outputQueue, &outputData, &outputSize)) {
        // Do something with the processed frame (e.g., print or further processing)
        printf("Processed Frame: ");
        for (size_t i = 0; i < outputSize; ++i) {
            printf("%02X ", outputData[i]);
        }
        printf("\n");

        free(outputData);  // Free the memory allocated for the output data
    }

    cleanupFrameSynchronizer(&synchronizer);

    return 0;
}
