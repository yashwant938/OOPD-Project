#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <functional>
#include <mutex>


using namespace std;
mutex coutMutex;

namespace fs = filesystem;

bool isHidden(const string& str) {
    return str.size() > 0 && str[0] == '.';
}
bool isDirectory(const fs::directory_entry& file) {
    return fs::is_directory(file.path());
}
class CdCommand{
public:
    void execute(const vector<string>& args){
        
        bool help = find(args.begin(), args.end(), "-h") != args.end();
        if(help){
            cout<< "Usage: cd <directory>" << endl;
            cout<< "directory: Directory name to traverse" << endl;
            return;
        }

        if (args.size() == 2) {
            if (fs::exists(args[1]) && fs::is_directory(args[1])) {
                fs::current_path(args[1]);
            } else {
                cerr << "Directory does not exist: " << args[1] << endl;
            }
        } else {
            cerr << "Usage: cd <directory>" << endl;
        }
    }
};

class LsCommand{
public:
    void execute(const vector<string>& args){
        bool recursive = find(args.begin(), args.end(), "-r") != args.end();
        bool detail = find(args.begin(), args.end(), "-l") != args.end();
        bool all = find(args.begin(), args.end(), "-a") != args.end();
        bool help = find(args.begin(), args.end(), "-h") != args.end();
        if(help){
            cout<< "Usage: ls [-r] [-a] [-l] [dir]" << endl;
            cout<< "-r: List Recursively" << endl;
            cout<< "-a: List all files, including hidden ones" << endl;
            cout<< "-l: Display detailed information about each file" << endl;
            cout<< "dir: If -l, give dir name" << endl;
            return;
        }
        
        if (args.size() <= 5) {
            fs::path destination;
            if(detail && fs::exists(args[2])){
                destination = fs::absolute(args[2]);
            }else{                
                destination = fs::current_path();
            }
            listDirectory(destination,recursive,all,detail,"-");
            cout<< endl;
        } else {
            cerr << "Usage: ls [-r]" << endl;
        }
    }
    void listDirectory(const fs::path& directory, bool recursive, bool all, bool detail, string depth){
        vector<fs::directory_entry> files;
        vector<thread> threads;
        std::unique_lock<std::mutex> lock(coutMutex);

        for (const auto& file : fs::directory_iterator(directory)) {
            files.push_back(file);
        }

        sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
            return isDirectory(a) > isDirectory(b);
        });

        for (const auto& file : files) {
            if (!isHidden(file.path().filename().string()) || all) {

                cout<< depth << file.path().filename().string();
                if(detail && !fs::is_directory(file.path())){
                    uintmax_t fileSize = fs::file_size(file.path());
                    double sizeInKB = static_cast<double>(fileSize) / 1024.0;
                    cout<<"\t"<<sizeInKB<<" KB";

                    fs::perms permissions = fs::status(file.path()).permissions();
                    int permissionsInt = static_cast<int>(permissions);
                    cout<< "\t" << oct << permissionsInt << dec;
                }

                cout<<endl;
                if (recursive && fs::is_directory(file.path())) {                    
                    threads.emplace_back(std::bind(&LsCommand::listDirectory, this, file.path(), recursive, all, detail, depth + "\t"));
                }
            }
        }
        lock.unlock();
        for (auto& thread : threads) {
            thread.join();
        }

    }
};



class MvCommand{
public:
    void execute(const vector<string>& args){
        
        bool help = find(args.begin(), args.end(), "-h") != args.end();
        bool force = find(args.begin(), args.end(), "-f") != args.end();
        if(help){
            cout<< "mv <source> <destination> [-f]" << endl;
            cout<< "source: Source file/directory to copy" << endl;
            cout<< "destination: Destination file/directory to paste" << endl;
            cout<< "-f: Do not prompt for confirmation before overwriting files" << endl;
            return;
        }

        if (args.size() <= 2) {
            cerr << "Usage: mv <source> <destination>" << endl;
            return;
        }
        

        auto source = fs::absolute(args[1]);
        auto destination = fs::absolute(args[2]);

        if (fs::exists(source)) {
            try {
                fs::path destinationFilePath = destination;
                if(!fs::exists(destinationFilePath)){
                    fs::create_directory(destinationFilePath);
                } 
                if(fs::is_directory(destinationFilePath)) {
                    destinationFilePath /= source.filename();    
                }
                if(fs::is_directory(source)) {
                    mvDir(source,destination,force);
                    fs::remove(source);
                }else{
                    fs::copy(source, destinationFilePath, fs::copy_options::overwrite_existing);
                    fs::remove(source);                   
                }

                cout<< "File/Directory moved successfully." << endl;
            
            } catch (const exception& e) {
                cerr << "Error copying file/directory: " << e.what() << endl;
            }
        } else {
            cerr << "Source file/directory does not exist." << endl;
        }
 
    }
    
