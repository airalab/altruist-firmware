/*
 *	airRohr firmware
 *	Copyright (C) 2016-2018  Code for Stuttgart a.o.
 *
 *  Russian translations
 *
 *	Texts should be as short as possible
 */

#define INTL_LANG "RU"
#define INTL_PM_SENSOR "Датчик твердых мелкодисперсных частиц"
const char INTL_CONFIGURATION[] PROGMEM = "Конфигурация";
#define INTL_COMMON_SETTINGS "Стандартные настройки"
#define INTL_WIFI_NETWORKS "Загрузить сети Wi-Fi ..."
#define INTL_APIS_SETTINGS "Дополнительные APIs"
#define INTL_LANGUAGE "Язык"
#define INTL_NO_WLAN_PWD "Сеть WiFi без пароля"
const char INTL_NO_NETWORKS[] PROGMEM =  "Cетей не найдено";
const char INTL_NETWORKS_FOUND[] PROGMEM = "Найденые сети: ";
const char INTL_AB_HIER_NUR_ANDERN[] PROGMEM = "Изменяйте значения ниже если действительно знаете что делаете";
const char INTL_SAVE[] PROGMEM = "Сохранить";
const char INTL_SENSORS[] PROGMEM = "Датчики";
const char INTL_MORE_SENSORS[] PROGMEM = "Больше датчиков";
const char INTL_SDS011[] PROGMEM = "SDS011 ({pm})";
const char INTL_GC[] PROGMEM = "Счетчик Гейгера ({cpm})";
const char INTL_DBMETER[] PROGMEM = "Датчик уровня шума, (интервал отправки должен быть > 30 сек)";
const char INTL_I2SNOISE[] PROGMEM = "I2S Датчик уровня шума, (интервал отправки должен быть > 30 сек)";
const char INTL_PMS[] PROGMEM = "Plantower PMS(1,3,5,6,7)003 ({pm})";
const char INTL_HPM[] PROGMEM = "Honeywell PM ({pm})";
const char INTL_NPM[] PROGMEM = "Tera Sensor Next PM ({pm})";
const char INTL_SPS30[] PROGMEM = "Sensirion SPS30 ({pm})";
const char INTL_PPD42NS[] PROGMEM = "PPD42NS ({pm})";
const char INTL_DHT22[] PROGMEM = "DHT22 ({t}, {h})";
const char INTL_HTU21D[] PROGMEM = "HTU21D ({t}, {h})";
const char INTL_BMP180[] PROGMEM = "BMP180 ({t}, {p})";
const char INTL_BMX280[] PROGMEM = "BME280 ({t}, {h}, {p}), BMP280 ({t}, {p})";
const char INTL_SHT3X[] PROGMEM = "SHT3X ({t}, {h})";
const char INTL_DS18B20[] PROGMEM = "DS18B20 ({t})";
const char INTL_CCS811_27[] PROGMEM = "CCS811 (I2C: 0x5A)";
const char INTL_CCS811_3F[] PROGMEM = "CCS811 (I2C: 0x5B)";
const char INTL_DNMS[] PROGMEM = "DNMS ({l_a})";
const char INTL_DNMS_CORRECTION[] PROGMEM = "поправка в dB(A)";
const char INTL_TEMP_CORRECTION[] PROGMEM = "Коррекция температуры в °C";
const char INTL_CUSTOM_ALTRUIST[] PROGMEM = "Пользовательский адрес Altruist Urban";
const char INTL_USE_CUSTOM_URBAN[] PROGMEM = "Использовать пользовательский адрес Altruist Urban";
const char INTL_INSIGHT_STANDALONE[] PROGMEM = "Insight standalone (без связи с Urban)";

