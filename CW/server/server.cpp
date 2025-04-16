#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <algorithm>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

const int BUFFER_SIZE = 1024;
std::vector<int> clients;
std::mutex clients_mutex;

void broadcastMessage(const std::string& message, int senderSocket) {
    std::lock_guard<std::mutex> lock(clients_mutex); /*Поскольку все потоки используют общий вектор clients, 
                                                        нужно защитить его от одновременного доступа*/
    for (int client : clients) {
        if (client != senderSocket) {
            send(client, message.c_str(), message.length(), 0);
        }
    }
}

void handleClient(int clientSocket, sockaddr_in clientAddr) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived <= 0) {
            std::cout << "Клиент отключился: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
            break;
        }

        buffer[bytesReceived] = '\0';
        std::string message = "[" + std::string(inet_ntoa(clientAddr.sin_addr)) + ":" + std::to_string(ntohs(clientAddr.sin_port)) + "] " + buffer;
        std::cout << message << std::endl;
        broadcastMessage(message, clientSocket);
    }

    // Удаление клиента
    {
        std::lock_guard<std::mutex> lock(clients_mutex); // используем mutex аналогично с broadcastMessage
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
    }
    close(clientSocket);
}

int main() {
    int listenerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenerSocket < 0) {
        std::cerr << "Ошибка создания сокета\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(0); // Автоматический выбор порта
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenerSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка привязки сокета\n";
        close(listenerSocket);
        return 1;
    }

    socklen_t addrLen = sizeof(serverAddr);
    getsockname(listenerSocket, (sockaddr*)&serverAddr, &addrLen);
    std::cout << "Сервер запущен на порту: " << ntohs(serverAddr.sin_port) << std::endl;

    listen(listenerSocket, 5);

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(listenerSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            std::cerr << "Ошибка подключения клиента\n";
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(clientSocket);
        }

        std::cout << "Клиент подключился: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
        std::thread t(handleClient, clientSocket, clientAddr);
        t.detach(); //делает поток фоновой задачей, которая работает независимо от main()
    }

    close(listenerSocket);
    return 0;
}

// g++ server.cpp -o server -pthread