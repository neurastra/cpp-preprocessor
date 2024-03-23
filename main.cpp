#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace std;
using filesystem::path;

const regex LIBRE{R"/(\s*#\s*include\s*<([^>]*)>\s*)/"};
const regex HEADER{R"/(\s*#\s*include\s*"([^"]*)"\s*)/"};

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

// напишите эту функцию
bool PreProcessing(const path& file_path, const vector<path>& include_directories, istream& source, ostream& destination) { 
    string line = ""s; 
    int line_number = 1; 
    while (getline(source, line)) { 
        smatch m, m_1; 
        if (!regex_match(line, m_1, LIBRE) && !regex_match(line, m, HEADER)) { 
            destination << line << endl; 
        } else { 
            ifstream input; 
            path path, path_1; 
            if (m.empty()) { 
                path = string(m_1[1]); 
            } else { 
                path = string(m[1]); 
                path_1 = file_path.parent_path() / path; 
                input.open(path_1); 
            } 
            for (const auto& entry_dir : include_directories) { 
                if (input.is_open() && input) { 
                    break; 
                } 
                path_1 = entry_dir / path; 
                input.open(path_1); 
            } 
            if (!input) { 
                cout << "unknown include file "sv << path.string() << " at file "sv << file_path.string() << " at line "sv << line_number << endl; 
                return false; 
            } 
            if (!PreProcessing(path_1, include_directories, input, destination)) { 
                return false; 
            } 
        } 
        ++line_number; 
    } 
    return true; 
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) { 
     
    ifstream input(in_file);  
    if (!input) { 
        cerr << in_file << endl; 
        return false; 
    } 
    ofstream output(out_file); 
    if (!output) { 
        cerr << out_file << endl; 
        return false; 
    } 
    return PreProcessing(in_file, include_directories, input, output); 
} 

string GetFileContents(string file) { 
    ifstream stream(file); 

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()}; 
} 

void Test() { 
    error_code err; 
    filesystem::remove_all("sources"_p, err); 
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err); 
    filesystem::create_directories("sources"_p / "include1"_p, err); 
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err); 
 
    { 
        ofstream file("sources/a.cpp"); 
        file << "// this comment before include\n" 
                "#include \"dir1/b.h\"\n" 
                "// text between b.h and c.h\n" 
                "#include \"dir1/d.h\"\n" 
                "\n" 
                "int SayHello() {\n" 
                "    cout << \"hello, world!\" << endl;\n" 
                "#   include<dummy.txt>\n" 
                "}\n"s; 
    } 
    { 
        ofstream file("sources/dir1/b.h"); 
        file << "// text from b.h before include\n" 
                "#include \"subdir/c.h\"\n" 
                "// text from b.h after include"s; 
    } 
    { 
        ofstream file("sources/dir1/subdir/c.h"); 
        file << "// text from c.h before include\n" 
                "#include <std1.h>\n" 
                "// text from c.h after include\n"s; 
    } 
    { 
        ofstream file("sources/dir1/d.h"); 
        file << "// text from d.h before include\n" 
                "#include \"lib/std2.h\"\n" 
                "// text from d.h after include\n"s; 
    } 
    { 
        ofstream file("sources/include1/std1.h"); 
        file << "// std1\n"s; 
    } 
    { 
        ofstream file("sources/include2/lib/std2.h"); 
        file << "// std2\n"s; 
    } 
 
    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p, 
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p}))); 
 
    ostringstream test_out; 
    test_out << "// this comment before include\n" 
                "// text from b.h before include\n" 
                "// text from c.h before include\n" 
                "// std1\n" 
                "// text from c.h after include\n" 
                "// text from b.h after include\n" 
                "// text between b.h and c.h\n" 
                "// text from d.h before include\n" 
                "// std2\n" 
                "// text from d.h after include\n" 
                "\n" 
                "int SayHello() {\n" 
                "    cout << \"hello, world!\" << endl;\n"s; 
 
    assert(GetFileContents("sources/a.in"s) == test_out.str()); 
} 

int main() {
    Test();
}
