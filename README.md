# Описание программы
Данная программа создана для подключения к устройству HWT905, которое является компасом. 

Для работы с данным компасом, необходимо выполнить ряд действий. 

Подключение устройства происходит по проводу, через USB порт. в программе установлено открытие порта /dev/ttyUSB0,
 возмоно имя порта для соединения может меняться. Программа запускает сервер и через него другая программа (asc), после подклюения
 получает данные о данных, полученных с устройства. 

Проверка работы сервера выплняется при помощи выполнения команды telnet ```localhost 8080```. 

После подлкючения к серверу необходимо ввести команду для чтения данных ```GET_DATA``` 

Полученные данные будут переданы клиенту, в строковом формате. На данным момент без определенного формата сообщения (протокла общения) 
