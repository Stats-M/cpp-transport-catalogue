#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <type_traits>
#include <cstring>    // for memcmp
#include <sstream>    // for istringstream

namespace json
{

class Node;  // Forward declaration. See declaration below
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;
//using Number = std::variant<int, double>;
using JSON_node = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error
{
public:
    using runtime_error::runtime_error;
};

// Класс узла JSON документа. Дочерние узлы являются значениями в 
// словаре родительского узла, т.е. достаточно 1 узла в документе 
// чтобы хранить в нем любое количество дочерних узлов
class Node
{
public:
    
    Node() : json_node_var_(nullptr)
    {}

    template<typename T>
    Node(T value) : json_node_var_(std::move(value))
    {}

    bool IsNull() const;        // В json_node_var_ хранится nullptr?
    bool IsInt() const;         // В json_node_var_ хранится int?
    bool IsDouble() const;      // В json_node_var_ хранится int или double?
    bool IsPureDouble() const;  // В json_node_var_ хранится строго double?
    bool IsString() const;      // В json_node_var_ хранится string?
    bool IsBool() const;        // В json_node_var_ хранится bool?
    bool IsArray() const;       // В json_node_var_ хранится vector?
    bool IsMap() const;         // В json_node_var_ хранится map?

    const Array& AsArray() const;
    const Dict& AsMap() const;
    bool AsBool() const;
    int AsInt() const;
    double AsDouble() const;
    const std::string& AsString() const;

    const JSON_node& GetValue() const;

    // Перегруженный сокращенный оператор сравнения для Node
    bool operator==(const Node& rhs) const;
    // Перегруженный сокращенный оператор неравенства для Node
    bool operator!=(const Node& rhs) const;
private:
    JSON_node json_node_var_;    // std::variant
};

// Класс JSON документа без методов манипуляции им (реализованы внешними функциями)
class Document
{
public:
    // Явный конструктор Document на основе корневого узла Node
    explicit Document(Node root);

    const Node& GetRoot() const;

    // Перегруженный сокращенный оператор сравнения для Document
    bool operator==(const Document& rhs) const;
    // Перегруженный сокращенный оператор неравенства для Document
    bool operator!=(const Document& rhs) const;
private:
    // Корневой узел. Является словарем для дочерних узлов при их наличии
    Node root_;
};

// Загружает и возвращает JSON документ из потока istream
Document Load(std::istream& input);

// Выводит JSON документ в поток ostream, начиная с корневого узла
void Print(const Document& doc, std::ostream& output);

// Таблица спецсимволов для сохранения строк в файл (поток, печать...)
static std::unordered_map<char, std::string> esc_symbols_save = {
    {'\\', std::string("\\\\")},
    {'"', std::string("\\\"")},
    {'\n', std::string("\\n")},
    {'\r',std::string("\\r")},
//    {'/', std::string("\\/")},  отключено, не требуется по заданию
    {'\b',std::string("\\b")},
    {'\f',std::string("\\f")}
    //{'\t',std::string("\\t")} - при печати это не нужно экранировать
};

// Структура для использования с паттерном Посетитель 
struct OstreamSolutionPrinter
{
    std::ostream& out;

    void operator()(std::nullptr_t) const
    {
        using namespace std::literals;
        out << "null"sv;  // endl не выводим, т.к. символ /n добавляется к выводу
    }
    void operator()(Array array) const
    {
        using namespace std::literals;
        out << "["sv << std::endl;
        bool print_comma = false;
        for (const auto& elem : array)
        {
            if (print_comma)
            {
                out << ", "sv << std::endl;
            }
            std::visit(OstreamSolutionPrinter{ out }, elem.GetValue());
            print_comma = true;
        }
        out << std::endl << "]"sv;
    }
    void operator()(Dict map) const
    {
        using namespace std::literals;
        out << "{"sv << std::endl;
        bool print_comma = false;
        for (const auto& elem : map)
        {
            if (print_comma)
            {
                out << ", "sv << std::endl;
            }
            out << "\""sv << elem.first << "\": "sv;
            std::visit(OstreamSolutionPrinter{ out }, elem.second.GetValue());
            print_comma = true;
        }
        out << std::endl << "}"sv;
    }
    void operator()(bool value) const
    {
        using namespace std::literals;
        if (value)
        {
            out << "true"sv;  // endl не выводим, т.к. символ /n добавляется к выводу
        }
        else
        {
            out << "false"sv;  // endl не выводим, т.к. символ /n добавляется к выводу
        }
    }
    void operator()(int num) const
    {
        out << num;  // endl не выводим, т.к. символ /n добавляется к выводу
    }
    void operator()(double num) const
    {
        out << num;  // endl не выводим, т.к. символ /n добавляется к выводу
    }
    void operator()(std::string line) const
    {
        using namespace std::literals;

        /* Реализация через цикл for each
        out << "\""sv;
        // Посимвольная обработка строки
        for (const auto& ch : line)
        {
            if (esc_symbols_save.find(ch) != esc_symbols_save.end())
            {
                // Считан спецсимвол. Выводим его
                out << esc_symbols_save[ch];
            }
            else
            {
                // Обычный символ, выводим как есть
                out << ch;
            }
        }
        out << "\""sv;
        */

        // Реализация через чтение потока
        std::istringstream strm(line);
        out << "\""sv;
        // Посимвольная обработка строки
        char ch;
        while (strm.get(ch))
        {
            if (esc_symbols_save.find(ch) != esc_symbols_save.end())
            {
                // Считан спецсимвол. Выводим его
                out << esc_symbols_save[ch];
            }
            else
            {
                // Обычный символ, выводим как есть
                out << ch;
            }
        }
        out << "\""sv;


    }
};

}  // namespace json