    void mvDir(const fs::path& source,const fs::path& destination,bool force){

        vector<thread> threads;
        for (const auto& file : fs::directory_iterator(source)) {
            if (!isHidden(file.path().filename().string())) {
                if(!fs::exists(destination)){
                    fs::create_directory(destination);
                }
                fs::path destinationFilePath = destination;
                destinationFilePath /= file.path().filename();

                if (fs::is_directory(file.path())) {
                    threads.emplace_back(std::bind(&MvCommand::mvDir, this, file.path(), destinationFilePath,force));
                }else{
                    if(force && fs::exists(destinationFilePath)){
                        fs::remove(destinationFilePath);
                    }
                    fs::copy(file.path(), destinationFilePath, fs::copy_options::overwrite_existing);
                }
            }
            if(!fs::is_directory(file.path())){
                fs::remove(file.path());
            }
        }

        for (auto& thread : threads) {
            thread.join();
        }
        for (const auto& file : fs::directory_iterator(source)) {
            fs::remove(file.path());
        }

    }
};

class RmCommand{
public:
    void execute(const vector<string>& args){
        bool force = find(args.begin(), args.end(), "-f") != args.end();
        bool interactive = find(args.begin(), args.end(), "-i") != args.end();
        bool recursive = find(args.begin(), args.end(), "-r") != args.end();
        
        bool help = find(args.begin(), args.end(), "-h") != args.end();
        if(help){
            cout<< "Usage: rm [-f] [-i] [-r] <file/directory>" << endl;
            cout<< "-f: Ignore nonexistent files and do not prompt" << endl;
            cout<< "-i: Prompt before every removal" << endl;
            cout<< "-r: Remove directories and their contents recursively" << endl;
            return;
        }

        if (args.size() >= 2) {
            const fs::path target = args[1];

            if (fs::exists(target)) {
                try {
                    if (fs::is_directory(target)) {
                        rmDir(target,recursive,interactive);
                    }else{
                        fs::remove(target);
                    }
                    cout<< "File/Direcory removed successfully." << endl;
                } catch (const exception& e) {
                    cerr << "Error removing file/directory: " << e.what() << endl;
                }
            } else {
                if(!force){
                    cerr << "File/Directory does not exist." << endl;
                }
            }
        } else {
            cerr << "Usage: rm [-f] [-i] <file/directory>" << endl;
        }
    }
    void rmDir(const fs::path& path, bool recursive, bool interactive){
        vector<thread> threads;
        for (const auto& file : fs::directory_iterator(path)) {
            if (fs::is_directory(file.path()) && recursive) {                
                threads.emplace_back(std::bind(&RmCommand::rmDir, this, file.path(), recursive, interactive));
            }
            if(!fs::is_directory(file.path())){
                if(interactive){
                    cout<< "Do you want to delete "<<file.path().filename().string()<<"? (y/n): ";
                    char response;
                    cin >> response;
                    if (response != 'y' && response != 'Y') {
                        cout<<"Skipping... "<<file.path().filename().string()<<endl;
                        continue;
                    }
                }
                
                fs::remove(file.path());
            }
            
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        if (fs::exists(path) && recursive) {
            fs::remove(path);
        }
    }
};


class CpCommand{
public:
    void execute(const vector<string>& args){
        
        bool help = find(args.begin(), args.end(), "-h") != args.end();
        bool recursive = find(args.begin(), args.end(), "-r") != args.end();
        bool isPrompt = find(args.begin(), args.end(), "-i") != args.end();
        if(help){
            cout<< "Usage: cp <source> <destination> [-i] [-r]" << endl;
            cout<< "-i: Prompt before overwriting files" << endl;
            cout<< "-r: Copy directories recursively" << endl;
            return;
        }

        if (args.size() >= 3) {
            auto source = fs::absolute(args[1]);
            auto destination = fs::absolute(args[2]);

            if (fs::exists(source)) {
                try {
                    fs::path destinationFilePath = destination;
                    if(!fs::exists(destinationFilePath)){
                        fs::create_directory(destinationFilePath);
                    }
                    if(fs::is_directory(destinationFilePath)) {
                        destinationFilePath /= source.filename();    
                    }
                    if(fs::is_directory(source)) {
                        cpDir(source,destinationFilePath,recursive,isPrompt);
                    }else{
                        
                        fs::copy(source, destinationFilePath, fs::copy_options::overwrite_existing);                        
                    }

                    cout<< "File/Directory copied successfully." << endl;
                } catch (const exception& e) {
                    cerr << "Error copying file/directory: " << e.what() << endl;
                }
            } else {
                cerr << "Source file/directory does not exist." << endl;
            }
        } else {
            cerr << "Usage: cp <source> <destination>" << endl;
        }
    }
    void cpDir(const fs::path& source,const fs::path& destination, bool recursive, bool isPrompt){

        vector<thread> threads;
        for (const auto& file : fs::directory_iterator(source)) {
            if (!isHidden(file.path().filename().string())) {
                if(!fs::exists(destination)){
                    fs::create_directory(destination);
                }
                fs::path destinationFilePath = destination;
                destinationFilePath /= file.path().filename();

                if(fs::is_directory(file.path())){
                    if(recursive){
                        threads.emplace_back(std::bind(&CpCommand::cpDir, this, file.path(), destinationFilePath,recursive,isPrompt));
                    }
                }else{
                    if(fs::exists(destinationFilePath) && isPrompt){
                        cout<<endl<<file.path().filename().string()<<" Exist in "<<destination<<endl;
                        cout<< "Do you want to overwrite the existing file/directory? (y/n): ";
                        char response;
                        cin >> response;
                        if (response != 'y' && response != 'Y') {
                            cout<<"Skipping... "<<file.path().filename().string()<<endl;
                            continue;
                        }
                    }
                    if(fs::exists(destinationFilePath)){
                        fs::remove(destinationFilePath);
                    }
                    fs::copy(file.path(), destinationFilePath, fs::copy_options::overwrite_existing);
                }
            }
            
        }        
        for (auto& thread : threads) {
            thread.join();
        }

    }
};

class CstmShell {
public:
    void run(bool isArgCli, char *argv[]) {
        while (true) {
            string input;
            vector<string> args;
            size_t pos = 0;
            if(isArgCli){
                for (char **arg = argv; *arg != nullptr; ++arg) {
                    args.push_back(*arg);
                }
            }else{
                cout<< ">";
                getline(cin, input);

                while ((pos = input.find(' ')) != string::npos) {

                    args.push_back(input.substr(0, pos));
                    input.erase(0, pos + 1);
                }
                args.push_back(input);
            }
            // Execute command based on the first argument
            if (args.empty()) {
                continue;
            } else if (args[0] == "cd") {
                CdCommand cd;
                cd.execute(args);
            } else if (args[0] == "ls") {
                LsCommand ls;
                ls.execute(args);
            } else if (args[0] == "mv") {
                MvCommand mv;
                mv.execute(args);
            } else if (args[0] == "rm") {
                RmCommand rm;
                rm.execute(args);
            } else if (args[0] == "cp") {
                CpCommand cp;
                cp.execute(args);
            } else if (args[0] == "exit") {
                break;
            } else {
                cerr << "Unknown command: " << args[0] << endl;
            }
            if(isArgCli){
                break;
            }
        }
    }
};

int main(int argc, char *argv[]) {
    CstmShell cstmShellObj;
    bool isArgCli=false;
    if(argc==1){
        cstmShellObj.run(isArgCli,argv);
    }else{
        isArgCli=true;
        ++argv;
        --argc;
        cstmShellObj.run(isArgCli,argv);
    }
    return 0;
}