// Выбор Urban (настройка и страница конфигурации)
#define INTL_SCANNING_URBANS "Поиск устройств Altruist Urban..."
#define INTL_SELECT_URBAN_TITLE "Выберите устройство Altruist Urban"
#define INTL_SELECT_URBAN_DESC "Выберите устройство Urban, с которого этот Insight будет получать данные уличных датчиков."
#define INTL_NO_URBANS_FOUND "Устройства Altruist Urban не найдены в этой сети. Убедитесь, что Urban включён и подключён к той же сети WiFi. Укажите IP вручную ниже и сохраните; Urban можно настроить позже в Настройках."
#define INTL_USE_CUSTOM_IP "Указать IP-адрес вручную:"
#define INTL_SETUP_INSIGHT_MODE_HINT "Есть Altruist Urban (уличный блок)? Отметьте галочку ниже, чтобы найти его в сети и связать. Без галочки — автономный режим только Insight."
#define INTL_SETUP_PAIR_WITH_URBAN "Настроить связь с Altruist Urban сейчас"
#define INTL_SETUP_CONTINUE "Дальше"
#define INTL_SKIP_URBAN_SELECTION "Пропустить &mdash; настроить позже в Настройках"
#define INTL_SETUP_COMPLETE "Настройка завершена"
#define INTL_SETTINGS_SAVED "Настройки сохранены"
#define INTL_DEVICE_RESTARTING "Перезапуск устройства..."
#define INTL_GUEST_CONNECTED "Подключено"
#define INTL_GUEST_WIFI_STEP_TITLE "Wi-Fi подключён"
#define INTL_GUEST_SETUP_STEP_1_LABEL "Шаг 1 из 2"
#define INTL_GUEST_SETUP_STEP_2_LABEL "Шаг 2 из 2"
#define INTL_GUEST_SETUP_STEP_1_TITLE "Подключение к Wi-Fi"
#define INTL_GUEST_INSIGHT_FINISH_HINT "Нажмите «Дальше», чтобы завершить настройку и перезагрузить устройство."
#define INTL_GUEST_INSIGHT_AUTO_FINISH_HINT "Если закрыть эту страницу, настройка завершится автоматически через"
#define INTL_GUEST_INSIGHT_AUTO_FINISH_SUFFIX "с (автономный режим)."
#define INTL_GUEST_IP_ADDRESS "IP адрес:"
#define INTL_GUEST_OPEN_IP_HINT "Скопируйте IP-адрес и откройте его в браузере."
#define INTL_GUEST_RESTART_PAUSE_HINT "Устройство перезапустится через несколько секунд — скопируйте IP сейчас."
#define INTL_GUEST_KEEP_OPEN_HINT "Не закрывайте эту страницу, пока не нажмёте «Дальше»."
#define INTL_DISP_MAP_PROMO_TITLE "Больше аналитики прямо в смартфоне"
#define INTL_DISP_MAP_PROMO_LINE1 "Заходите на нашу веб-карту: AQI, история данных,"
#define INTL_DISP_MAP_PROMO_LINE2 "цветные графики, удобный шэринг и"
#define INTL_DISP_MAP_PROMO_LINE3 "много других фич уже скоро"
#define INTL_DISP_MAP_DOMAIN "SENSORS.SOCIAL"
#define INTL_SCAN_BTN "Поиск"
#define INTL_SCAN_SCANNING "Поиск..."
#define INTL_SCAN_NO_URBANS "Устройства Urban не найдены."
#define INTL_SCAN_FOUND_PREFIX "Найдено "
#define INTL_SCAN_FOUND_SUFFIX " устройств(о) Urban."
#define INTL_SCAN_FAILED "Ошибка поиска: "

