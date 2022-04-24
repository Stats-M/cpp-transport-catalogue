#include "json.h"

using namespace std;

namespace json
{

// ---------------------------------------------------------------------------
// Пространство имен внутренних внеклассовых функций обработки входного потока
namespace
{

// Таблица спецсимволов для загрузки строк из файла (потока, строки string)
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

// Функция считывает данные типа vector<Node>
Node LoadArray(istream& input)
{

    // Есть ли что читать?
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

// Функция считывает данные типа string
Node LoadString(istream& input)
{
    // using namespace std::literals;

    // Не читаем через >>. Это форматированный тип чтения! (FormattedInputFunctions)
    // Мы должны сами разбирать символы и esc-последовательности.

    /*
    Логика LoadString: поток пришел сюда без открывающей двойной кавычки, считываем до закрывающей
    цикл пока есть символы{
        если символ “, заканчиваем читать
        если символ \, читаем ещё один символ(через get()), ищем комбинацию(если такую не знаем : исключение) и вставляем в буфер нужный символ.
        если символ \n или \r: исключение, строка не может закончится на следующей линии
        в остальных случаях добавляем символ в строку
    }
        мы дошли до конца потока(eof) ? если да, то исключение
    */

    // Есть ли что читать?
    if (input.peek() == std::char_traits<char>::eof())     // std::char_traits<char>::eof() == -1
    {
        throw ParsingError("LoadString - unexpected end of file"s);
    }

    std::string line;
    char c;
    // Читаем посимвольно
    while (input.get(c))
    {
        // Сначала проверим особые случаи
        if (c == '\\')
        {
            // Прочитан управляющий символ. Читаем следующий за ним, если он есть
            if (input.peek() != std::char_traits<char>::eof())
            {
                input.get(c);
                if (esc_symbols_load.find(c) != esc_symbols_load.end())
                {
                    // Считан корректный управляющий символ. Запоминаем его
                    c = esc_symbols_load[c];
                }
                else
                {
                    // После обратного слэша считан недопустимый символ
                    throw ParsingError("LoadString - wrong control character: \\"s + c);
                }
            }
        }
        else if (c == '"')
        {
            // Обнаружен символ конца строки. Возвращаем имеющийся результат (без кавычки)
            return Node(line);
        }

        // Добавляем считанный символ к итоговой строке
        line += c;
    } // while()

    // Если мы здесь, значит, произошел выход из цикла до момента 
    // обнаружения символа конца строки. Ошибка.
    throw ParsingError("LoadString - closing double quote is missing"s);
}

// Функция считывает данные типа map<string, Node>
Node LoadDict(istream& input)
{

    // Есть ли что читать?
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

// Функция считывает данные типа bool или значение null
Node LoadBoolNull(istream& input)
{
    // using namespace std::literals;
        
    // Возможные варианты [t]rue, [f]alse, [n]ull (первые буквы считаны ранее)
    // Пытаемся прочитать минимальный блок 3 символа
    char str_buf[4];
    // cppreference.com
    // .get(buf, count) - Same as get(s, count, widen('\n')), that is, reads at most count-1 characters...
    input.get(str_buf, 4);
    // Не проверяем eof: если в потоке ничего больше нет, eof будет true, хотя данные прочитаны
    // Вместо этого проверим сколько символов прочитали, должно быть 3
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
        // Для случая false нужно дочитать 1 символ из потока.
        // Поток не в eof(), чтение 1 символа допустимо
        else if ((0 == std::memcmp(str_buf, "als", 3)) && (input.get() == 'e'))
        {
            return Node{ false };
        }
        else
        {
            // Ошибка. Не обнаружено ни одно из валидных значений
            throw ParsingError("LoadBoolNull - invalid data: "s + std::string(str_buf));
        }
    }
    else
    {
        // Ошибка. Мы должны прочесть остаток bool или null, но поток закончился раньше
        throw ParsingError("LoadBoolNull - unexpected end of file on function entry. Data: "s + std::string(str_buf));
    }
}

// Функция считывает данные типа int или double
Node LoadNumber(istream& input)
{
    // using namespace std::literals;

    std::string parsed_num;

    // Лямбда. Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input]
    {
        parsed_num += static_cast<char>(input.get());
        if (!input)
        {
            throw ParsingError("LoadNumber::read_char - Failed to read number from stream"s);
        }
    };

    // Лямбда. Считывает одну или более цифр в parsed_num из input
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

    // Считываем знак числа, если он есть
    if (input.peek() == '-')
    {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0')
    {
        // После 0 в JSON не могут идти другие цифры, считываем  только сам 0
        read_char();
    }
    else
    {
        // Считываем все цифры какие есть
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.')
    {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
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
            // Сначала пробуем преобразовать строку в int
            try
            {
                int val = std::stoi(parsed_num);
                // Если получилось - возвращаем результат
                return Node{ val };
            }
            catch (...)
            {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        double d_val = std::stod(parsed_num);
        // Если получилось - возвращаем результат
        return Node{ d_val };
    }
    catch (...)
    {
        throw ParsingError("LoadNumber - Failed to convert "s + parsed_num + " to number"s);
    }
}


// Точка входа алгоритма чтения JSON документа из потока, либо  
// точка начала чтения составного узла (массив, словарь).
// Возвращает корневой узел, на основе которого можно создать
// объект типа json::Document
Node LoadNode(istream& input)
{
    char c;
    
    // Читаем через форматированный ввод >>, т.к. это начало документа 
    // и мы можем пропустить все пробелы и спецсимволы, которых тут быть не должно
    if (input >> c)
    {
        // Поток не пуст (input.eof() != true). Анализируем полученный символ 

        if (c == '[')
        {
            // Читаем массив
            return LoadArray(input);
        }
        else if (c == '{')
        {
            // Читаем словарь
            return LoadDict(input);
        }
        else if (c == '"')
        {
            // Читаем строку
            return LoadString(input);
        }
        else if ((c == 't') || (c == 'f') || (c == 'n'))
        {
            // Читаем bool или null. Символ обратно в поток НЕ возвращаем
            return LoadBoolNull(input);
        }
        else
        {
            // Читаем число int или double.
            // Возвращаем обратно символ в поток
            input.putback(c);
            //return LoadInt(input);  // OBSOLETE CODE - v1 JSON
            return LoadNumber(input);
        }
    }
    else
    {
        // Бросаем исключение
        throw ParsingError("LoadNode - unexpected end of file"s);
    }
}

}  // namespace


// ----------------- Node -------------------
// ---------Группа проверок типа Is*---------
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
    // Вернет true и для int (int - подмножество чисел double)
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
// -----Группа возвращающих методов типа As*-----
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
// -----------------Операторы--------------------
bool Node::operator==(const Node& rhs) const
{
    // Сравниваем значения, а не адреса
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
    // Сравниваем значения, а не адреса
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
    // Создаем JSON документ на основе корневого узла, 
    // возвращаемого функцией LoadNode()
    return Document{ LoadNode(input) };
}

void Print(const Document& doc, std::ostream& output)
{
    std::visit(OstreamSolutionPrinter{ output }, doc.GetRoot().GetValue());
}

}  // namespace json