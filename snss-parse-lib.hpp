#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <stdint.h>
#include <vector>
#include <memory>


// TODO Documentation

// ---------------- //
// Session commands //
// ---------------- //

struct Command {
    uint16_t size;
    uint32_t type;
    virtual ~Command() {}
};

struct CommandSetWindowBounds3 : Command {
    int32_t window_id;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t show_state; // https://0x1.pl/s/36
    virtual ~CommandSetWindowBounds3() {}
};

struct CommandLastActiveTime : Command {
    int32_t id;
    int32_t UNKNOWN_0; // TODO
    int64_t last_active_time;
    virtual ~CommandLastActiveTime() {}
};

struct CommandTabClosed : Command {
    int32_t id;
    int32_t UNKNOWN_0; // TODO
    int64_t close_time;
    virtual ~CommandTabClosed() {}
};

using CommandWindowClosed = CommandTabClosed;

struct CommandSetActiveWindow : Command {
    uint32_t window_id;
    virtual ~CommandSetActiveWindow() {}
};

struct IDAndIndex : Command {
    uint32_t id;
    int32_t index;
    virtual ~IDAndIndex() {}
};

struct CommandUpdateTabNavigation : Command {
    uint32_t pickle_size;
    int32_t tabid;
    int32_t index;
    std::string url; // int + string
    std::wstring title; // int + wstring
    // TODO Add all the other fields that are missing
    // See serialized_navigation_entry.cc => WriteToPickle()
    virtual ~CommandUpdateTabNavigation() {}
};

struct CommandSessionStorageAssociated : Command {
    uint32_t pickle_size;
    int32_t tabid;
    std::string session_storage_persistent_id; // int + string
    virtual ~CommandSessionStorageAssociated() {}
};

struct CommandSetWindowWorkspace2 : Command {
    uint32_t pickle_size;
    int32_t window_id;
    std::string workspace; // int + string
    virtual ~CommandSetWindowWorkspace2() {}
};


// ---------------- //
// - Tab commands - //
// ---------------- //

// TODO Old struct didn't include timestamp https://0x1.pl/s/41
struct CommandSelectedNavigationInTab : Command {
    int32_t tabid;
    int32_t index;
    int64_t timestamp;
    virtual ~CommandSelectedNavigationInTab() {}
};

struct CommandRestoredEntry : Command {
    int32_t entry_id;
    virtual ~CommandRestoredEntry() {}
};

struct CommandPinnedState : Command {
    bool pinned;
    virtual ~CommandPinnedState() {}
};

// ---------------- //
//  Read utilities  //
// ---------------- //

template <typename T>
T read_arbitrary(std::ifstream& f, int size, int offset) {
    T ret;
    f.seekg(offset);
    f.read(reinterpret_cast<char *>(&ret), size);
    return ret;
}

template <typename T>
T read_arbitrary(std::ifstream& f, int size) {
    T ret;
    f.read(reinterpret_cast<char *>(&ret), size);
    return ret;
}

void read_char(std::ifstream& f, char* out, int size, int offset) {
    f.seekg(offset);
    f.read(out, size);
}

void read_char(std::ifstream& f, char* out, int size) {
    f.read(out, size);
}

bool read_string(std::ifstream& f, std::string& ret, bool terminate) {
    uint32_t len = read_arbitrary<uint32_t>(f, 4);
    int start = f.tellg();
    int end = start + len;
    ret.resize(end - start);
    f.read(&ret[0], end - start);
    if (terminate) {
        uint16_t z = read_arbitrary<uint16_t>(f, 1);
        // For whatever reason the string is padded with an arbitrary amount of zeroes
        if (0 != z)
            return false; // If there is no zero, a string16 won't follow
        int count = 0;
        while (0 == z) {
            count += 1;
            z = read_arbitrary<uint16_t>(f, 1);
        }
        if (count > 3)
            return false;
        f.seekg(f.tellg() - 1L);

    }
    return true;
}