const char INTL_NEO6M[] PROGMEM = "GPS (NEO 6M)";
const char INTL_RWS_OWNER[] PROGMEM = "Адрес владельца подписки";
const char INTL_GROUP_MENU[] PROGMEM = "Группа устройств (RWS)";
const char INTL_GROUP_INTRO[] PROGMEM = "Выберите режим участия в Robonomics Web Services (owner и on-chain список устройств).";
const char INTL_GROUP_MODE_TITLE[] PROGMEM = "Режим работы";
const char INTL_GROUP_MODE_STANDALONE[] PROGMEM = "Standalone — устройство само себе master (setDevices только с собой)";
const char INTL_GROUP_MODE_MASTER[] PROGMEM = "Создать группу — это устройство master группы";
const char INTL_GROUP_MODE_FOLLOWER[] PROGMEM = "Войти в группу — указать адрес master ниже";
const char INTL_GROUP_MODE_MANUAL[] PROGMEM = "Manual owner — legacy: только owner, без автоматического setDevices";
const char INTL_GROUP_SELF_ADDRESS[] PROGMEM = "Robonomics-адрес этого устройства (скопируйте на master при входе в группу)";
const char INTL_GROUP_MASTER_PANEL[] PROGMEM = "Master группы";
const char INTL_GROUP_FOLLOWER_PANEL[] PROGMEM = "Вход в группу";
const char INTL_GROUP_MANUAL_PANEL[] PROGMEM = "Manual owner";
const char INTL_GROUP_ID_LABEL[] PROGMEM = "ID группы";
const char INTL_GROUP_MASTER_ADDRESS[] PROGMEM = "Robonomics-адрес master";
const char INTL_GROUP_MASTER_INCLUDED[] PROGMEM = "Master (добавляется в setDevices автоматически)";
const char INTL_GROUP_KNOWN_DEVICES[] PROGMEM = "Дополнительные устройства — followers (SS58, по одному в строке)";
const char INTL_GROUP_KNOWN_DEVICES_HINT[] PROGMEM = "Адрес этого устройства выше всегда включён как master. Добавьте SS58 follower-ов сюда и нажмите Save.";
const char INTL_GROUP_FOLLOWER_HINT[] PROGMEM = "Скопируйте свой адрес выше в список на master, введите адрес master здесь и нажмите Save.";
const char INTL_GROUP_MANUAL_HINT[] PROGMEM = "Datalog использует этот owner. setDevices не вызывается автоматически.";
const char INTL_GROUP_STATUS_GROUP_CREATING[] PROGMEM = "Группа создана — синхронизация on-chain";
const char INTL_GROUP_STATUS_LIST_UPDATED[] PROGMEM = "Список устройств обновлён — синхронизация on-chain";
const char INTL_GROUP_STATUS_LIST_SYNCED[] PROGMEM = "Список устройств синхронизирован on-chain";
const char INTL_GROUP_STATUS_LABEL[] PROGMEM = "Статус";
const char INTL_GROUP_STATUS_CREATED[] PROGMEM = "Группа создана, устройства синхронизированы";
const char INTL_GROUP_CURRENT_DEVICES[] PROGMEM = "Текущий список устройств";
const char INTL_GROUP_STATUS_PENDING[] PROGMEM = "Ожидание синхронизации";
const char INTL_GROUP_STATUS_DEVICES_SYNCED[] PROGMEM = "Устройства синхронизированы on-chain";
const char INTL_GROUP_STATUS_JOINED[] PROGMEM = "Группа подключена";
const char INTL_GROUP_STATUS_MANUAL[] PROGMEM = "Manual owner настроен";
const char INTL_GROUP_SAVE_OK[] PROGMEM = "Настройки группы сохранены.";
const char INTL_GROUP_SAVE_FAILED[] PROGMEM = "Не удалось сохранить настройки группы.";
const char INTL_GROUP_SAVE_CONFIG_FAILED[] PROGMEM = "Не удалось записать конфигурацию в память устройства.";
const char INTL_GROUP_ERROR_INVALID_MASTER[] PROGMEM = "Укажите корректный Robonomics-адрес master.";
const char INTL_GROUP_ERROR_INVALID_MANUAL_OWNER[] PROGMEM = "Укажите корректный Robonomics-адрес owner.";
const char INTL_SCREEN_MENU[] PROGMEM = "Режим экрана";
const char INTL_SCREEN_INTRO[] PROGMEM = "Выберите способ обновления e-paper дисплея. Разные партии панелей по-разному реагируют на partial refresh.";
const char INTL_SCREEN_MODE_SAFE[] PROGMEM = "Safe (безопасный)";
const char INTL_SCREEN_MODE_SAFE_HINT[] PROGMEM = "Только полное обновление экрана. Рекомендуется для всех устройств. Предотвращает артефакты на дисплее.";
const char INTL_SCREEN_MODE_EXPERIMENTAL[] PROGMEM = "Experimental partial refresh";
const char INTL_SCREEN_MODE_EXPERIMENTAL_HINT[] PROGMEM = "Быстрые частичные обновления. Меньше мерцания, но возможны ghosting и битая картинка на некоторых панелях.";
const char INTL_SCREEN_SAVE_OK[] PROGMEM = "Режим экрана сохранён.";
const char INTL_SCREEN_SAVE_FAILED[] PROGMEM = "Не удалось сохранить режим экрана.";
const char INTL_SCREEN_SAVE_INVALID_MODE[] PROGMEM = "Выбран недопустимый режим экрана.";
const char INTL_SCREEN_SAVE_CONFIG_FAILED[] PROGMEM = "Не удалось записать конфигурацию в память устройства.";
const char INTL_ROBONOMICS_PUBLIC_NODE[] PROGMEM = "Адрес публичной ноды Робономики";
const char INTL_ROBONOMICS_CONNECTIVITY_HOST[] PROGMEM = "Хост Robonomics Map (connectivity)";
const char INTL_ROBONOMICS_CONNECTIVITY_HOSTS[] PROGMEM = "Пул хостов Robonomics Map (по одному в строке)";
const char INTL_COORD_LAT[] PROGMEM = "Широта";
const char INTL_COORD_LON[] PROGMEM = "Долгота";
const char INTL_COORDS[] PROGMEM = "GPS: Широта, Долгота";
const char INTL_BASICAUTH[] PROGMEM = "Активировать аутентификацию для входа в интерфейс сенсора";
#define INTL_REPORT_ISSUE "Сообщить о проблеме"

