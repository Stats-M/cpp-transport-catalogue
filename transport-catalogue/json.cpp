#include "json.h"

using namespace std;

namespace json
{

// ---------------------------------------------------------------------------
// ������������ ���� ���������� ������������ ������� ��������� �������� ������
namespace
{

// ������� ������������ ��� �������� ����� �� ����� (������, ������ string)
static std::unordered_map<char, char> esc_symbols_load = {
    {'\\', '\\'},
    {'"', '"'},
    {'n', '\n'},
    {'r', '\r'},
    {'/', '/'},
    {'b', '\b'},
    {'f', '\f'},
    {'t', '\t'}
};

Node LoadNode(istream& input); // Forward declaration. See definition below.

// ������� ��������� ������ ���� vector<Node>
Node LoadArray(istream& input)
{

    // ���� �� ��� ������?
    if (input.peek() == std::char_traits<char>::eof())     // std::char_traits<char>::eof() == -1
    {
        throw ParsingError("LoadArray - unexpected end of file"s);
    }
    else if (input.peek() == ',')
    {
        throw ParsingError("LoadArray - wrong format"s);
    }

    Array result;

    for (char c; input >> c && c != ']';)
    {
        if (c != ',')
        {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    return Node(result);
}

/* OBSOLETE CODE - v1 JSON
Node LoadInt(istream& input)
{
    int result = 0;
    while (isdigit(input.peek()))
    {
        result *= 10;
        result += input.get() - '0';
    }
    return Node(result);
}
*/

// ������� ��������� ������ ���� string
Node LoadString(istream& input)
{
    // using namespace std::literals;

    // �� ������ ����� >>. ��� ��������������� ��� ������! (FormattedInputFunctions)
    // �� ������ ���� ��������� ������� � esc-������������������.

    /*
    ������ LoadString: ����� ������ ���� ��� ����������� ������� �������, ��������� �� �����������
    ���� ���� ���� �������{
        ���� ������ �, ����������� ������
        ���� ������ \, ������ ��� ���� ������(����� get()), ���� ����������(���� ����� �� ����� : ����������) � ��������� � ����� ������ ������.
        ���� ������ \n ��� \r: ����������, ������ �� ����� ���������� �� ��������� �����
        � ��������� ������� ��������� ������ � ������
    }
        �� ����� �� ����� ������(eof) ? ���� ��, �� ����������
    */

    // ���� �� ��� ������?
    if (input.peek() == std::char_traits<char>::eof())     // std::char_traits<char>::eof() == -1
    {
        throw ParsingError("LoadString - unexpected end of file"s);
    }

    std::string line;
    char c;
    // ������ �����������
    while (input.get(c))
    {
        // ������� �������� ������ ������
        if (c == '\\')
        {
            // �������� ����������� ������. ������ ��������� �� ���, ���� �� ����
            if (input.peek() != std::char_traits<char>::eof())
            {
                input.get(c);
                if (esc_symbols_load.find(c) != esc_symbols_load.end())
                {
                    // ������ ���������� ����������� ������. ���������� ���
                    c = esc_symbols_load[c];
                }
                else
                {
                    // ����� ��������� ����� ������ ������������ ������
                    throw ParsingError("LoadString - wrong control character: \\"s + c);
                }
            }
        }
        else if (c == '"')
        {
            // ��������� ������ ����� ������. ���������� ��������� ��������� (��� �������)
            return Node(line);
        }

        // ��������� ��������� ������ � �������� ������
        line += c;
    } // while()

    // ���� �� �����, ������, ��������� ����� �� ����� �� ������� 
    // ����������� ������� ����� ������. ������.
    throw ParsingError("LoadString - closing double quote is missing"s);
}

// ������� ��������� ������ ���� map<string, Node>
Node LoadDict(istream& input)
{

    // ���� �� ��� ������?
    if (input.peek() == std::char_traits<char>::eof())     // std::char_traits<char>::eof() == -1
    {
        throw ParsingError("LoadDict - unexpected end of file"s);
    }
    else if (input.peek() == ',')
    {
        throw ParsingError("LoadDict - wrong format"s);
    }

    Dict result;

    for (char c; input >> c && c != '}';)
    {
        if (c == ',')
        {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({ move(key), LoadNode(input) });
    }

    return Node(result);
}

// ������� ��������� ������ ���� bool ��� �������� null
Node LoadBoolNull(istream& input)
{
    // using namespace std::literals;
        
    // ��������� �������� [t]rue, [f]alse, [n]ull (������ ����� ������� �����)
    // �������� ��������� ����������� ���� 3 �������
    char str_buf[4];
    // cppreference.com
    // .get(buf, count) - Same as get(s, count, widen('\n')), that is, reads at most count-1 characters...
    input.get(str_buf, 4);
    // �� ��������� eof: ���� � ������ ������ ������ ���, eof ����� true, ���� ������ ���������
    // ������ ����� �������� ������� �������� ���������, ������ ���� 3
//    if (!input.eof())
    if (input.gcount() == 3)
    {
        if (0 == std::memcmp(str_buf, "rue", 3))
        {
            return Node{ true };
        }
        else if (0 == std::memcmp(str_buf, "ull", 3))
        {
            return Node{ nullptr };
        }
        // ��� ������ false ����� �������� 1 ������ �� ������.
        // ����� �� � eof(), ������ 1 ������� ���������
        else if ((0 == std::memcmp(str_buf, "als", 3)) && (input.get() == 'e'))
        {
            return Node{ false };
        }
        else
        {
            // ������. �� ���������� �� ���� �� �������� ��������
            throw ParsingError("LoadBoolNull - invalid data: "s + std::string(str_buf));
        }
    }
    else
    {
        // ������. �� ������ �������� ������� bool ��� null, �� ����� ���������� ������
        throw ParsingError("LoadBoolNull - unexpected end of file on function entry. Data: "s + std::string(str_buf));
    }
}

// ������� ��������� ������ ���� int ��� double
Node LoadNumber(istream& input)
{
    // using namespace std::literals;

    std::string parsed_num;

    // ������. ��������� � parsed_num ��������� ������ �� input
    auto read_char = [&parsed_num, &input]
    {
        parsed_num += static_cast<char>(input.get());
        if (!input)
        {
            throw ParsingError("LoadNumber::read_char - Failed to read number from stream"s);
        }
    };

    // ������. ��������� ���� ��� ����� ���� � parsed_num �� input
    auto read_digits = [&input, read_char]
    {
        if (!std::isdigit(input.peek()))
        {
            throw ParsingError("LoadNumber::read_digits - A digit is expected"s);
        }
        while (std::isdigit(input.peek()))
        {
            read_char();
        }
    };

    // ��������� ���� �����, ���� �� ����
    if (input.peek() == '-')
    {
        read_char();
    }
    // ������ ����� ����� �����
    if (input.peek() == '0')
    {
        // ����� 0 � JSON �� ����� ���� ������ �����, ���������  ������ ��� 0
        read_char();
    }
    else
    {
        // ��������� ��� ����� ����� ����
        read_digits();
    }

    bool is_int = true;
    // ������ ������� ����� �����
    if (input.peek() == '.')
    {
        read_char();
        read_digits();
        is_int = false;
    }

    // ������ ���������������� ����� �����
    if (int ch = input.peek(); ch == 'e' || ch == 'E')
    {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-')
        {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try
    {
        if (is_int)
        {
            // ������� ������� ������������� ������ � int
            try
            {
                int val = std::stoi(parsed_num);
                // ���� ���������� - ���������� ���������
                return Node{ val };
            }
            catch (...)
            {
                // � ������ �������, ��������, ��� ������������,
                // ��� ���� ��������� ������������� ������ � double
            }
        }
        double d_val = std::stod(parsed_num);
        // ���� ���������� - ���������� ���������
        return Node{ d_val };
    }
    catch (...)
    {
        throw ParsingError("LoadNumber - Failed to convert "s + parsed_num + " to number"s);
    }
}


// ����� ����� ��������� ������ JSON ��������� �� ������, ����  
// ����� ������ ������ ���������� ���� (������, �������).
// ���������� �������� ����, �� ������ �������� ����� �������
// ������ ���� json::Document
Node LoadNode(istream& input)
{
    char c;
    
    // ������ ����� ��������������� ���� >>, �.�. ��� ������ ��������� 
    // � �� ����� ���������� ��� ������� � �����������, ������� ��� ���� �� ������
    if (input >> c)
    {
        // ����� �� ���� (input.eof() != true). ����������� ���������� ������ 

        if (c == '[')
        {
            // ������ ������
            return LoadArray(input);
        }
        else if (c == '{')
        {
            // ������ �������
            return LoadDict(input);
        }
        else if (c == '"')
        {
            // ������ ������
            return LoadString(input);
        }
        else if ((c == 't') || (c == 'f') || (c == 'n'))
        {
            // ������ bool ��� null. ������ ������� � ����� �� ����������
            return LoadBoolNull(input);
        }
        else
        {
            // ������ ����� int ��� double.
            // ���������� ������� ������ � �����
            input.putback(c);
            //return LoadInt(input);  // OBSOLETE CODE - v1 JSON
            return LoadNumber(input);
        }
    }
    else
    {
        // ������� ����������
        throw ParsingError("LoadNode - unexpected end of file"s);
    }
}

}  // namespace


// ----------------- Node -------------------
// ---------������ �������� ���� Is*---------
bool Node::IsNull() const
{
    return std::holds_alternative<std::nullptr_t>(json_node_var_);
}

bool Node::IsInt() const
{
    return std::holds_alternative<int>(json_node_var_);
}

bool Node::IsDouble() const
{
    // ������ true � ��� int (int - ������������ ����� double)
    return std::holds_alternative<double>(json_node_var_)
        || std::holds_alternative<int>(json_node_var_);
}

bool Node::IsPureDouble() const
{
    return std::holds_alternative<double>(json_node_var_) 
        && !std::holds_alternative<int>(json_node_var_);
}

bool Node::IsString() const
{
    return std::holds_alternative<std::string>(json_node_var_);
}

bool Node::IsBool() const
{
    return std::holds_alternative<bool> (json_node_var_);
}

bool Node::IsArray() const
{
    return std::holds_alternative<Array>(json_node_var_);
}

bool Node::IsMap() const
{
    return std::holds_alternative<Dict>(json_node_var_);
}

// ------------------- Node ---------------------
// -----������ ������������ ������� ���� As*-----
const Array& Node::AsArray() const
{
    if (IsArray())
    {
        return std::get<Array>(json_node_var_);
    }
    else
    {
        throw std::logic_error("Node::AsArray cant return non-array");
    }
}

const Dict& Node::AsMap() const
{
    if (IsMap())
    {
        return std::get<Dict>(json_node_var_);
    }
    else
    {
        throw std::logic_error("Node::AsMap cant return non-map");
    }
}

bool Node::AsBool() const
{
    if (IsBool())
    {
        return std::get<bool>(json_node_var_);
    }
    else
    {
        throw std::logic_error("Node::AsBool cant return non-bool");
    }
}

int Node::AsInt() const
{
    if (IsInt())
    {
        return std::get<int>(json_node_var_);
    }
    else
    {
        throw std::logic_error("Node::AsInt cant return non-int");
    }
}

double Node::AsDouble() const
{
    if (IsInt())
    {
        return std::get<int>(json_node_var_) * 1.0L;
    }
    else if (IsDouble())
    {
        return std::get<double>(json_node_var_);
    }
    else
    {
        throw std::logic_error("Node::AsDouble cant return non-double");
    }
}

const std::string& Node::AsString() const
{
    if (IsString())
    {
        return std::get<std::string>(json_node_var_);
    }
    else
    {
        throw std::logic_error("Node::AsString cant return non-string");
    }
}

const JSON_node& Node::GetValue() const
{
    return json_node_var_;
}

// ------------------- Node ---------------------
// -----------------���������--------------------
bool Node::operator==(const Node& rhs) const
{
    // ���������� ��������, � �� ������
    return (json_node_var_ == rhs.json_node_var_);
    //return (*this == rhs);
}

bool Node::operator!=(const Node& rhs) const
{
    return !(*this == rhs);
}

// --------------- Document ---------------------

Document::Document(Node root)
    : root_(move(root))
{}

const Node& Document::GetRoot() const
{
    return root_;
}
bool Document::operator==(const Document& rhs) const
{
    // ���������� ��������, � �� ������
    return (this->GetRoot().GetValue() == rhs.GetRoot().GetValue());
    //return (*this == rhs);
}

bool Document::operator!=(const Document& rhs) const
{
    return !(*this == rhs);
}

// ----------- Non-class functions ---------------

Document Load(istream& input)
{
    // ������� JSON �������� �� ������ ��������� ����, 
    // ������������� �������� LoadNode()
    return Document{ LoadNode(input) };
}

void Print(const Document& doc, std::ostream& output)
{
    std::visit(OstreamSolutionPrinter{ output }, doc.GetRoot().GetValue());
}

}  // namespace json