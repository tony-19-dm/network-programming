#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

const int BUFFER_SIZE = 1024;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Использование: " << argv[0] << " <IP сервера> <порт сервера>" << std::endl;
        return 1;
    }

    const char* serverIP = argv[1];
    int serverPort = atoi(argv[2]);

    int clientSocket;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE];

    // Создание UDP сокета
    if ((clientSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }

    // Настройка адреса сервера
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    
    // если не удалось преобразовать сетевой адрес, то завершаем
    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0) {
        std::cerr << "Ошибка в IP адресе" << std::endl;
        close(clientSocket);
        return 1;
    }

    // Цикл отправки данных
    for (int i = 1; i <= 5; ++i) { // Отправляем числа от 1 до 5
        std::string message = std::to_string(i);
        std::cout << "Отправка числа " << i << " на сервер..." << std::endl;

        // Отправка данных на сервер
        if (sendto(clientSocket, message.c_str(), message.size(), 0,
                   (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            std::cerr << "Ошибка при отправке данных" << std::endl;
            continue;
        }

        // Получение ответа от сервера
        int bytesReceived = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, nullptr, nullptr);
        if (bytesReceived < 0) {
            std::cerr << "Ошибка при получении данных" << std::endl;
            continue;
        }

        buffer[bytesReceived] = '\0'; // Добавляем завершающий нуль для строки
        std::cout << "Ответ от сервера: " << buffer << std::endl;

        sleep(i); // Задержка в i секунд
    }

    close(clientSocket);
    return 0;
}