#include "../snss-parse-lib.hpp"


// This will print all urls to stdout and can be used to back up all open tabs (with their history).
// Example usage: ./parse-all-urls "Current Session" > tabs.txt

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
        std::cout << "For example on ArchLinux you could try and parse ";
        std::cout << "~/.config/chromium/Default/Current Session" << std::endl;
        return 0;
    }

    std::vector<std::shared_ptr<Command>> cmds = read_session_file(std::string(argv[1]));

    for (auto & x : cmds) {
        if (x->type == 6) {
            std::shared_ptr<CommandUpdateTabNavigation> c = std::static_pointer_cast<CommandUpdateTabNavigation>(x);
            std::cout << c->tabid << " " << c->index << " " << c->url << std::endl;
        }
    }
}

