# KOS - K operation system
Простая микроядерная операционная система для микроконтроллеров stm32f103, однако может быть переделана под другие ARM Cortex-M процессоры (необходимо переписать функции работы с SD картой и прерываниями)

Представляет собой мультизадачную операционную систему, способную хранить и выполнять по 1 процессу (для переключения процессов сохраняет их в ПЗУ)

Репозиторий содержит набор функций и структур для работы с SD-картой, загрузкой, исполнением и сохранением процессов.

Для работы используются библиотеки HAL STM32 и FatFS elm-chan'а

Код библиотек не представлен в репозитории(кроме файла diskio.c), для сборки системы необходимо самостоятельно установить их

# Использование
1. Создать структуру типа SDCardPort
2. Вызвать функцию initializeSDCard
3. Вызвать f_mount библиотеки FatFS
4. Вызвать функцию initializeScheduler
5. Добавить процессы с помощью функции createProcess
6. вызвать runProcesses (исполняет все процессы по 1 разу)

Простой пример:

    sdport0 = (struct SDCardPort){ // struct SDCardPort sdport0
		.state = NOTINITIALIZED,
		.GPIOx = SPI1_CS_GPIO_Port,
		.hspi = &hspi1,
		.pin = SPI1_CS_Pin
	};
	while(initializeSDCard(&sdport0) != 0){HAL_Delay(1);}
	FATFS SDFatFs;
	FRESULT res = f_mount(&SDFatFs, _T("") , 0);
	if(initializeScheduler() == SCHEDULER_OK){
		struct ProcessSettings settings = {.tacts=1};
		createProcess(_T("my_proc"),&settings);
		runProcesses();
	}
	f_mount(NULL, _T(""), 0);

# Компиляция программ под систему
Все процессы, загружаемые системой, должны иметь точку входа >= 0x20002000 и иметь размер не более 12 КиБ (в т.ч. стек)