std::wstring read_string16(std::ifstream& f, bool terminate) {
    uint32_t len = read_arbitrary<uint32_t>(f, 4) * 2; // *2 because UTF-16
#ifdef DEBUG
    std::cout << "Reading string16: " << std::endl;
    std::cout << std::dec << len << std::endl;
    std::cout << std::hex << f.tellg() << std::endl;
    std::cout << std::dec;
#endif
    char ret[len];
    read_char(f, ret, len);
    if (terminate) {
        read_arbitrary<uint16_t>(f, 1); // String is null terminated with 0x00
    }
    return std::wstring (&ret[0], &ret[len]);
}

void probe_next_command(std::ifstream& f) {
    uint16_t size = read_arbitrary<uint16_t>(f, 2);
    uint8_t type_id = read_arbitrary<uint8_t>(f, 1);
    std::cout << "S: " << size << " T: " << static_cast<uint16_t>(type_id) << std::endl;
}

// ---------------- //
//  Read functions  //
// ---------------- //

// TODO Missing ids: 11,13,18
std::shared_ptr<Command> read_next_session_command(std::ifstream& f) {
    std::shared_ptr<Command> ret;
    int prev = f.tellg();
    uint16_t size = read_arbitrary<uint16_t>(f, 2);
    uint8_t type_id = read_arbitrary<uint8_t>(f, 1);

    if (0 == size) {
        std::cerr << "Error: Encountered command with size 0. This can indicate a corrupt file.";
    }

    // See https://0x1.pl/s/33 for all command ids
    switch(type_id) {
        case 6: // CommandUpdateTabNavigation
        {
            ret = std::shared_ptr<CommandUpdateTabNavigation>(new CommandUpdateTabNavigation());
            std::shared_ptr<CommandUpdateTabNavigation> ret2 = std::static_pointer_cast<CommandUpdateTabNavigation>(ret);
            ret2->pickle_size = read_arbitrary<uint32_t>(f, 4);
            ret2->tabid = read_arbitrary<int32_t>(f, 4);
            ret2->index = read_arbitrary<int32_t>(f, 4);
            if (read_string(f, ret2->url, true))
                ret2->title = read_string16(f, true);
            // TODO Add missing stuff
            break;
        }
        case 0: // CommandSetTabWindow
        case 2: // CommandSetTabIndexInWindow
        case 5: // CommandTabNavigationPathPrunedFromBack
        case 7: // CommandSetSelectedNavigationIndex
        case 8: // CommandSetSelectedTabInIndex
        case 9: // CommandSetWindowType
        case 12: // CommandSetPinnedState (TODO works, but we should read a bool here, not int!)
        {
            ret = std::shared_ptr<IDAndIndex>(new IDAndIndex());
            std::shared_ptr<IDAndIndex> ret2 = std::static_pointer_cast<IDAndIndex>(ret);
            //IDAndIndex & cmd = (IDAndIndex &) ret;
            ret2->id = read_arbitrary<uint32_t>(f, 4);
            ret2->index = read_arbitrary<int32_t>(f, 4);
            break;
        }
        case 14: // CommandSetWindowBounds3
        {
            ret = std::shared_ptr<CommandSetWindowBounds3>(new CommandSetWindowBounds3());
            std::shared_ptr<CommandSetWindowBounds3> ret2 = std::static_pointer_cast<CommandSetWindowBounds3>(ret);
            ret2->window_id = read_arbitrary<int32_t>(f, 4);
            ret2->x = read_arbitrary<int32_t>(f, 4);
            ret2->y = read_arbitrary<int32_t>(f, 4);
            ret2->width = read_arbitrary<int32_t>(f, 4);
            ret2->height = read_arbitrary<int32_t>(f, 4);
            ret2->show_state = read_arbitrary<int32_t>(f, 4);
            break;
        }
        case 16: // CommandTabClosed
        {
            ret = std::shared_ptr<CommandTabClosed>(new CommandTabClosed());
            std::shared_ptr<CommandTabClosed> ret2 = std::static_pointer_cast<CommandTabClosed>(ret);
            ret2->id = read_arbitrary<int32_t>(f, 4);
            ret2->UNKNOWN_0 = read_arbitrary<int32_t>(f, 4); // TODO
            ret2->close_time = read_arbitrary<int64_t>(f, 8);
            break;
        }
        case 17: // CommandWindowClosed
        {
            ret = std::shared_ptr<CommandWindowClosed>(new CommandWindowClosed());
            std::shared_ptr<CommandWindowClosed> ret2 = std::static_pointer_cast<CommandWindowClosed>(ret);
            ret2->id = read_arbitrary<int32_t>(f, 4);
            ret2->UNKNOWN_0 = read_arbitrary<int32_t>(f, 4); // TODO
            ret2->close_time = read_arbitrary<int64_t>(f, 8);
            break;
        }
        case 19: // CommandSessionStorageAssociated
        {
            ret = std::shared_ptr<CommandSessionStorageAssociated>(new CommandSessionStorageAssociated());
            std::shared_ptr<CommandSessionStorageAssociated> ret2 = std::static_pointer_cast<CommandSessionStorageAssociated>(ret);
            ret2->pickle_size = read_arbitrary<uint32_t>(f, 4);
            ret2->tabid = read_arbitrary<int32_t>(f, 4);
            read_string(f, ret2->session_storage_persistent_id, false);
            break;
        }
        case 20: // CommandSetActiveWindow
        {
            ret = std::shared_ptr<CommandSetActiveWindow>(new CommandSetActiveWindow());
            std::shared_ptr<CommandSetActiveWindow> ret2 = std::static_pointer_cast<CommandSetActiveWindow>(ret);
            ret2->window_id = read_arbitrary<uint32_t>(f, 4);
            break;
        }
        case 21: // CommandLastActiveTime
        {
            ret = std::shared_ptr<CommandLastActiveTime>(new CommandLastActiveTime());
            std::shared_ptr<CommandLastActiveTime> ret2 = std::static_pointer_cast<CommandLastActiveTime>(ret);
            ret2->id = read_arbitrary<int32_t>(f, 4);
            ret2->UNKNOWN_0 = read_arbitrary<int32_t>(f, 4); // TODO
            ret2->last_active_time = read_arbitrary<int64_t>(f, 8);
            break;
        }
        case 23: // CommandSetWindowWorkspace2
        {
            ret = std::shared_ptr<CommandSetWindowWorkspace2>(new CommandSetWindowWorkspace2());
            std::shared_ptr<CommandSetWindowWorkspace2> ret2 = std::static_pointer_cast<CommandSetWindowWorkspace2>(ret);
            ret2->pickle_size = read_arbitrary<uint32_t>(f, 4);
            ret2->window_id = read_arbitrary<int32_t>(f, 4);
            read_string(f, ret2->workspace, false);
            break;
        }
        default:
        {
            std::cerr << "Warning: Encountered unsupported id " << static_cast<uint16_t>(type_id) << std::endl;
            std::cerr << std::hex << "P: " << prev << std::dec << " S: " << size << " T: " << static_cast<uint16_t>(type_id) << std::endl;
            ret = std::shared_ptr<Command>(new Command());
        }
    }
    ret->size = size;
    ret->type = type_id;

    // Finish reading the command in case we skipped stuff
    // Size already includes the 2 bytes from the uint16 size
    prev += size + 2L;
    if (f.tellg() < prev) {
        f.seekg(prev);
    }
    return ret;
}

