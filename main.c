#include "ringBuffer.h"
#include "hwt905.h"
#include "defines.h"
#include "ports.h"

typedef struct 
{
    int *serial_port;
    uint32_t max_uart_delay;
    hwt905_values *values;
    size_t uart_buffer_read_len, uart_buffer_write_len;
    struct headname *headp;   
}uart_args;

struct command_elem
{
    uint8_t *command;
    size_t bytes;
    TAILQ_ENTRY(command_elem) entries;
};

TAILQ_HEAD(headname, command_elem);
int serial_port;
uart_args uart_args_values;
ringBuffer readRingBuffer;


void delete_command_elem(struct headname *headp) {
	
	struct command_elem *elem;
	if (TAILQ_FIRST(headp)->command != NULL)
		free(TAILQ_FIRST(headp)->command);
	elem = TAILQ_FIRST(headp);
	TAILQ_REMOVE(headp, TAILQ_FIRST(headp), entries);
	free(elem);
}

/// @brief Функция открытвает порт для взаимодествия с HWT905
/// @param path путь до порта
/// @param serial_port номер порта
/// @return возвращет 0 в случае успеха и -1 в случае неудачи
int open_serial_port(char const *path, int *serial_port) {

	*serial_port = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (*serial_port == -1) {
        perror("Ошибка открытия порта");
        return 1;
    }

    struct termios tty;

    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(*serial_port, &tty) != 0) {
        perror("Ошибка получения атрибутов порта");
        close(*serial_port);
        return 1;
    }

    // Настройка атрибутов порта
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);
    
    tty.c_cflag |= (CLOCAL | CREAD);    // Включаем приемник и игнорируем контроль модема
    tty.c_cflag &= ~PARENB;               // Без контроля четности
    tty.c_cflag &= ~CSTOPB;               // Один стоп-бит
    tty.c_cflag &= ~CSIZE;                // Очищаем маску размера
    tty.c_cflag |= CS8;                   // 8 бит данных

    tty.c_lflag &= ~ICANON;               // Не канонический режим
    tty.c_lflag &= ~ECHO;                 // Не эхо
    tty.c_lflag &= ~ECHOE;                // Не эхо ошибки
    tty.c_lflag &= ~ECHONL;               // Не эхо новой строки
    tty.c_lflag &= ~ISIG;                 // Игнорируем символы управления

    tty.c_oflag &= ~OPOST;                // Не модифицируем выходные данные

    // Применяем настройки
    if (tcsetattr(*serial_port, TCSANOW, &tty) != 0) {
        perror("Ошибка установки атрибутов порта");
        close(*serial_port);
        return 1;
    }

	// Очищаем буфер ввода
    tcflush(*serial_port, TCIFLUSH);

	return 0;
}

/// @brief фукнция вычисляет контрольную сумму для сравнения с той, которая пришла в сообщении, для проверки корректности данных
/// @param buffer указатель на данные пришедшие в сообщении
/// @param len длина сообщения
/// @return возвращает вычисленную контрольную сумму
uint8_t crc_generate(const uint8_t *const buffer, const size_t len)
{
    uint8_t sum = 0;
    for(int i = 0; i < len - 1; i++)// последний байт это как раз сумма, с ним и нужно сравнивать
    {
        sum += buffer[i];
    }
    return sum;

}

/// @brief функция для чтения ответат от hwt905. проверяет что считала функция, если прошло время больше, чем можно ожидать ответ или контрольная сумма не правильная возвращает 0
/// @param fd file descriptor
/// @param buffer указатель на буфер, куда будет производиться запись
/// @param len количество мест в буфере
/// @param start_time время начала чтения
/// @param max_time_delay максимальное ожидание ответа 
/// @return количество считаных байт или 0 в случае неудачи
size_t read_all(int fd, uint8_t *buffer, size_t len, struct timeval *start_time, uint32_t max_time_delay)
{
    size_t read_bytes = 0;
    struct timeval current_time;
    uint32_t time_delay;

    while (true)
    {
        gettimeofday(&current_time, NULL);
        read_bytes += read(fd, &buffer[read_bytes], len - read_bytes);
		//PRINTHEX8ARRAY(buffer, read_bytes);

        // читаем и проверяем, что контрольная сумма правильная
       if (read_bytes > 0 && crc_generate(buffer, read_bytes) == buffer[read_bytes - 1])
		{			
			return read_bytes;
		}
		
        //проверяем сколько нам не приходит ответ
		time_delay = (current_time.tv_sec - start_time->tv_sec) * 1000000 +
					  current_time.tv_usec - start_time->tv_usec;

        //если ответа нет слишком долго - выходим
		if (time_delay >= max_time_delay * 1000) {

			printf("\nНе получен ответ от устройства, прочитано %lu байт - ", read_bytes);
			PRINTHEX8ARRAY(buffer, read_bytes);
			read_bytes = 0;
			break;
		}
	}
	
	return read_bytes;
}

