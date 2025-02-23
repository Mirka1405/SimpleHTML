#include<fstream>
#include<string>
#include<list>
#include<iostream>
#include<sstream>
#include<filesystem>
#include<vector>
namespace fs = std::filesystem;
using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
class Tag{
    public:
    virtual ~Tag() = default;
    std::list<Tag*> children;
    std::string text;
    int indentlevel;
    Tag(int indent=0,std::string txt=""){
        indentlevel=indent;
        text=txt;
    }
    virtual void output(std::ostream& o) {
        std::cout<<"uhhhh"<<std::endl;
    }
    virtual void input(std::istream& in) {
        std::cout<<"hmmmm"<<std::endl;
    }
};

class StringTag:public Tag{
    public:
    char openingchar;
    StringTag(int indent,char openchar) : Tag(indent){
        openingchar=openchar;
    }
    void output(std::ostream& o) override{
        for(char i: text){
            if(i=='\n'&&openingchar!='`'){
                o<<"<br/>";
                continue;
            }
            o<<i;
        }
    }
    void input(std::istream& in) override {
        char c;
        std::stringstream buf;
        bool escaped = false;

        while (in.get(c)) {
            if (escaped) {
                buf << c;
                escaped = false;
                continue;
            }
            if (c == '\\') {
                escaped = true;
                continue;
            }
            if (c == openingchar) {
                break; // End of string
            }
            if (c == '\n') {
                buf << '\n';
                // Skip indentation after newline
                while (in.peek() == ' ') in.get();
            } else {
                buf << c;
            }
        }

        text = buf.str();
    }
};
class SingularTag:public Tag{
    public:
    SingularTag(int indent=0,std::string txt=""):Tag(indent,txt){}
    std::string mainname(){
        return text.substr(0, text.find(' '));
    }
    void output(std::ostream& o) override{
        o<<"<"<<text<<"/>";
    }
    void input(std::istream& in) override{
        char c;
        in.get(c);
        while(!in.eof()&&c!='\n'){
            text+=c;
            in.get(c);
        }
    }
};
class CommentTag:public SingularTag{
    public:
    CommentTag(int indent=0,std::string txt=""):SingularTag(indent,txt){}
    void output(std::ostream& o) override{
        o<<"<!--"<<text<<"-->";
    }
};
class CloseableTag:public SingularTag{
    public:
    ~CloseableTag(){
        for(auto i=children.begin();i!=children.end();i++){
            delete *i;
        }
    }
    CloseableTag(int indent=0,std::string txt=""):SingularTag(indent,txt){}
    void close(std::ostream& o){
        o<<"</"<<mainname()<<">";
    }
    void output(std::ostream& o) override{
        o<<"<"<<text<<">";
        for(auto i = children.begin();i!=children.end();i++){
            (*i)->output(o);
        }
        close(o);
    }
    void input(std::istream& in) override {
        char c;
        text.clear();
        std::getline(in,text);
    }
};
class MainTag:public CloseableTag{
    public:
    ~MainTag() = default;
    MainTag():CloseableTag(-4,"MAIN"){}
    void output(std::ostream& o) override{
        for(auto i = children.begin();i!=children.end();i++){
            (*i)->output(o);
        }
    }
    void input(std::istream& in) override {
        std::vector<Tag*> workingtree{this};
        char c;

        while (in.get(c)) {
            if (c == '\n') continue; // Skip empty lines

            // Put back character and process line
            in.putback(c);
            
            // Read indentation
            int indent = 0;
            while (in.peek() == ' ') {
                in.get();
                indent++;
            }

            // Read tag type
            char typeChar = in.peek();
            if(typeChar=='\n')continue;
            Tag* child = create_child(in, indent);

            // Hierarchy management
            while (!workingtree.empty() && 
                   workingtree.back()->indentlevel >= indent) {
                workingtree.pop_back();
            }
            if (child) {
                workingtree.back()->children.push_back(child);
                workingtree.push_back(child);
            }
        }
    }