// TODO Missing ids: 3,6,7,8,9
std::shared_ptr<Command> read_next_tab_command(std::ifstream& f) {
    std::shared_ptr<Command> ret;
    int prev = f.tellg();
    uint16_t size = read_arbitrary<uint16_t>(f, 2);
    uint8_t type_id = read_arbitrary<uint8_t>(f, 1);

    if (0 == size) {
        std::cerr << "Error: Encountered command with size 0. This can indicate a corrupt file.";
    }

    switch(type_id) {
        case 1: // CommandUpdateTabNavigation
        {
            // TODO Copied from read_next_session_command
            ret = std::shared_ptr<CommandUpdateTabNavigation>(new CommandUpdateTabNavigation());
            std::shared_ptr<CommandUpdateTabNavigation> ret2 = std::static_pointer_cast<CommandUpdateTabNavigation>(ret);
            ret2->pickle_size = read_arbitrary<uint32_t>(f, 4);
            ret2->tabid = read_arbitrary<int32_t>(f, 4);
            ret2->index = read_arbitrary<int32_t>(f, 4);
            if (read_string(f, ret2->url, true))
                ret2->title = read_string16(f, true);
            // TODO Add missing stuff
            break;
        }
        case 2: // CommandRestoredEntry
        {
            ret = std::shared_ptr<CommandRestoredEntry>(new CommandRestoredEntry());
            std::shared_ptr<CommandRestoredEntry> ret2 = std::static_pointer_cast<CommandRestoredEntry>(ret);
            ret2->entry_id = read_arbitrary<int32_t>(f, 4);
            break;
        }
        case 4: // CommandSelectedNavigationInTab
        {
            ret = std::shared_ptr<CommandSelectedNavigationInTab>(new CommandSelectedNavigationInTab());
            std::shared_ptr<CommandSelectedNavigationInTab> ret2 = std::static_pointer_cast<CommandSelectedNavigationInTab>(ret);
            ret2->tabid = read_arbitrary<int32_t>(f, 4);
            ret2->index = read_arbitrary<int32_t>(f, 4);
            ret2->timestamp = read_arbitrary<int64_t>(f, 8); // TODO First probe for next command
            break;
        }
        case 5: // CommandPinnedState
        {
            std::cout << std::hex << prev << std::dec << size << " " << std::endl;
            ret = std::shared_ptr<CommandPinnedState>(new CommandPinnedState());
            std::shared_ptr<CommandPinnedState> ret2 = std::static_pointer_cast<CommandPinnedState>(ret);
            ret2->pinned = read_arbitrary<bool>(f, 1);
            break;
        }
        default:
        {
            std::cerr << "Warning: Encountered unsupported id " << static_cast<uint16_t>(type_id) << std::endl;
            std::cerr << std::hex << "P: " << prev << std::dec << " S: " << size << " T: " << static_cast<uint16_t>(type_id) << std::endl;
            ret = std::shared_ptr<Command>(new Command());
        }
    }

    ret->size = size;
    ret->type = type_id;

    // Finish reading the command in case we skipped stuff
    prev += size + 2L;
    if (f.tellg() < prev) {
        f.seekg(prev);
    }
    return ret;
}