#define INTL_PANEL_TITLE_WIFI "Настройка WiFi"
#define INTL_PANEL_TITLE_ROBONOMICS "Robonomics"
#define INTL_PANEL_TITLE_GPS "GPS & Датчики"
#define INTL_PANEL_TITLE_AUTH "Аутентификация"
#define INTL_PANEL_TITLE_DEBUG "Уровень отладки"
#define INTL_PANEL_TITLE_FIRMWARE "Версия Прошивки"
#define INTL_PANEL_TITLE_WIFI_CONFIG "WiFi в режиме настройки"
#define INTL_PANEL_TITLE_CSV "CSV"
#define INTL_PANEL_TITLE_CUSTOMAPI "Пользовательский API"
#define INTL_PANEL_TITLE_INFLUX "Ifnlux DB"
#define INTL_PANEL_TITLE_DATA_SHARING "Публикация на карту"
#define INTL_DATA_SHARING_DISCLAIMER "По умолчанию все данные датчиков отправляются на публичную карту. Вы можете выбрать, какие данные передавать. Остальные данные будут отображаться на экране устройства и доступны локально."
#define INTL_DATA_SHARING_ADDITIONAL "Доп. датчики (опционально)"
const char INTL_SHARE_TEMPERATURE[] PROGMEM = "Температура";
const char INTL_SHARE_HUMIDITY[] PROGMEM = "Влажность";
const char INTL_SHARE_PRESSURE[] PROGMEM = "Давление";
const char INTL_SHARE_CO2[] PROGMEM = "CO2";
const char INTL_SHARE_PM[] PROGMEM = "Пыль (PM2.5/PM10)";
const char INTL_SHARE_NOISE[] PROGMEM = "Уровень шума";
const char INTL_SHARE_CO[] PROGMEM = "Угарный газ (CO)";
const char INTL_SHARE_RADIATION[] PROGMEM = "Радиация";
const char INTL_SHARE_O3[] PROGMEM = "Озон (O3)";
const char INTL_SHARE_NO2[] PROGMEM = "Диоксид азота (NO2)";
const char INTL_SHARE_FAST_AQI[] PROGMEM = "FAST AQI";
const char INTL_SHARE_EPA_AQI[] PROGMEM = "EPA AQI";