/// @brief функция для генерации сообщения в hwt905. в зависимости от контекста создает сообщение с запросом требуемых данных или калибровки.
/// @param buffer указатель на место в памяти, куда будет записано сообщение
/// @param reg_address команда, которую необходимо выполнить hwt905
/// @return возвращает длину отправляемого сообщения
size_t msg_generate_return_content(uint8_t *const buffer,  enum REQUEST_REGISTERS req_register)
{
    buffer[0] = REQUEST_PREFIX;
    buffer[1] = SECOND_REGISTER;
    buffer[2] = RSW;
    buffer[3] = req_register;
    buffer[4] = 0x00;
	return 5;
}

size_t msg_read_time(uint8_t *const buffer, const size_t buffer_len)
{
    return msg_generate_return_content(buffer, TIME_REQ);
}

size_t msg_read_acceleration(uint8_t *const buffer, const size_t buffer_len)
{
    return msg_generate_return_content(buffer, ACCELERATION_REQ);
}

size_t msg_read_angular_velocity(uint8_t *const buffer, const size_t buffer_len)
{
    return msg_generate_return_content(buffer, ANGULAR_VELONCY_REQ);
}

size_t msg_read_angle(uint8_t *const buffer, const size_t buffer_len)
{
    return msg_generate_return_content(buffer, ANGLE_REQ);
}

size_t msg_read_magnetic(uint8_t *const buffer, const size_t buffer_len)
{
    return msg_generate_return_content(buffer, MAGNETIC_REQ);
}




