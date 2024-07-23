#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "portaudio.h"

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define SERVER_PORT 12345

PaStream *stream;
int sockfd;
struct sockaddr_in serverAddr, clientAddr;
socklen_t addrLen;
pthread_t sendThread, recvThread;

static int audioCallback(const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData)
{
    float *out = (float *)outputBuffer;
    ssize_t receivedBytes = recvfrom(sockfd, out, framesPerBuffer * sizeof(float), 0,
                                     (struct sockaddr *)&clientAddr, &addrLen);
    if (receivedBytes < 0) {
        perror("recvfrom");
        return paContinue;
    }
    return paContinue;
}

void *sendAudio(void *arg)
{
    float buffer[FRAMES_PER_BUFFER];
    memset(buffer, 0, sizeof(buffer));
    
    while (1) {
        ssize_t sentBytes = sendto(sockfd, buffer, FRAMES_PER_BUFFER * sizeof(float), 0,
                                   (struct sockaddr *)&clientAddr, addrLen);
        if (sentBytes < 0) {
            perror("sendto");
        }
    }
    return NULL;
}

int main()
{
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

    if (pthread_create(&sendThread, NULL, sendAudio, NULL) != 0) {
        perror("pthread_create");
        goto error;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        goto error;
    }

    printf("Receiving and Playing audio. Press Enter to stop...\n");
    getchar(); // Wait for Enter key press

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
    pthread_cancel(sendThread);
    pthread_join(sendThread, NULL);
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