const char INTL_FS_WIFI_DESCRIPTION[] PROGMEM = "Название WiFi устройства в режиме конфигурации";
const char INTL_FS_WIFI_NAME[] PROGMEM = "Имя";
const char INTL_MORE_SETTINGS[] PROGMEM = "Продвинутые настройки";
const char INTL_AUTO_UPDATE[] PROGMEM = "Автоматическое обновление";
const char INTL_USE_BETA[] PROGMEM = "Загружать бета-версии";
const char INTL_DISPLAY[] PROGMEM = "OLED SSD1306";
const char INTL_SH1106[] PROGMEM = "OLED SH1106";
const char INTL_FLIP_DISPLAY[] PROGMEM = "Перевернуть OLED экран";
const char INTL_LCD1602_27[] PROGMEM = "LCD 1602 (I2C: 0x27)";
const char INTL_LCD1602_3F[] PROGMEM = "LCD 1602 (I2C: 0x3F)";
const char INTL_LCD2004_27[] PROGMEM = "LCD 2004 (I2C: 0x27)";
const char INTL_LCD2004_3F[] PROGMEM = "LCD 2004 (I2C: 0x3F)";
const char INTL_DISPLAY_WIFI_INFO[] PROGMEM = "Отображать информацию о WiFi";
const char INTL_DISPLAY_DEVICE_INFO[] PROGMEM = "Отображать информацию об устройстве";
const char INTL_DEBUG_LEVEL[] PROGMEM = "Уровень&nbsp;отладки";
const char INTL_MEASUREMENT_INTERVAL[] PROGMEM = "Интервал отправки данных (с)";
const char INTL_LEDS_BRIGHTNESS[] PROGMEM = "Яркость светодиодов (%)";
const char INTL_LEDS_ON[] PROGMEM = "Включить светодиоды";
const char INTL_LEDS_OFF_HOUR[] PROGMEM = "Час выключения LED (0-23)";
const char INTL_LEDS_ON_HOUR[] PROGMEM = "Час включения LED (0-23)";
const char INTL_SDS_MEAS_INTERVAL[] PROGMEM = "Интервал измерения SDS (с)";
const char INTL_DATALOG_SENDING_INTERVAL[] PROGMEM = "Интервал отправки даталогов";
const char INTL_DURATION_ROUTER_MODE[] PROGMEM = "Длительность режима маршрутизатора";
const char INTL_MORE_APIS[] PROGMEM = "Другие API";
const char INTL_SEND_TO_OWN_API[] PROGMEM = "Отправить в свой API";
const char INTL_SERVER[] PROGMEM = "Сервер";
const char INTL_PATH[] PROGMEM = "Путь";
const char INTL_PORT[] PROGMEM = "Порт";
const char INTL_USER[] PROGMEM = "Пользователь";
const char INTL_PASSWORD[] PROGMEM = "Пароль";
const char INTL_LOCAL_HOSTNAME[] PROGMEM = "Локальный домен (Измените его, если у вас больше одного альтруиста в одной сети)";
const char INTL_MEASUREMENT[] PROGMEM = "Measurement";
const char INTL_SEND_TO[] PROGMEM = "Отправлять в {v}";
const char INTL_READ_FROM[] PROGMEM = "Считывать с {v}";
const char INTL_SENSOR_IS_REBOOTING[] PROGMEM = "Устройство перезапускается...";
const char INTL_RESTART_DEVICE[] PROGMEM = "Перезапустить устройство";
const char INTL_DELETE_CONFIG[] PROGMEM = "Config.json удалить";
const char INTL_RESTART_SENSOR[] PROGMEM = "Перезапустить устройство";
#define INTL_HOME "Меню"
#define INTL_BACK_TO_HOME "Вернуться в основное меню"
const char INTL_CURRENT_DATA[] PROGMEM = "Текущие значения";
// Graphs screen
#define INTL_DISP_GRAPHS_HEADER_PREFIX ""
#define INTL_DISP_GRAPHS_HINT_LINE1 "долгое нажатие ->"
#define INTL_DISP_GRAPHS_HINT_LINE2 "след./пред."
#define INTL_DISP_GRAPHS_HINT_LINE3 "экран"
const char INTL_DEVICE_STATUS[] PROGMEM = "Состояние устройства";
#define INTL_ACTIVE_SENSORS_MAP "Карта активных датчиков (внешняя ссылка)"
#define INTL_CONFIGURATION_DELETE "Удалить конфигурацию"
#define INTL_CONFIGURATION_REALLY_DELETE "Подтвердите удаление конфигурации"
#define INTL_DELETE "Удалить"
#define INTL_CANCEL "Отменить"
#define INTL_REALLY_RESTART_SENSOR "Подтвердите перезапуск устройства"
#define INTL_RESTART "Перезапустить"
const char INTL_SAVE_AND_RESTART[] PROGMEM = "Сохранить и перезапустить";
#define INTL_FIRMWARE "Прошивка:"
#define INTL_IP_ADDRESS "IP адрес"
const char INTL_SD_CONNECTED[] PROGMEM = "SD карта подключена";
const char INTL_FREE_RAM[] PROGMEM = "Свободно памяти (ОЗУ)";
const char INTL_LAST_OTA[] PROGMEM = "Последняя проверка OTA";
#define INTL_OTA_UPDATE "Обновление прошивки"
const char INTL_OTA_CHECK_UPDATE[] PROGMEM = "Проверить обновление";
const char INTL_OTA_CURRENT_VERSION[] PROGMEM = "Текущая версия";
const char INTL_OTA_CHECK_REQUESTED[] PROGMEM = "Запрос на обновление отправлен. Устройство загрузит и установит новую прошивку, если она доступна.";
const char INTL_OTA_NO_WIFI[] PROGMEM = "WiFi не подключен. Невозможно проверить обновления.";
const char INTL_OTA_SWITCH_LANG[] PROGMEM = "Сменить язык";
const char INTL_OTA_CURRENT_LANG[] PROGMEM = "Текущий язык";
const char INTL_OTA_SWITCH_LANG_NOTE[] PROGMEM = "Устройство загрузит и установит прошивку на выбранном языке";
const char INTL_OTA_LANG_SAME[] PROGMEM = "Этот язык уже используется.";
const char INTL_OTA_LANG_REQUESTED[] PROGMEM = "Запрос на смену языка отправлен. Пожалуйста подождите. Устройство загрузит и установит прошивку на выбранном языке.";
const char INTL_UPTIME[] PROGMEM = "Время работы"; 
const char INTL_RESET_REASON[] PROGMEM = "Причина перезагрузки";
const char INTL_OTA_RETURN[] PROGMEM = "OTA Ответ";
const char INTL_COUNT_SUCCESS_SENDS[] PROGMEM = "количество успешных отправок";
const char INTL_LAST_SEND_TIME[] PROGMEM = "время последней отправки";
#define INTL_CHIP_TYPE "Тип чипа"
#define INTL_ROBONOMICS_ADDR "Адрес в Робономике"
const char INTL_DEBUG_SETTING_TO[] PROGMEM = "Настройки отладки";
#define INTL_NONE "отключена"
#define INTL_ERROR "только ошибки"
#define INTL_WARNING "предупреждения"
#define INTL_MIN_INFO "минимум информации"
#define INTL_MED_INFO "среднеинформативно"
#define INTL_MAX_INFO "максимум информации"
#define INTL_CONFIG_DELETED "Config.json удалён"
#define INTL_CONFIG_CAN_NOT_BE_DELETED "Config.json нельзя удалить"
#define INTL_CONFIG_NOT_FOUND "Config.json не найден"
const char INTL_TIME_TO_FIRST_MEASUREMENT[] PROGMEM = "Еще {v} секунд до первого замера.";
const char INTL_TIME_SINCE_LAST_MEASUREMENT[] PROGMEM = " секунд после последнего замера.";
const char INTL_PARTICLES_PER_LITER[] PROGMEM = "Частицы/ литр";
const char INTL_PARTICULATE_MATTER[] PROGMEM = "Датчик пыли";
const char INTL_TEMPERATURE[] PROGMEM = "Температура";
const char INTL_NOISE[] PROGMEM = "Шум";
const char INTL_NOISE_MAX[] PROGMEM = "Максимальный шум";
const char INTL_NOISE_MEAN[] PROGMEM = "Средний шум";
const char INTL_HUMIDITY[] PROGMEM = "Относительная влажность";
const char INTL_PRESSURE[] PROGMEM = "Давление воздуха";
const char INTL_RADIATION[] PROGMEM = "Радиация";
const char INTL_CO2[] PROGMEM = "CO2";
const char INTL_LEQ_A[] PROGMEM = "LAeq";
const char INTL_LA_MIN[] PROGMEM = "LA min";
const char INTL_LA_MAX[] PROGMEM = "LA max";
const char INTL_LATITUDE[] PROGMEM = "Широта";
const char INTL_LONGITUDE[] PROGMEM = "Долгота";
const char INTL_ALTITUDE[] PROGMEM = "Высота";
const char INTL_TIME_LOCAL[] PROGMEM = "Время";
const char INTL_SIGNAL_STRENGTH[] PROGMEM = "Сигнал";
const char INTL_SIGNAL_QUALITY[] PROGMEM = "Качество";
#define INTL_NUMBER_OF_MEASUREMENTS "Количество измерений"
#define INTL_TIME_SENDING_MS "Время, потраченное на отправку"
#define INTL_SENSOR "Датчик"
#define INTL_PARAMETER "Параметр"
#define INTL_VALUE "Значение"