/// @brief парсинг полученного сообщения от HWT905
/// @param buffer текст полученного сообщения
/// @param len длина полученного сообщения
/// @param values список значений
void parse_hwt905_answer(const uint8_t *const buffer, const size_t len, hwt905_values *values) 
{

	uint8_t id;
	printf("\n-----------------------------------------\n");
	switch (buffer[1]) 
    {

	case TIME:
        //проверка контрольной суммы
        if(buffer[len-1] != crc_generate(buffer, len))
		{
			printf("\nНеверная контрольная сумма\n");
        	break;
		}
		values->YY = buffer[2];
        values->MM = buffer[3];
        values->DD = buffer[4];
        values->hh = buffer[5];
        values->ss = buffer[6];
        values->ms = (buffer[7] | (buffer[8] << 8));
        printf("Текущее время:\n  Дата: %i:%i%i\n  Время: %i:%i:%i:%i\n", values->YY, values->MM, values->DD,
            values->hh, values->mm, values->ss, values->ms);
		break;
	case ACCELERATION:
        //проверка контрольной суммы
        if(buffer[len-1] != crc_generate(buffer, len))
        {
			printf("\nНеверная контрольная сумма\n");
        	break;
		}
        values->acceleration[0] = ((buffer[3] << 8) | buffer[2]) / 32768. * 16 * G;
        values->acceleration[1] = ((buffer[5] << 8) | buffer[4]) / 32768. * 16 * G;
        values->acceleration[2] = ((buffer[7] << 8) | buffer[6]) / 32768. * 16 * G;
        values->temperature = ((buffer[9] << 8 ) | buffer[8]) / 100.;
        printf("Текущее ускорение объекта:\n  по оси X: %lf\n  по оси Y: %lf\n  по оси Z: %lf\n  полученная температура: %lf\n", 
            values->acceleration[0], values->acceleration[1], values->acceleration[2], values->temperature);
		break;
	case ANGULAR_VELONCY:
        //проверка контрольной суммы
        if(buffer[len-1] != crc_generate(buffer, len))
        {
			printf("\nНеверная контрольная сумма\n");
        	break;
		}
        values->angularVelocity[0] = ((buffer[3] << 8) | buffer[2]) / 32768. * 2000;
        values->angularVelocity[1] = ((buffer[5] << 8) | buffer[4]) / 32768. * 2000;
        values->angularVelocity[2] = ((buffer[7] << 8) | buffer[6]) / 32768. * 2000;
        values->temperature = ((buffer[9] << 8 ) | buffer[8]) / 100.;
        printf("Текущая угловая скорость объекта:\n  по оси X: %lf\n  по оси Y: %lf\n  по оси Z: %lf\n  полученная температура: %lf\n", 
            values->angularVelocity[0], values->angularVelocity[1], values->angularVelocity[2], values->temperature);
		break;
	case ANGLE:
		//проверка контрольной суммы
        if(buffer[len-1] != crc_generate(buffer, len))
        {
			printf("\nНеверная контрольная сумма\n");
        	break;
		}  
        values->angle[0] = ((buffer[3] << 8) | buffer[2]) / 32768. * 180;
        values->angle[1] = ((buffer[5] << 8) | buffer[4]) / 32768. * 180;
        values->angle[2] = ((buffer[7] << 8) | buffer[6]) / 32768. * 180;
        values->version = ((buffer[9] << 8 ) | buffer[8]);
        printf("Текущий угол поворота объекта:\n  по оси X: %lf\n  по оси Y: %lf\n  по оси Z: %lf\n  полученная версия(?): %i\n", 
            values->angle[0], values->angle[1], values->angle[2], values->version);
		break;
	case MAGNETIC:
        //проверка контрольной суммы
        if(buffer[len-1] != crc_generate(buffer, len))
        {
			printf("\nНеверная контрольная сумма\n");
        	break;
		}
        values->magneta[0] = ((buffer[3] << 8) | buffer[2]);
        values->magneta[1] = ((buffer[5] << 8) | buffer[4]);
        values->magneta[2] = ((buffer[7] << 8) | buffer[6]);
        //values->temperature = ((buffer[9] << 8 ) | buffer[8]) / 100.; // поему-то передается температура всегда 0
        printf("Текущая знчение магнитного поля (индукции):\n  по оси X: %i\n  по оси Y: %i\n  по оси Z: %i\n  полученная температура: %lf\n", 
            values->magneta[0], values->magneta[1], values->magneta[2], values->temperature);
		break;
	case QUATERION:
        //проверка контрольной суммы
        if(buffer[len-1] != crc_generate(buffer, len))
        {
			printf("\nНеверная контрольная сумма\n");
        	break;
		}
        values->quaterion[0] = ((buffer[3] << 8) | buffer[2]) / 32768.;
        values->quaterion[1] = ((buffer[5] << 8) | buffer[4]) / 32768.;
        values->quaterion[2] = ((buffer[7] << 8) | buffer[6]) / 32768.;
        values->quaterion[3] = ((buffer[7] << 8) | buffer[6]) / 32768.;
        printf("Текущию кватерионы(?):\n  Кватерион 0: %lf\n  Кватерион 1: %lf\n  Кватерион 2: %lf\n  Кватерион (3): %lf\n", 
            values->quaterion[0], values->quaterion[1], values->quaterion[2], values->quaterion[3]);
		break;
	default:
		printf("Получена неизвестная комманда - ");
		PRINTHEX8ARRAY(buffer, len);
		printf("\n");
	}
}


