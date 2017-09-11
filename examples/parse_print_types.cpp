#include "../snss-parse-lib.hpp"


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
        std::cout << "For example on ArchLinux you could try and parse ";
        std::cout << "~/.config/chromium/Default/Current Session" << std::endl;
    }

    std::ifstream f(argv[1], std::ios::binary | std::ios::in);
    if (!f.is_open() || f.fail())
        return 1;

    std::vector<std::shared_ptr<Command>> cmds = read_session_file(f);

    for (auto & x : cmds) {
        std::cout << " Size: " << x->size << " Type: " << static_cast<uint16_t>(x->type) << std::endl;
    }

    f.close();
}

