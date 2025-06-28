#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>

// Trim whitespace
std::string trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Split print args
std::vector<std::string> split_arguments(const std::string& input)
{
    std::vector<std::string> args;
    std::string current;
    bool in_string = false;

    for (char c : input)
    {
        if (c == '"') in_string = !in_string;
        if (c == ',' && !in_string)
        {
            args.push_back(trim(current));
            current.clear();
        }
        else
        {
            current += c;
        }
    }
    if (!current.empty()) args.push_back(trim(current));
    return args;
}

// Translate print()
std::string translate_print(const std::string& line)
{
    size_t start = line.find("print(");
    size_t end = line.rfind(")");
    if (start == std::string::npos || end == std::string::npos || end <= start + 6)
        return "";

    std::string content = line.substr(start + 6, end - (start + 6));
    std::vector<std::string> args = split_arguments(content);

    std::string translated = "std::cout << ";
    for (size_t i = 0; i < args.size(); ++i)
    {
        translated += args[i];
        if (i != args.size() - 1) translated += " << ";
    }
    translated += " << std::endl;";
    return translated;
}

bool starts_with(const std::string& str, const std::string& prefix)
{
    return str.rfind(prefix, 0) == 0;
}

bool ends_with(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string fix_for_commas(const std::string& line)
{
    size_t start = line.find("for(");
    size_t end = line.find(")");
    if (start == std::string::npos || end == std::string::npos || end <= start + 4) return line;

    std::string inside = line.substr(start + 4, end - (start + 4));
    std::replace(inside.begin(), inside.end(), ',', ';');
    return line.substr(0, start + 4) + inside + line.substr(end);
}

int main()
{
    std::ifstream input("test.cimp");
    std::ofstream output("output.cpp");

    if (!input.is_open())
    {
        std::cerr << "[x] Failed to open test.cimp" << std::endl;
        return 1;
    }

    output << "#include <iostream>\n";
    output << "#include <string>\n";
    output << "using namespace std;\n\n";

    std::string line;
    int line_num = 0;
    bool in_multiline_comment = false;
    int indent_level = 0;
    bool inside_main = false;
    std::vector<std::string> types = {
        "int", "long", "float", "double", "bool", "string",
        "long int", "short int", "long double", "unsigned int",
        "unsigned long", "unsigned short", "unsigned char",
        "signed int", "signed char", "void"
    };

    std::vector<std::string> function_defs;
    std::vector<std::string> main_body;

    while (std::getline(input, line))
    {
        line_num++;
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        if (in_multiline_comment)
        {
            if (trimmed.find("*/") != std::string::npos)
                in_multiline_comment = false;
            continue;
        }
        if (trimmed.find("/*") != std::string::npos)
        {
            in_multiline_comment = true;
            continue;
        }
        if (trimmed.find("//") == 0)
            continue;

        std::string current_line;

        if (starts_with(trimmed, "def "))
        {
            std::string func_def = trimmed.substr(4); // Remove "def "
            if (ends_with(func_def, ":"))
                func_def.pop_back();
            current_line = func_def + " {";
            function_defs.push_back(current_line);
            indent_level = 1;
            inside_main = false;
            continue;
        }
        else if (trimmed == "end")
        {
            indent_level--;
            current_line = std::string(indent_level * 4, ' ') + "}";
        }
        else if (starts_with(trimmed, "print("))
        {
            current_line = std::string(indent_level * 4, ' ') + translate_print(trimmed);
        }
        else if (starts_with(trimmed, "cin(") && trimmed.back() == ')')
        {
            std::string var = trimmed.substr(4, trimmed.size() - 5);
            current_line = std::string(indent_level * 4, ' ') + "cin >> " + var + ";";
        }
        else if (ends_with(trimmed, ":"))
        {
            std::string control = trimmed.substr(0, trimmed.size() - 1);
            if (starts_with(control, "for("))
                control = fix_for_commas(control);
            current_line = std::string(indent_level * 4, ' ') + control + " {";
            indent_level++;
        }
        else
        {
            bool matchedType = false;
            for (const std::string& type : types)
            {
                if (starts_with(trimmed, type + " "))
                {
                    std::string cpp_line = trimmed;
                    if (type == "string") cpp_line = "std::" + trimmed;
                    current_line = std::string(indent_level * 4, ' ') + cpp_line + ";";
                    matchedType = true;
                    break;
                }
            }

            if (!matchedType)
            {
                if (trimmed.find("=") != std::string::npos ||
                    trimmed.find("++") != std::string::npos ||
                    trimmed.find("--") != std::string::npos)
                {
                    current_line = std::string(indent_level * 4, ' ') + trimmed + ";";
                }
                else if (trimmed.find("(") != std::string::npos &&
                         trimmed.find(")") != std::string::npos)
                {
                    current_line = std::string(indent_level * 4, ' ') + trimmed + ";";
                }
                else
                {
                    std::cerr << "[!] Unrecognized line " << line_num << ": " << trimmed << "\n";
                }
            }
        }

        if (!current_line.empty())
        {
            if (starts_with(trimmed, "def "))
                inside_main = false;
            else if (starts_with(trimmed, "main:") || !inside_main)
                main_body.push_back(current_line);
            else
                function_defs.push_back(current_line);
        }
    }

    for (const std::string& func_line : function_defs)
        output << func_line << "\n";

    output << "\nint main() {\n";
    for (const std::string& line : main_body)
        output << line << "\n";
    output << "    return 0;\n";
    output << "}\n";

    input.close();
    output.close();

    std::cout << "[✓] Translated test.cimp → output.cpp\n";

    int compile = system("g++ output.cpp -o run.out");
    if (compile != 0)
    {
        std::cerr << "[x] Compilation failed.\n";
        return 1;
    }

    std::cout << "[✓] Compiled → ./run.out\n";
    std::cout << "[→] Running output:\n\n";
    system("./run.out");
    return 0;
}