/// @brief функция для запуска опроса устрйоства hwt905 в отдельном потоке
/// @param arg указатель на список аругментов uart
/// @return 
void* uart_pthread_function(void *arg) {

	uart_args *uart_args_values = (uart_args*) arg;
	uint8_t *uart_buffer_read = NULL, *uart_buffer_write = NULL;

	if ((uart_buffer_read = (uint8_t*) malloc(uart_args_values->uart_buffer_read_len)) < 0) {
		
		printf("Error %i from malloc: %s\n", errno, strerror(errno));
		pthread_exit(NULL);
	}
	
	if ((uart_buffer_write = (uint8_t*) malloc(uart_args_values->uart_buffer_write_len)) < 0) {
		
		printf("Error %i from malloc: %s\n", errno, strerror(errno));
        free(uart_buffer_read);
		pthread_exit(NULL);
	}
	
	while (true) {
		
		function_run(msg_read_time, *uart_args_values->serial_port,
					 uart_buffer_write, uart_args_values->uart_buffer_write_len,
					 uart_buffer_read, uart_args_values->uart_buffer_read_len,
					 uart_args_values->max_uart_delay, uart_args_values->values);
					 
		function_run(msg_read_acceleration, *uart_args_values->serial_port,
					 uart_buffer_write, uart_args_values->uart_buffer_write_len,
					 uart_buffer_read, uart_args_values->uart_buffer_read_len,
					 uart_args_values->max_uart_delay, uart_args_values->values);
					 
		function_run(msg_read_angular_velocity, *uart_args_values->serial_port,
					 uart_buffer_write, uart_args_values->uart_buffer_write_len,
					 uart_buffer_read, uart_args_values->uart_buffer_read_len,
					 uart_args_values->max_uart_delay, uart_args_values->values);
		function_run(msg_read_angle, *uart_args_values->serial_port,
					 uart_buffer_write, uart_args_values->uart_buffer_write_len,
					 uart_buffer_read, uart_args_values->uart_buffer_read_len,
					 uart_args_values->max_uart_delay, uart_args_values->values);
		function_run(msg_read_magnetic, *uart_args_values->serial_port,
					 uart_buffer_write, uart_args_values->uart_buffer_write_len,
					 uart_buffer_read, uart_args_values->uart_buffer_read_len,
					 uart_args_values->max_uart_delay, uart_args_values->values);
		
		while (!TAILQ_EMPTY(uart_args_values->headp)) {
			
			function_run(NULL, *uart_args_values->serial_port,
						 TAILQ_FIRST(uart_args_values->headp)->command, TAILQ_FIRST(uart_args_values->headp)->bytes,
						 uart_buffer_read, uart_args_values->uart_buffer_read_len,
						 uart_args_values->max_uart_delay, uart_args_values->values);
			delete_command_elem(uart_args_values->headp);
		}
		
		usleep(100*1000);
	}
	pthread_exit(NULL);
}



/// @brief фукнция обертка для удобного вызова различных функций в процессе работы программы
/// @param func указатель на функцию запроса данных
/// @param serial_port порт
/// @param uart_buffer_write буфер, использующися для записи
/// @param uart_buffer_write_len длина буфера записи
/// @param uart_buffer_read буфер использующийся для чтения
/// @param uart_buffer_read_len длина буфера чтения
/// @param max_uart_delay максимальное ожидание ответа от датчика
/// @param values значения которые получила программа
void function_run(size_t (*func)(uint8_t *const, const size_t), int serial_port, uint8_t *uart_buffer_write,
                            size_t uart_buffer_write_len, uint8_t *uart_buffer_read, size_t uart_buffer_read_len, 
                            uint32_t max_uart_delay, hwt905_values *values) 
{
	
	size_t bytes_write, bytes_read = 0;
	uint32_t time_delay;
	struct timeval current_time;
	
	if (func != NULL)
		bytes_write = func(uart_buffer_write, uart_buffer_write_len);
	else
		bytes_write = uart_buffer_write_len;
	
	gettimeofday(&current_time, NULL);
	bytes_read = read_all(serial_port, uart_buffer_read, uart_buffer_read_len, 
						  &current_time, max_uart_delay);
	if (bytes_read > 0) {
		parse_hwt905_answer(uart_buffer_read, bytes_read, values);
	}
}

void cleanup(int signaln)
{
	printf("Process hwt905 ending\n");
	close(serial_port);
	free(uart_args_values.values);
	free(readRingBuffer.buffer);

	exit(0); // Завершаем программу
}

bool find_msg_beginning(ringBuffer* ringBuffer)
{
	for(size_t i =0; i < ringBuffer->bytes_avail; i++)
	{
		if(ringBuffer->buffer[ringBuffer->head] != 0x55)
		{
			
			ringBuffer->head = (ringBuffer->head + 1) % ringBuffer->buffer_size;
		}
		else
		{
			ringBuffer->bytes_avail -= i;
			return true;
		}
	}
	ringBuffer->bytes_avail = 0;
	return false;	
}


// Print system error and exit
void error(char *msg)
{
    perror(msg);
    exit(1);
}