#define INTL_REGION "Регион"
#define INTL_REGION_GLOBAL "Весь мир"
#define INTL_REGION_EU "Европа"
#define INTL_REGION_AS "Азия"
#define INTL_REGION_AF "Африка"
#define INTL_REGION_AU "Австралия"
#define INTL_REGION_NA "Северная Америка"
#define INTL_REGION_SA "Южная Америка"

/* Insight display (e-paper) strings */
#define INTL_DISP_PRODUCT_INSIGHT "Altruist Insight"  /* keep in English */
#define INTL_DISP_WIFI_CLEARED "Данные WiFi удалены"
#define INTL_DISP_RESTARTING "Перезапуск..."
#define INTL_DISP_WIFI_SETUP "Настройка Wi-Fi"
#define INTL_DISP_CONNECT_TO "Подключиться к"
#define INTL_DISP_PASSWORD_PREFIX "Пароль: "
#define INTL_DISP_TITLE_INSIGHT "ALTRUIST INSIGHT"  /* keep in English */
#define INTL_DISP_CONNECTING_WIFI "Подключение к Wi-Fi"
#define INTL_DISP_PLEASE_WAIT "Подождите..."
#define INTL_DISP_SD_NOT_FOUND "SD карта не найдена"
#define INTL_DISP_INSERT_SD "Вставьте SD карту"
#define INTL_DISP_FAT32_FORMATTED "(формат FAT32)"
#define INTL_DISP_NO_DATA_FILES "Файлы данных не найдены"
#define INTL_DISP_DEVICE_WILL_CREATE "Устройство создаст"
#define INTL_DISP_FILES_AUTOMATICALLY "файлы автоматически"
#define INTL_DISP_AFTER_COLLECTING "после сбора данных"
#define INTL_DISP_SD_NOT_AVAILABLE "SD карта недоступна"
#define INTL_DISP_GRAPHS_REQUIRE_SD "Графикам нужна SD карта"
#define INTL_DISP_ENABLE_SD "Включите SD карту"
#define INTL_DISP_INSIGHT_HEADER "Insight"
#define INTL_DISP_INSIGHT_ONLY "Insight only"
#define INTL_DISP_URBAN_HEADER "Urban"
#define INTL_DISP_URBAN_ONLY "Urban only"
#define INTL_DISP_GOING_TO_SLEEP "Уход в сон..."
#define INTL_DISP_OTA_UPDATING "Обновление прошивки"
#define INTL_DISP_OTA_DO_NOT_DISCONNECT "Не отключайте питание"
#define INTL_DISP_OTA_FAILED "Ошибка обновления"
#define INTL_DISP_OTA_WILL_RETRY "Повтор позже"
#define INTL_DISP_OTA_SUCCESS "Прошивка обновлена"
#define INTL_DISP_OTA_RESTARTING "Перезагрузка..."
#define INTL_DISP_WAITING_URBAN_ID "Ожидание Urban ID..."
#define INTL_DISP_URBAN_IP "IP Urban:"
#define INTL_DISP_INSIGHT_IP "IP Insight:"
#define INTL_DISP_SD_CARD "SD карта:"
#define INTL_DISP_WIFI_STATUS "Статус WiFi:"
#define INTL_DISP_WIFI_NAME "Имя WiFi:"
#define INTL_DISP_UNIQUE_ADDR "Уник. адрес:"
#define INTL_DISP_DEVICE_INFO "Информация об устройстве"
#define INTL_DISP_SCAN_FOR_MORE "Сканируйте для подробностей"
#define INTL_DISP_NO_DATA "--"
#define INTL_DISP_TEMPERATURE "Температура"
#define INTL_DISP_HUMIDITY "Влажность"
#define INTL_DISP_PRESSURE "Давление"
#define INTL_DISP_AIR "Воздух"
#define INTL_DISP_AIR_QUALITY "Качество воздуха"
#define INTL_DISP_NOISE "Шум"
#define INTL_DISP_MAIN_URBAN "URBAN"
#define INTL_DISP_MAIN_INSIGHT "INSIGHT"
#define INTL_DISP_SENSORS_MAP "КАРТА ДАТЧИКОВ"
#define INTL_DISP_EXPLORE_ADVANTAGES "Изучите все возможности"
#define INTL_DISP_EXPLORE_ENVIRONMENT "Исследуйте окружающую среду"
#define INTL_DISP_EXPLORE_YOUR "Исследуйте вашу"
#define INTL_DISP_ENVIRONMENT_CAPS "ОКРУЖАЮЩУЮ СРЕДУ"
#define INTL_DISP_MAP_ENV_BETTER "Узнайте об окружающей среде."
#define INTL_DISP_MAP_REVIEW_INSIGHTS "Анализ за разные периоды."
#define INTL_DISP_MAP_COMPARE_CONDITIONS "Сравните с соседями."
#define INTL_DISP_SCAN_TO_OPEN "Сканируйте для перехода"
#define INTL_DISP_POWERED_BY "Powered by Robonomics"
#define INTL_DISP_POWERED "При поддержке"
#define INTL_DISP_BY_ROBONOMICS "Robonomics"
#define INTL_DISP_NOT_CONNECTED "Нет подключения"
#define INTL_DISP_CONNECTED "Подключено"
#define INTL_DISP_DISCONNECTED "Отключено"
#define INTL_DISP_NOT_SET "Не задано"
#define INTL_DISP_NOISE_MAX "Шум макс."
#define INTL_DISP_NOISE_AVG "Шум ср."
#define INTL_DISP_NO_DATA_AVAILABLE "Нет данных"
#define INTL_DISP_NOT_ENOUGH_DATA_YET "Пока недостаточно данных"
#define INTL_DISP_COLLECTING_DATA "Сбор данных..."
#define INTL_DISP_LOADING "Загрузка..."
#define INTL_DISP_ANALYTICS_C_LEGEND "C=Консервативная"
#define INTL_DISP_ANALYTICS_B_LEGEND "B=Биохакинг"
#define INTL_DISP_ANALYTICS_GRADE "Оценка"
#define INTL_DISP_ANALYTICS_COL_METRIC "Метрика"
#define INTL_DISP_ANALYTICS_COL_MAX "Макс"
#define INTL_DISP_ANALYTICS_COL_MIN "Мин"
#define INTL_DISP_ANALYTICS_COL_CONSERV "Консерв"
#define INTL_DISP_ANALYTICS_COL_BIOHACK "Биохак"
#define INTL_DISP_ANALYTICS_ROW_CO2 "CO2 ppm"
#define INTL_DISP_ANALYTICS_ROW_TEMP "Температура C"
#define INTL_DISP_ANALYTICS_ROW_HUM "Влажность %"
#define INTL_DISP_ANALYTICS_ROW_PM25 "PM2.5 мкг/м3"
#define INTL_DISP_ANALYTICS_ROW_NOISE "Шум дБ"
#define INTL_DISP_ANALYTICS_AT "в"
#define INTL_DISP_ANALYTICS_HOUR_SUFFIX "ч"
#define INTL_DISP_INFO_LABEL "Инфо:"
#define INTL_DISP_LEVEL_HIGH "выше нормы"
#define INTL_DISP_LEVEL_LOW "ниже нормы"
#define INTL_DISP_IS_TOO ""
#define INTL_DISP_CHECK_MAP_FULL_DATA "Смотрите полные данные и аналитику на карте датчиков."
#define INTL_STANDALONE_SHOP_PROMPT "Больше измерений для вашего дома"
/** Insight standalone: второй ряд футера — текст у QR (Font8, перенос). */
#define INTL_STANDALONE_INSIGHT_FOOTER_PROMPT \
    "Добавьте шум, пыль, воздух и уличные замеры к Insight —  cyberpunks.shop."
#define INTL_DISP_DEW_POINT_U_PREFIX "Точка росы (U): "
#define INTL_DISP_DEW_POINT_IS "Точка росы: "
#define INTL_DISP_TEMP_SHORT "Темп."
#define INTL_DISP_PRESS_SHORT "Давл."
#define INTL_DISP_NOISE_AVGMAX_SUFFIX "(ср | макс)"
