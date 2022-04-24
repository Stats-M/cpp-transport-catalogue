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

// ��� ������ ������ ������������� ��� ������� �������� JSON
class ParsingError : public std::runtime_error
{
public:
    using runtime_error::runtime_error;
};

// ����� ���� JSON ���������. �������� ���� �������� ���������� � 
// ������� ������������� ����, �.�. ���������� 1 ���� � ��������� 
// ����� ������� � ��� ����� ���������� �������� �����
class Node
{
public:
    
    Node() : json_node_var_(nullptr)
    {}

    template<typename T>
    Node(T value) : json_node_var_(std::move(value))
    {}

    bool IsNull() const;        // � json_node_var_ �������� nullptr?
    bool IsInt() const;         // � json_node_var_ �������� int?
    bool IsDouble() const;      // � json_node_var_ �������� int ��� double?
    bool IsPureDouble() const;  // � json_node_var_ �������� ������ double?
    bool IsString() const;      // � json_node_var_ �������� string?
    bool IsBool() const;        // � json_node_var_ �������� bool?
    bool IsArray() const;       // � json_node_var_ �������� vector?
    bool IsMap() const;         // � json_node_var_ �������� map?

    const Array& AsArray() const;
    const Dict& AsMap() const;
    bool AsBool() const;
    int AsInt() const;
    double AsDouble() const;
    const std::string& AsString() const;

    const JSON_node& GetValue() const;

    // ������������� ����������� �������� ��������� ��� Node
    bool operator==(const Node& rhs) const;
    // ������������� ����������� �������� ����������� ��� Node
    bool operator!=(const Node& rhs) const;
private:
    JSON_node json_node_var_;    // std::variant
};

// ����� JSON ��������� ��� ������� ����������� �� (����������� �������� ���������)
class Document
{
public:
    // ����� ����������� Document �� ������ ��������� ���� Node
    explicit Document(Node root);

    const Node& GetRoot() const;

    // ������������� ����������� �������� ��������� ��� Document
    bool operator==(const Document& rhs) const;
    // ������������� ����������� �������� ����������� ��� Document
    bool operator!=(const Document& rhs) const;
private:
    // �������� ����. �������� �������� ��� �������� ����� ��� �� �������
    Node root_;
};

// ��������� � ���������� JSON �������� �� ������ istream
Document Load(std::istream& input);

// ������� JSON �������� � ����� ostream, ������� � ��������� ����
void Print(const Document& doc, std::ostream& output);

// ������� ������������ ��� ���������� ����� � ���� (�����, ������...)
static std::unordered_map<char, std::string> esc_symbols_save = {
    {'\\', std::string("\\\\")},
    {'"', std::string("\\\"")},
    {'\n', std::string("\\n")},
    {'\r',std::string("\\r")},
//    {'/', std::string("\\/")},  ���������, �� ��������� �� �������
    {'\b',std::string("\\b")},
    {'\f',std::string("\\f")}
    //{'\t',std::string("\\t")} - ��� ������ ��� �� ����� ������������
};

// ��������� ��� ������������� � ��������� ���������� 
struct OstreamSolutionPrinter
{
    std::ostream& out;

    void operator()(std::nullptr_t) const
    {
        using namespace std::literals;
        out << "null"sv;  // endl �� �������, �.�. ������ /n ����������� � ������
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
            out << "true"sv;  // endl �� �������, �.�. ������ /n ����������� � ������
        }
        else
        {
            out << "false"sv;  // endl �� �������, �.�. ������ /n ����������� � ������
        }
    }
    void operator()(int num) const
    {
        out << num;  // endl �� �������, �.�. ������ /n ����������� � ������
    }
    void operator()(double num) const
    {
        out << num;  // endl �� �������, �.�. ������ /n ����������� � ������
    }
    void operator()(std::string line) const
    {
        using namespace std::literals;

        /* ���������� ����� ���� for each
        out << "\""sv;
        // ������������ ��������� ������
        for (const auto& ch : line)
        {
            if (esc_symbols_save.find(ch) != esc_symbols_save.end())
            {
                // ������ ����������. ������� ���
                out << esc_symbols_save[ch];
            }
            else
            {
                // ������� ������, ������� ��� ����
                out << ch;
            }
        }
        out << "\""sv;
        */

        // ���������� ����� ������ ������
        std::istringstream strm(line);
        out << "\""sv;
        // ������������ ��������� ������
        char ch;
        while (strm.get(ch))
        {
            if (esc_symbols_save.find(ch) != esc_symbols_save.end())
            {
                // ������ ����������. ������� ���
                out << esc_symbols_save[ch];
            }
            else
            {
                // ������� ������, ������� ��� ����
                out << ch;
            }
        }
        out << "\""sv;


    }
};

}  // namespace json