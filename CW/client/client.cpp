#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>

const int BUFFER_SIZE = 1024;

void receiveMessages(int socket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while ((bytesReceived = recv(socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << "\n" << buffer << "\n> " << std::flush;
    }

    std::cout << "\n[Соединение с сервером потеряно]\n";
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Использование: " << argv[0] << " <IP сервера> <порт сервера>\n";
        return 1;
    }

    const char* serverIP = argv[1];
    int serverPort = atoi(argv[2]);

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Ошибка создания сокета\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка подключения к серверу\n";
        close(clientSocket);
        return 1;
    }

    std::cout << "Подключено к чату. Введите сообщения (для выхода: /exit):\n";

    std::thread recvThread(receiveMessages, clientSocket);

    std::string message;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, message);

        if (message == "/exit") {
            break;
        }

        send(clientSocket, message.c_str(), message.length(), 0);
    }

    close(clientSocket);
    recvThread.join();
    return 0;
}

// g++ client.cpp -o client -pthread

// ./client 127.0.0.1 52429