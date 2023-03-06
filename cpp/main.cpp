#include <iostream>
#include <fstream>
#include <filesystem>
#include <bitset>
#include <vector>

void printHelp() {
    std::cout << "USAGE:\n\n";
    std::cout << "sim8086 path/to/binary\n";
}

const std::vector<std::string> REGISTER_TABLE = {
        "al",
        "cl",
        "dl",
        "bl",
        "ah",
        "ch",
        "dh",
        "bh",
        "ax",
        "cx",
        "dx",
        "bx",
        "sp",
        "bp",
        "si",
        "di"};

std::string output = "bits 16\n\n";

bool parseByte(unsigned char* &runner) {
    unsigned char byte = *runner;
    unsigned char instruction = byte >> 2;
    unsigned char direction_field = (byte & 0b10) >> 1;
    unsigned char word_field = byte & 0b1;

    switch (instruction) {
        case 0b100010: { // mov
            output += "mov ";

            // TODO: look ahead if we have enough bytes left
            ++runner;

            unsigned char operands = *runner;
            unsigned char mode_field = operands >> 6;
            unsigned char reg_field = (operands & 0b111000) >> 3;
            unsigned char rm_field = operands & 0b111;

            unsigned char reg_one_index = (word_field << 3) | reg_field;
            std::string register_one = REGISTER_TABLE[reg_one_index];

            switch (mode_field) {
                // register-to-register mode
                case 0b11: {
                    std::string register_two = REGISTER_TABLE[(word_field << 3) | rm_field];

                    std::string destination;
                    std::string source;
                    if (direction_field) {
                        destination = register_one;
                        source = register_two;
                    } else {
                        destination = register_two;
                        source = register_one;
                    }

                    output += destination + ", " + source + "\n";

                    break;
                }

                default: {
                    std::cout << "Encountered unhandled mode: " << std::bitset<8>(mode_field) << std::endl;
                    return false;
                }
            }
            break;
        }

        default: {
            std::cout << "Encountered unhandled instruction: " << std::bitset<8>(instruction) << std::endl;
            return false;
        }
    }

    return true;
}

bool parseData(const unsigned char* data, unsigned int size) {
    unsigned char* head = const_cast<unsigned char*>(data);
    while (head < data + size) {
        bool success = parseByte(head);
        if (!success) return success;
        ++head;
    }

    return true;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printHelp();
        return 1;
    }

    std::string filename(argv[1]);
    std::ifstream input_file;

    input_file.open(filename, std::ios_base::in | std::ios::binary);
    if (!input_file.good()) {
        std::cout << "Could not open file " << filename << std::endl;
        return 1;
    }

    auto file_size = std::filesystem::file_size(filename);
    char* data = new char[file_size];
    input_file.read(data, file_size);

    bool success = parseData(reinterpret_cast<unsigned char*>(data), file_size);

    delete[] data;

    if (!success) {
        std::cout << "Exiting with error\n";
        return 1;
    }

    std::ofstream output_file;
    output_file.open(filename + "_decompiled.asm", std::ios_base::out);
    if (output_file.is_open()) {
        output_file << output;
    } else {
        std::cout << "Could not open file " << filename << "_decompiled.asm" << std::endl;
        return 1;
    }

    std::cout << "Done!\n";
    return 0;
}
