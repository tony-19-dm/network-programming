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
#include <sys/select.h>

const int BUFFER_SIZE = 1024;
const int MAX_CLIENTS = 10;

void reaper(int sig) {
    int status;
    while (wait3(&status, WNOHANG, (struct rusage*)0) >= 0);
}

int main() {
    int listener, client_socket[MAX_CLIENTS], max_sd, activity, valread, sd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    char buf[BUFFER_SIZE];
    fd_set readfds;

    // Инициализация всех клиентских сокетов в 0
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

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

    if (getsockname(listener, (struct sockaddr*)&serverAddr, &clientAddrLen) < 0 ){
        std::cerr << "Ошибка при получении информации о сокете" << std::endl;
        close(listener);
        return 1;
    } 
    std::cout << "Сервер запущен на порту: " << ntohs(serverAddr.sin_port) << std::endl;

    signal(SIGCHLD, reaper);

    while (true) {
        // Очищаем набор файловых дескрипторов
        FD_ZERO(&readfds);

        // Добавляем основной сокет в набор
        FD_SET(listener, &readfds);
        max_sd = listener;

        // Добавляем клиентские сокеты в набор
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Ожидаем активности на одном из сокетов
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            std::cerr << "Ошибка select" << std::endl;
        }

        // Если есть новое соединение, обрабатываем его
        if (FD_ISSET(listener, &readfds)) {
            int new_socket = accept(listener, (struct sockaddr *)&clientAddr, &clientAddrLen);
            if (new_socket < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // Добавляем новый сокет в массив клиентских сокетов
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    std::cout << "Добавлен новый клиент [" << inet_ntoa(clientAddr.sin_addr) << ":"
                              << ntohs(clientAddr.sin_port) << "]" << std::endl;
                    break;
                }
            }
        }

        // Обрабатываем ввод от клиентов
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (FD_ISSET(sd, &readfds)) {
                valread = recv(sd, buf, BUFFER_SIZE, 0);
                if (valread == 0) {
                    // Клиент отключился
                    getpeername(sd, (struct sockaddr*)&clientAddr, &clientAddrLen);
                    std::cout << "Клиент отключен [" << inet_ntoa(clientAddr.sin_addr) << ":"
                              << ntohs(clientAddr.sin_port) << "]" << std::endl;
                    close(sd);
                    client_socket[i] = 0;
                } else {
                    // Обрабатываем данные от клиента
                    buf[valread] = '\0';
                    std::cout << "Получено от клиента [" << inet_ntoa(clientAddr.sin_addr) << ":"
                              << ntohs(clientAddr.sin_port) << "]: " << buf << std::endl;

                    int number = atoi(buf);
                    int transformedNumber = number * 2;

                    std::string response = "Преобразованное число: " + std::to_string(transformedNumber);
                    send(sd, response.c_str(), response.size(), 0);
                }
            }
        }
    }

    close(listener);
    return 0;
}