int main(int argc, char *argv[])
{

	 //привязываем к сигналам уничтожения и закрытия программы очистку памяти 
    signal(SIGTERM, cleanup);
    signal(SIGINT, cleanup);
	
    
    struct headname headp;
	
    char *path = "/dev/ttyUSB0"; //TODO исправить путь до порта 
    pthread_t uart_pthread;
	ssize_t read_bytes;

	
	readRingBuffer.buffer_size = 256;
	readRingBuffer.bytes_avail = 0;
	readRingBuffer.head = 0;
	readRingBuffer.tail = 0;
	readRingBuffer.buffer = (uint8_t*)malloc(readRingBuffer.buffer_size);

    TAILQ_INIT(&headp);
	
	uart_args_values.serial_port = &serial_port;
	uart_args_values.uart_buffer_read_len = 11;
	uart_args_values.uart_buffer_write_len = 30;
	uart_args_values.max_uart_delay = 500;
	uart_args_values.headp = &headp;
	uart_args_values.values = (hwt905_values*) malloc(sizeof(hwt905_values));
	
	
	// создание shared memory
	struct shared_memory *shared_mem_ptr;
    sem_t *mutex_sem;
    int fd_shm;
    char mybuf [1024];





   if (open_serial_port(path, &serial_port) < 0)
   {
		printf("Ошибка открытия порта");
		exit(EXIT_FAILURE);
   }
    printf("Access open serial port\n");


	uint8_t buffer[130];


	uint8_t write_buffer[5];
	usleep(100*1000);
	


	uint8_t unlock_cmd[] = {REQUEST_PREFIX, SECOND_REGISTER, 0x69, 0x88, 0xb5};
	uint8_t bandRate_cmd[] = {REQUEST_PREFIX, SECOND_REGISTER, RATE, 0x04, 0x00}; // 0x01 = 0.2Hz 0x02 = 0.5Hz 0x03 = 1Hz 0x04 = 2Hz 0x05 = 5Hz 0x06 = 10Hz

	uint8_t readAll_cmd[] = {REQUEST_PREFIX, SECOND_REGISTER, RSW, TIME_REQ | ACCELERATION_REQ | ANGULAR_VELONCY_REQ | ANGLE_REQ | MAGNETIC_REQ, 0x02};
	uint8_t save_cmd[] = {REQUEST_PREFIX, SECOND_REGISTER, SAVE, 0x00, 0x00};


	//разблокировка
	if(write(serial_port, unlock_cmd, sizeof(unlock_cmd)) != sizeof(unlock_cmd))
		printf("unlock message error\n");
	usleep(100000);

    // отправка команды для задания 
    if(write(serial_port, bandRate_cmd, sizeof(bandRate_cmd)) != sizeof(bandRate_cmd))
		printf("request message error\n");
	usleep(100000);

    // save
   
    if( write(serial_port, save_cmd, sizeof(save_cmd)) != sizeof(save_cmd))
		printf("save message error\n");
	usleep(100000);



	// unlock port 
    if(write(serial_port, unlock_cmd, sizeof(unlock_cmd)) != sizeof(unlock_cmd))
		printf("unlock message error\n");
	usleep(100000);

    // отправка команды на считываение всех данных
    
    if(write(serial_port, readAll_cmd, sizeof(readAll_cmd)) != sizeof(readAll_cmd))
		printf("request message error\n");
	usleep(100000);

    // save
   
    if( write(serial_port, save_cmd, sizeof(save_cmd)) != sizeof(save_cmd))
		printf("save message error\n");
	usleep(100000);




	

	uint8_t parse_buffer[11];

	read_bytes = read(serial_port, buffer, 15);
	PRINTHEX8ARRAY(buffer, read_bytes);
    

	int count = 0;

	//for(int i = 0; i < 1000; i++)
	while (true) // TODO изменить бесконечный цикл???
	{
		// TODO тут будет проверка подключения клиента




		read_bytes = read(serial_port, buffer, 50);
		if(read_bytes != -1)
		{	
			printf("считанные данные:");
			PRINTHEX8ARRAY(buffer, read_bytes);
		}
		put(&readRingBuffer, buffer, read_bytes);
		printf("\nДанные, считанные в ринг буффер: ");
		find_msg_beginning(&readRingBuffer);
		print_ring_buffer_hex(&readRingBuffer);
		get(&readRingBuffer, parse_buffer, 11);
		printf("данные для парсинга:");
		PRINTHEX8ARRAY(parse_buffer, 11);
		printf("\n");
		parse_hwt905_answer(parse_buffer, 11, uart_args_values.values);
		usleep(100*1000);

		count += 1;
		if(count % 10 == 0)
		{
							
			char *cp;

			// Critical section
			time_t now = time(NULL);
			cp = ctime(&now);
			int len = strlen(cp);
			if(*(cp + len -1) == '\n')
				*(cp + len -1) = '\0';
			// sprintf(shared_mem_ptr->buf, "%d: HWT905 message number = %i | date_time =  %s acceleration (%lf; %lf; %lf), MF (%i; %i; %i), Angular velocity (%lf;%lf;%lf), Temp = %lf\n", 
			// 	getpid(),count / 10, cp, uart_args_values.values->acceleration[0], uart_args_values.values->acceleration[1], uart_args_values.values->acceleration[2],
			// 	uart_args_values.values->magneta[0],uart_args_values.values->magneta[1], uart_args_values.values->magneta[2],
			// 	uart_args_values.values->angularVelocity[0],  uart_args_values.values->angularVelocity[1],  uart_args_values.values->angularVelocity[2],
			// 	uart_args_values.values->temperature);

            // отправка сообщения по TCP 
            // TODO реализовать отправку по TCP


			printf("%d: Данные отправленные в shared memory HWT905 message number = %i | date_time =  %s acceleration (%lf; %lf; %lf), MF (%i; %i; %i), Angular velocity (%lf;%lf;%lf), Temp = %lf\n", 
				getpid(),count, cp, uart_args_values.values->acceleration[0], uart_args_values.values->acceleration[1], uart_args_values.values->acceleration[2],
				uart_args_values.values->magneta[0],uart_args_values.values->magneta[1], uart_args_values.values->magneta[2],
				uart_args_values.values->angularVelocity[0],  uart_args_values.values->angularVelocity[1],  uart_args_values.values->angularVelocity[2],
				uart_args_values.values->temperature);

			// // Release mutex sem: V(mutex_sem)
			// if(sem_post(mutex_sem) == -1)
			// 	error("sem_post: mutex_sem");
		}
	}
   	
    // if (pthread_create(&uart_pthread, NULL, uart_pthread_function, (void*) &uart_args_values) < 0) {

	// 	printf("Error %i from pthread_create: %s\n", errno, strerror(errno));
	// 	exit(EXIT_FAILURE);
	// }
	
   	// pthread_join(uart_pthread, NULL);

	printf("Последние актуальные данные по каждой позиции, полученные за время работы программы:\n ");
	printf("Год: %i \n  Месяц: %i\n  День: %i\n  Время: %i:%i:%i:%i\n", uart_args_values.values->YY, uart_args_values.values->MM, uart_args_values.values->DD,
	 	uart_args_values.values->hh, uart_args_values.values->mm, uart_args_values.values->ss, uart_args_values.values->ms);
	printf("Ускороение: (%lf, %lf, %lf)\n  ", uart_args_values.values->acceleration[0], uart_args_values.values->acceleration[1], uart_args_values.values->acceleration[2]);
	printf("Угловая скорость (%lf, %lf, %lf)\n",uart_args_values.values->angularVelocity[0], uart_args_values.values->angularVelocity[1], uart_args_values.values->angularVelocity[2]);
	printf("Температура: %lf\n",  uart_args_values.values->temperature); 
	printf("Угол: (%lf, %lf, %lf)\n", uart_args_values.values->angle[0], uart_args_values.values->angle[1], uart_args_values.values->angle[2]);
	printf("Магнитное поле: (%i, %i, %i)\n", uart_args_values.values->magneta[0], 
		 uart_args_values.values->magneta[1], uart_args_values.values->magneta[2]);
	printf("Кватерионы: (%lf, %lf, %lf, %lf)", uart_args_values.values->quaterion[0], uart_args_values.values->quaterion[1],
		 uart_args_values.values->quaterion[2], uart_args_values.values->quaterion[3]);

	free(uart_args_values.values);
	free(readRingBuffer.buffer);
	close(serial_port);
    return 0;
}