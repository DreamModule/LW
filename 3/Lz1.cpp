#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <memory>
#include <array>
#include <algorithm>

namespace Core {

    using Byte = uint8_t;
    constexpr std::size_t BUFFER_SIZE = 16 * 1024;
    
    struct Signature {
        static constexpr std::size_t SIZE = 6;
        static constexpr std::array<Byte, SIZE> VALUE = {0x4F, 0x54, 0x49, 0x4B, 0x56, 0x30};
    };

    class IOException : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class InvalidFormatException : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class BinaryStreamUtils {
    public:
        static void write_u16_le(std::ostream& out, uint16_t value) {
            Byte buf[2];
            buf[0] = static_cast<Byte>(value & 0xFF);
            buf[1] = static_cast<Byte>((value >> 8) & 0xFF);
            out.write(reinterpret_cast<char*>(buf), 2);
        }

        static void write_u64_le(std::ostream& out, uint64_t value) {
            Byte buf[8];
            for (int i = 0; i < 8; ++i) {
                buf[i] = static_cast<Byte>((value >> (i * 8)) & 0xFF);
            }
            out.write(reinterpret_cast<char*>(buf), 8);
        }

        static uint16_t read_u16_le(std::istream& in) {
            Byte buf[2];
            if (!in.read(reinterpret_cast<char*>(buf), 2)) throw IOException("Unexpected EOF reading uint16");
            return static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8);
        }

        static uint64_t read_u64_le(std::istream& in) {
            Byte buf[8];
            if (!in.read(reinterpret_cast<char*>(buf), 8)) throw IOException("Unexpected EOF reading uint64");
            uint64_t val = 0;
            for (int i = 0; i < 8; ++i) {
                val |= static_cast<uint64_t>(buf[i]) << (i * 8);
            }
            return val;
        }
    };

    class CodecV0 {
    public:
        void encode(std::istream& source, std::ostream& dest, uint64_t source_size) {
            dest.write(reinterpret_cast<const char*>(Signature::VALUE.data()), Signature::SIZE);
            BinaryStreamUtils::write_u16_le(dest, 0);
            BinaryStreamUtils::write_u64_le(dest, source_size);

            std::vector<char> buffer(BUFFER_SIZE);
            while (source) {
                source.read(buffer.data(), buffer.size());
                dest.write(buffer.data(), source.gcount());
            }
        }

        void decode(std::istream& source, std::ostream& dest) {
            std::array<Byte, Signature::SIZE> sig_buffer;
            if (!source.read(reinterpret_cast<char*>(sig_buffer.data()), Signature::SIZE)) {
                throw IOException("File too short for signature");
            }

            if (sig_buffer != Signature::VALUE) {
                throw InvalidFormatException("Invalid file signature");
            }

            uint16_t version = BinaryStreamUtils::read_u16_le(source);
            if (version != 0) {
                throw InvalidFormatException("Unsupported format version: " + std::to_string(version));
            }

            uint64_t original_size = BinaryStreamUtils::read_u64_le(source);
            uint64_t processed = 0;

            std::vector<char> buffer(BUFFER_SIZE);
            while (processed < original_size && source) {
                std::streamsize chunk = static_cast<std::streamsize>(std::min(static_cast<uint64_t>(BUFFER_SIZE), original_size - processed));
                if (!source.read(buffer.data(), chunk)) break;
                dest.write(buffer.data(), source.gcount());
                processed += source.gcount();
            }

            if (processed != original_size) {
                std::cerr << "Warning: Data stream ended prematurely. Expected: " << original_size << ", Got: " << processed << std::endl;
            }
        }
    };
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 4) {
            std::cerr << "Usage: " << argv[0] << " <-c|-d> <input> <output>" << std::endl;
            return 1;
        }

        std::string mode = argv[1];
        std::filesystem::path input_path = argv[2];
        std::filesystem::path output_path = argv[3];

        Core::CodecV0 codec;

        if (mode == "-c") {
            auto file_size = std::filesystem::file_size(input_path);
            std::ifstream in(input_path, std::ios::binary);
            std::ofstream out(output_path, std::ios::binary);
            if (!in || !out) throw Core::IOException("Failed to open streams");
            codec.encode(in, out, file_size);
        } 
        else if (mode == "-d") {
            std::ifstream in(input_path, std::ios::binary);
            std::ofstream out(output_path, std::ios::binary);
            if (!in || !out) throw Core::IOException("Failed to open streams");
            codec.decode(in, out);
        } 
        else {
            throw std::invalid_argument("Unknown mode");
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
