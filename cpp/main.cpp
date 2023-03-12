#include <iostream>
#include <fstream>
#include <filesystem>
#include <bitset>
#include <vector>
#include <cstdint>

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using i8  = std::int8_t;
using i16 = std::int16_t;

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

const std::vector<std::string> EFFECTIVE_ADDRESS_CALCULATION_TABLE = {
        "bx + si",
        "bx + di",
        "bp + si",
        "bp + di",
        "si",
        "di",
        "bp",
        "bx"};

std::string output = "bits 16\n\n";

bool parseByte(unsigned char* &runner) {
    unsigned char byte = *runner;
    unsigned char instruction;

    // handle possible 4-bit witdth
    instruction = byte >> 4;
    switch (instruction) {
        // mov (immediate to register)
        case 0b1011: {
            output += "mov ";

            unsigned char word_field = (byte & 0b1000) >> 3;
            unsigned char reg_field = byte & 0b1111;

            output += REGISTER_TABLE[reg_field] + ", ";

            byte = *(++runner);

            i16 data = reinterpret_cast<char&>(byte);

            if (word_field) {
                data = *reinterpret_cast<i16*>(runner);
                ++runner;
            }

            output += std::to_string(data) + "\n";

            return true;
        }
    }


    // handle possible 6-bit witdth
    instruction = byte >> 2;
    switch (instruction) {
        // mov (register/memory to/from register)
        case 0b100010: {
            output += "mov ";

            unsigned char direction_field = (byte & 0b10) >> 1;
            unsigned char word_field = byte & 0b1;

            // TODO: look ahead if we have enough bytes left
            ++runner;

            unsigned char operands = *runner;
            unsigned char mode_field = operands >> 6;
            unsigned char reg_field = (operands & 0b111000) >> 3;
            unsigned char rm_field = operands & 0b111;

            unsigned char reg_one_index = (word_field << 3) | reg_field;
            std::string register_one = REGISTER_TABLE[reg_one_index];

            std::string destination;
            std::string source;
            switch (mode_field) {
                // memory-mode, no displacement (except for r/m = 110)
                case 0b0: {
                    if (rm_field == 0b110) {
                        // direct access

                        i16 data = *reinterpret_cast<i16*>(++runner);
                        ++runner;

                        source = register_one;
                        destination = "[" + std::to_string(data) + "]";
                    } else {
                        source = register_one;
                        destination = "[" + EFFECTIVE_ADDRESS_CALCULATION_TABLE[rm_field] + "]";
                    }

                    break;
                }
                // memory-mode, 8-bit displacement
                case 0b1: {
                    source = register_one;
                    i8 data = *reinterpret_cast<i8*>(++runner);
                    destination = "[" + EFFECTIVE_ADDRESS_CALCULATION_TABLE[rm_field];
                    if (data > 0) {
                        destination += " + " + std::to_string(data);
                    }
                    destination += "]";

                    break;
                }
                // memory-mode, 16-bit displacement
                case 0b10: {
                    source = register_one;
                    i16 data = *reinterpret_cast<i16*>(++runner);
                    ++runner;
                    destination = "[" + EFFECTIVE_ADDRESS_CALCULATION_TABLE[rm_field];
                    if (data > 0) {
                        destination += " + " + std::to_string(data);
                    }
                    destination += "]";

                    break;
                }
                // register-to-register mode
                case 0b11: {
                    std::string register_two = REGISTER_TABLE[(word_field << 3) | rm_field];

                    source = register_one;
                    destination = register_two;

                    break;
                }

                default: {
                    std::cout << "Encountered unhandled mode: " << std::bitset<8>(mode_field) << std::endl;
                    return false;
                }
            }

            if (direction_field) {
                std::string tmp = source;
                source = destination;
                destination = tmp;
            }
            output += destination + ", " + source + "\n";
            return true;
        }

        default: {
            std::cout << "Encountered unhandled instruction: " << std::bitset<8>(instruction) << std::endl;
            return false;
        }
    }

    return false;
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
        std::cout << output << std::endl;
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
