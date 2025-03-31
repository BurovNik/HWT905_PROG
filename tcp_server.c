#include "ports.h"


bool start_TCP_server(int *server_fd, struct sockaddr_in *address, int *opt, int *adrlen)
{
    // Создаем TCP-сокет
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    
    // Настраиваем опции сокета (переиспользование порта)
    if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, opt, sizeof(*opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;  // Принимаем подключения с любого IP
    address->sin_port = htons(PORT);
    
    // Привязываем сокет к порту
    if (bind(*server_fd, (struct sockaddr *)address, sizeof(*address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Начинаем слушать входящие подключения (макс. очередь = 5)
    if (listen(*server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Сервер запущен и слушает порт %d...\n", PORT);
    
}


void form_answer_buffer(char* buffer, size_t size, hwt905_values *data, int count)
{
    snprintf(buffer, size,
        "%d: Данные HWT905 | message number = %i | acceleration (%lf; %lf; %lf), "
        "MF (%i; %i; %i), Angular velocity (%lf; %lf; %lf), Temp = %lf\n",
        getpid(), count,
        data->acceleration[0], data->acceleration[1], data->acceleration[2],
        data->magneta[0], data->magneta[1], data->magneta[2],
        data->angularVelocity[0], data->angularVelocity[1], data->angularVelocity[2],
        data->temperature);
}


bool send_data( hwt905_values *data, int client_socket)
{
    char response[1024];
    form_answer_buffer(response, sizeof(response), data, 1);
    if (send(client_socket, response, strlen(response), 0) != strlen(response))
    {
        printf("Ошика при отправке\n");
        return false;
    }
    printf("Сообщение успешно отправлено");
    return true;
}

