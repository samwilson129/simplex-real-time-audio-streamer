#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include "portaudio.h"

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define SERVER_PORT 12345
#define SERVER_IP "192.168.0.167" //change to receiver VM ip addresss
#define TIMEOUT_USEC 50000

PaStream *stream;
int sockfd;
struct sockaddr_in serverAddr;

unsigned short calculate_checksum(const char *data, size_t length) {
    unsigned long sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}

int send_packet_with_ack(const void *data, size_t length) {
    char packet[FRAMES_PER_BUFFER * sizeof(float) + sizeof(unsigned short)];
    memcpy(packet, data, length);

    unsigned short checksum = calculate_checksum(packet, length);
    memcpy(packet + length, &checksum, sizeof(checksum));

    struct timeval timeout;
    fd_set readfds;
    ssize_t sentBytes, recvBytes;

    while (1) {
        sentBytes = sendto(sockfd, packet, length + sizeof(checksum), 0,
                           (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("sendto");
            return -1;
        }

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT_USEC;

        int retval = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (retval == -1) {
            perror("select");
            return -1;
        } else if (retval > 0) {
            char ack[4];
            recvBytes = recvfrom(sockfd, ack, sizeof(ack), 0, NULL, NULL);
            if (recvBytes > 0 && strcmp(ack, "ACK") == 0) {
                return 0;
            }
        }

        printf("Retransmitting packet...\n");
    }
}

static int audioCallback(const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData) {
    if (inputBuffer != NULL) {
        if (send_packet_with_ack(inputBuffer, framesPerBuffer * sizeof(float)) < 0) {
            fprintf(stderr, "Failed to send packet\n");
        }
    }
    return paContinue;
}

int main() {
    PaError err;
    PaStreamParameters inputParameters;

    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        goto error;
    }

    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default input device.\n");
        goto error;
    }
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultHighInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream, &inputParameters, NULL, SAMPLE_RATE,
                        FRAMES_PER_BUFFER, paClipOff, audioCallback, NULL);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        goto error;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        goto error;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        goto error;
    }

    printf("Recording and Sending audio. Press Enter to stop...\n");
    getchar();

    err = Pa_StopStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        goto error;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        goto error;
    }

    Pa_Terminate();
    close(sockfd);
    printf("PortAudio terminated.\n");

    return 0;

error:
    if (stream) {
        Pa_AbortStream(stream);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
    if (sockfd >= 0) {
        close(sockfd);
    }
    fprintf(stderr, "An error occurred.\n");
    return 1;
}

