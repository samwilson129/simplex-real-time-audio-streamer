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
int clientfd;
struct sockaddr_in serverAddr;
struct sockaddr_in clientAddr;
socklen_t addrLen = sizeof(clientAddr);

static int audioCallback(const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData) {
    float *out = (float *)outputBuffer;
    ssize_t receivedBytes = recv(clientfd, out, framesPerBuffer * sizeof(float), 0);
    if (receivedBytes < 0) {
        perror("recv");
        return paContinue;
    }
    return paContinue;
}

int main() {
    PaError err;
    PaStreamParameters outputParameters;
    int opt = 1;

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

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        goto error;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
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

    if (listen(sockfd, 1) < 0) {
        perror("listen");
        goto error;
    }

    clientfd = accept(sockfd, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientfd < 0) {
        perror("accept");
        goto error;
    }

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
    close(clientfd);
    close(sockfd);
    printf("PortAudio terminated.\n");

    return 0;

error:
    if (stream) {
        Pa_AbortStream(stream);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
    if (clientfd >= 0) {
        close(clientfd);
    }
    if (sockfd >= 0) {
        close(sockfd);
    }
    fprintf(stderr, "An error occurred.\n");
    return 1;
}

