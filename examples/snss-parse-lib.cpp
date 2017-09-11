#include "../snss-parse-lib.hpp"


int main() {
    std::ifstream f("/home/instance/.config/chromium/Default/Current Tabs", std::ios::binary | std::ios::in);

    int filesize = get_file_size(f);
    std::cout << "Filesize: " << filesize << std::endl;

    char magic[4];
    read_char(f, magic, 4);
    uint32_t version = read_arbitrary<uint32_t>(f, 4);

    if (0 != std::strcmp("SNSS", magic))
        return 1;
    std::cout << magic << " v" << version << std::endl;


    // -------

    while (f.tellg() < filesize) {
        std::shared_ptr<Command> c = read_next_tab_command(f);
        if (c->size == 0) {
            break;
        }
        std::cout << std::hex << "P: " << f.tellg() << std::dec << " S: " << c->size << " T: " << static_cast<uint16_t>(c->type) << std::endl;
    }

    //probe_next_command(f);
}

