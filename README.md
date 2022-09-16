### cpp-transport-catalogue
# Транспортный справочник / роутер
Система хранения транспортных маршрутов и обработки запросов к ней:
- Входные данные и ответ в JSON-формате.
- Выходной JSON-файл может содержать визуализацию карты маршрута(ов) в формате SVG-файла.  
- Поиск кратчайшего маршрута. 
- Сериализация базы данных и настроек справочника при помощи Google Protobuf. 
- Объекты JSON поддерживают цепочки вызовов (method chaining) при конструировании, превращая ошибки применения данных формата JSON в ошибки компиляции.

### Использованные идеомы, технологии и элементы языка
- OOP: inheritance, abstract interfaces, final classes
- Unordered map/set
- STL smart pointers
- std::variant and std:optional
- JSON load / output
- SVG image format embedded inside XML output
- Curiously Recurring Template Pattern (CRTP)
- Method chaining
- Facade design pattern
- C++17 with С++20 Ranges emulation
- Directed Weighted Graph data structure for Router module
- Google Protocol Buffers for data serialization
- Static libraries .LIB/.A
- CMake generated project and dependency files

## Сборка проекта (для Visual Studio)
1. Скачать архив Google Protobuf с [официального репозитория](https://github.com/protocolbuffers/protobuf/releases) и разархивировать его на вашем компьютере в папку, не содержащую русских символов и пробелов в имени и/или пути к ней. В данном проекте использовался Protobuf 21.1
2. Создать внутри распакованного архива папку `my_build` и открыть ее в командной строке.
3. Сгенерировать проект Protobuf для VS при помощи cmake:
`cmake ../cmake -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=path\proto-21-1-src\my_build -Dprotobuf_MSVC_STATIC_RUNTIME=OFF`
4. Для сборки проектов из командной строки, создайте внутри папки `my_build` папки `Debug` и `Release` и выполните команды
```
cmake --build . --config Debug
cmake --install . --config Debug
cmake --build . --config Release
cmake --install . --config Release
```
5. Выполнять команду `cmake --install` необязательно, если вы планируете компоновать файлы библиотеки вручную.
6. =или= Для сборки проектов при помощи среды VS откройте файл решения `my_build\protobuf.sln` в среде VS, выберите требуемые конфигурации (Debug, Release) и/или целевые платформы и соберите проект как обычно.
7. Добавьте папки со сгенерированными .lib библиотеками protobuf в дополнительные зависимости проекта транспортного справочника (Additional Include Directories, Additional Dependencies и пр.).

## Запуск программы
Приложение транспортного справочника спроектировано для работы в 2 режимах: режиме создания базы данных и режиме запросов к базе данных.

Для создания базы данных транспортного справочника с последующей ее сериализацией в файл необходимо запустить программу с параметром make_base. Входные данные поступают из stdin, поэтому можно переопределить источник данных, например, указав входной JSON-файл, из которого будет взята информация для наполнения базы данных вместо stdin.
Пример:  
`transport_catalogue.exe make_base <input_data.json`

Для обработки запросов к созданной базе данных (сама база данных десериализуется из ранее созданного файла) необходимо запустить программу с параметром process_requests, указав входной JSON-файл, содержащий запрос(ы) к БД и выходной файл, который будет содержать ответы на запросы, также в формате JSON.  
Пример:  
`transport_catalogue.exe process_requests <requests.json >output.txt`

## Формат входных данных
Входные данные принимаются из stdin в JSON формате. Структура верхнего уровня имеет следующий вид:  
```
{
  "base_requests": [ ... ],
  "render_settings": { ... },
  "routing_settings": { ... },
  "serialization_settings": { ... },
  "stat_requests": [ ... ]
}
```  
Каждый элемент является словарем, содержащим следующие данный:  
`base_requests` — описание автобусных маршрутов и остановок.  
`stat_requests` — запросы к транспортному справочнику.  
`render_settings` — настройки рендеринга карты в формате .SVG.  
`routing_settings` — настройки роутера для поиска кратчайших маршрутов.  
`serialization_settings` — настройки сериализации/десериализации данных.

#### Примеры входного файла и файла с запросом к справочнику прилагаются.