int get_file_size(std::ifstream& f) {
    f.seekg(0, std::ios::end);
    int filesize = f.tellg();
    f.seekg(0);
    return filesize;
}

bool open_and_check_file(std::ifstream& f, int *filesize) {
    *filesize = get_file_size(f);

    char magic[4];
    read_char(f, magic, 4);
    read_arbitrary<uint32_t>(f, 4); // version

    if (0 != std::strcmp("SNSS", magic)) {
        std::cerr << "Error: Not a valid SNSS file." << std::endl;
        return false;
    }

    return true;
}

std::vector<std::shared_ptr<Command>> read_session_file(std::ifstream& f) {
    int filesize;
    if (!open_and_check_file(f, &filesize))
        return {};

    std::vector<std::shared_ptr<Command>> ret;

    while (f.tellg() < filesize) {
        std::shared_ptr<Command> c = read_next_session_command(f);
        ret.push_back(c);

        if (c->size == 0) {
            break;
        }
    }

    return ret;
}

std::vector<std::shared_ptr<Command>> read_session_file(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary | std::ios::in);
    if (!f.is_open() || f.fail()) {
        std::cerr << "Error while trying to open file " << filename << "." << std::endl;
        return {};
    }

    std::vector<std::shared_ptr<Command>> ret;
    ret = read_session_file(f);
    f.close();
    return ret;
}

std::vector<std::shared_ptr<Command>> read_tabs_file(std::ifstream& f) {
    int filesize;
    if (!open_and_check_file(f, &filesize))
        return {};

    std::vector<std::shared_ptr<Command>> ret;

    while (f.tellg() < filesize) {
        std::shared_ptr<Command> c = read_next_tab_command(f);
        ret.push_back(c);

        if (c->size == 0) {
            break;
        }
    }

    return ret;
}

std::vector<std::shared_ptr<Command>> read_tabs_file(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary | std::ios::in);
    if (!f.is_open() || f.fail()) {
        std::cerr << "Error while trying to open file " << filename << "." << std::endl;
        return {};
    }

    std::vector<std::shared_ptr<Command>> ret;
    ret = read_tabs_file(f);
    f.close();
    return ret;
}

