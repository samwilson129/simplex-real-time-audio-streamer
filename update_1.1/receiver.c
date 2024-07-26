#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "portaudio.h"

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define SERVER_PORT 12345

PaStream *stream;
int sockfd;
struct sockaddr_in serverAddr, clientAddr;
socklen_t addrLen;

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

ssize_t recv_packet_with_ack(void *buffer, size_t length) {
    char packet[FRAMES_PER_BUFFER * sizeof(float) + sizeof(unsigned short)];
    ssize_t receivedBytes = recvfrom(sockfd, packet, length + sizeof(unsigned short), 0,
                                     (struct sockaddr *)&clientAddr, &addrLen);
    if (receivedBytes < 0) {
        perror("recvfrom");
        return -1;
    }

    unsigned short receivedChecksum;
    memcpy(&receivedChecksum, packet + length, sizeof(receivedChecksum));

    unsigned short calculatedChecksum = calculate_checksum(packet, length);
    if (calculatedChecksum != receivedChecksum) {
        fprintf(stderr, "Checksum mismatch\n");
        return -1;
    }

    char ack[] = "ACK";
    if (sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)&clientAddr, addrLen) < 0) {
        perror("sendto");
        return -1;
    }

    memcpy(buffer, packet, length);
    return length;
}

static int audioCallback(const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData) {
    float *out = (float *)outputBuffer;
    ssize_t receivedBytes = recv_packet_with_ack(out, framesPerBuffer * sizeof(float));
    if (receivedBytes < 0) {
        fprintf(stderr, "Failed to receive packet\n");
        return paContinue;
    }
    return paContinue;
}

int main() {
    PaError err;
    PaStreamParameters outputParameters;

    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        goto error;
    }

    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = 1;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream, NULL, &outputParameters, SAMPLE_RATE,
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
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        goto error;
    }

    addrLen = sizeof(clientAddr);

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        goto error;
    }

    printf("Receiving and Playing audio. Press Enter to stop...\n");
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

