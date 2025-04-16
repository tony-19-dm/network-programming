#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>

const int BUFFER_SIZE = 1024;

void reaper(int sig) {
    int status;
    while (wait3(&status, WNOHANG, (struct rusage*)0) >= 0);
}

int main() {
    int serverSocket, listener;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    socklen_t serverAddrLen = sizeof(serverAddr);
    char buf[BUFFER_SIZE];
    int bytes_read;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = 0;  // 0 означает, что порт будет выбран автоматически
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        close(listener);
        return 1;
    }

    listen(listener, 3);

    if (getsockname(listener, (struct sockaddr*)&serverAddr, &serverAddrLen) < 0 ){
        std::cerr << "Ошибка при получении информации о сокете" << std::endl;
        close(listener);
        return 1;
    } 
    std::cout << "Сервер запущен на порту: " << ntohs(serverAddr.sin_port) << std::endl;

    
    signal(SIGCHLD, reaper);

    while (1) {
        serverSocket = accept(listener, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (serverSocket < 0) {
            perror("accept");
            exit(3);
        }

        switch (fork()) {
        case -1:
            perror("fork");
            break;

        case 0:
            close(listener);
            while (1) {
                bytes_read = recv(serverSocket, buf, BUFFER_SIZE, 0);
                if (bytes_read <= 0) break;
                buf[bytes_read] = '\0';
                std::cout << "Получено от клиента [" << inet_ntoa(clientAddr.sin_addr) << ":"
                << ntohs(clientAddr.sin_port) << "]: " << buf << std::endl;

                int number = atoi(buf);
                int transformedNumber = number * 2;

                std::string response = "Преобразованное число: " + std::to_string(transformedNumber);
                send(serverSocket, response.c_str(), response.size(), 0);
            }

            close(serverSocket);
            exit(0);

        default:
            close(serverSocket);
        }
    }

    close(listener);
    return 0;
}