    Tag* create_child(std::istream& in, int indent) {
        char typeChar;
        in.get(typeChar);

        if (typeChar == '/') {
            auto tag = new SingularTag(indent);
            std::getline(in, tag->text, '\n');
            return tag;
        } 
        else if (typeChar == '"' || typeChar == '\'' || typeChar == '`') {
            auto tag = new StringTag(indent, typeChar);
            tag->input(in); // Let StringTag handle multi-line reading
            return tag;
        } 
        else if (typeChar == '#') {
            auto tag = new CommentTag(indent);
            std::getline(in, tag->text, '\n');
            return tag;
        } 
        else {
            in.putback(typeChar);
            auto tag = new CloseableTag(indent);
            std::getline(in, tag->text, '\n');
            return tag;
        }
    }
};

void convert(std::ifstream& i, std::ofstream& o) {
    try {
        // Validate streams
        if (!i || !o) {
            throw std::runtime_error("Invalid input/output stream");
        }

        MainTag maintag;
        maintag.input(i);
        o.clear();
        maintag.output(o);
        o.close();
        i.close();
    } catch (const std::exception& e) {
        std::cerr << "Conversion failed: " << e.what() << std::endl;
        throw;
    }
}
int main(int argc, char** argv){
    if(argc==1){
        std::cerr<<"Not enough arguments.\nUsage: sml <source file/dir> [result file/dir name]"<<std::endl;
        return 1;
    }
    
    std::string path(argv[1]);
    bool isfile = path.size()>=4&&path.substr(path.size() - 4) == ".sml";
    
    if(isfile)path = path.substr(0,path.size() - 4);

    std::string filepath(path+".sml");
    auto statdir = fs::status(path);
    auto dest = argc>2?argv[2]:path+"html";
    if (!isfile&&fs::is_directory(statdir)) {
        std::cout<<"Copying from dir "<<path<<" to dir "<<dest<<std::endl;
        if (fs::exists(dest)) {
            fs::remove_all(dest);
        }
    
        // Copy with directory structure preservation
        const auto copyOptions = fs::copy_options::recursive 
                               | fs::copy_options::directories_only;
        fs::copy(path, dest, copyOptions);
        for (const auto& dirEntry : fs::recursive_directory_iterator(path)) {
            if (dirEntry.is_regular_file()) {
                auto relPath = fs::relative(dirEntry.path(), path);
                auto targetPath = dest / relPath;
                fs::create_directories(targetPath.parent_path());
                fs::copy_file(dirEntry.path(), targetPath, fs::copy_options::overwrite_existing);
            }
        }
        std::vector<fs::path> toconvert;
        for (const auto& dirEntry : fs::recursive_directory_iterator(dest)) {
            if (dirEntry.is_regular_file() && dirEntry.path().extension() == ".sml") {
                toconvert.push_back(dirEntry.path());
            }
        }
        std::cout<<toconvert.size()<<" .sml files found"<<std::endl;
        for (const auto& file : toconvert) {
            try{
                auto inputfile = std::ifstream(file);
                if (!inputfile) {
                    std::cerr << "Failed to open input file: " << file << std::endl;
                    continue;
                }
                auto newpath = file;
                newpath.replace_extension(".html");
                auto outputfile = std::ofstream(newpath);
                if (!outputfile) {
                    std::cerr << "Failed to create output file: " << newpath << std::endl;
                    continue;
                }
                convert(inputfile, outputfile);
                std::cout << "Processed: " << file << " -> " << newpath << std::endl;
                
                fs::remove(file);
            } catch(const std::exception& e){
                std::cout<<e.what()<<std::endl;
            }
        }
    
        return 0;
    }
    auto statfile = fs::status(filepath);
    if(!fs::exists(statfile)){
        std::cout<<"File/directory does not exist";
        return 1;
    }
    auto inputfile = std::ifstream(filepath);
    auto outputfile = std::ofstream(path+".html");
    
    convert(inputfile,outputfile);
    std::cout << "Processed: " << filepath << " -> " << path+".html" << std::endl;
}
