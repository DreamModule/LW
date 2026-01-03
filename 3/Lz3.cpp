#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <array>
#include <string>
#include <functional>
#include <map>
#include <stdexcept>

namespace CodecEngine {

    using Byte = uint8_t;
    constexpr uint32_t BUFFER_SIZE = 64 * 1024;

    class CodecException : public std::runtime_error { using runtime_error::runtime_error; };
    class IntegrityException : public CodecException { using CodecException::CodecException; };

    class CRC32 {
        static std::array<uint32_t, 256> table;
        static bool initialized;
        static void init_table() {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t c = i;
                for (int j = 0; j < 8; j++) c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
                table[i] = c;
            }
            initialized = true;
        }
    public:
        static uint32_t calculate(const char* data, size_t length, uint32_t previous_crc = 0xFFFFFFFF) {
            if (!initialized) init_table();
            uint32_t crc = previous_crc;
            const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
            for (size_t i = 0; i < length; i++) {
                crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
            }
            return crc; // Note: usually finalized with ^ 0xFFFFFFFF, keep in stream context
        }
        static uint32_t finalize(uint32_t crc) { return crc ^ 0xFFFFFFFF; }
    };
    std::array<uint32_t, 256> CRC32::table;
    bool CRC32::initialized = false;

    struct HeaderV1 {
        static constexpr size_t FIXED_SIZE = 24;
        static constexpr std::array<Byte, 6> SIG = {0x4F, 0x54, 0x49, 0x4B, 0x56, 0x31};
        
        uint8_t ver_major = 1;
        uint8_t ver_minor = 0;
        uint16_t compression_id = 0;
        uint16_t protection_id = 0;
        uint64_t original_size = 0;
        uint32_t extra_size = 0;
    };

    class StreamIO {
    public:
        static void write_u16(std::ostream& os, uint16_t v) {
            Byte b[] = {static_cast<Byte>(v), static_cast<Byte>(v >> 8)};
            os.write(reinterpret_cast<char*>(b), 2);
        }
        static void write_u32(std::ostream& os, uint32_t v) {
            Byte b[4];
            for(int i=0;i<4;++i) b[i] = static_cast<Byte>(v >> (i*8));
            os.write(reinterpret_cast<char*>(b), 4);
        }
        static void write_u64(std::ostream& os, uint64_t v) {
            Byte b[8];
            for(int i=0;i<8;++i) b[i] = static_cast<Byte>(v >> (i*8));
            os.write(reinterpret_cast<char*>(b), 8);
        }
        static uint16_t read_u16(std::istream& is) {
            Byte b[2]; is.read(reinterpret_cast<char*>(b), 2);
            return b[0] | (b[1] << 8);
        }
        static uint32_t read_u32(std::istream& is) {
            Byte b[4]; is.read(reinterpret_cast<char*>(b), 4);
            uint32_t v = 0; for(int i=0;i<4;++i) v |= static_cast<uint32_t>(b[i]) << (i*8);
            return v;
        }
        static uint64_t read_u64(std::istream& is) {
            Byte b[8]; is.read(reinterpret_cast<char*>(b), 8);
            uint64_t v = 0; for(int i=0;i<8;++i) v |= static_cast<uint64_t>(b[i]) << (i*8);
            return v;
        }
    };

    class ArchiveProcessor {
    public:
        void compress(std::istream& in, std::ostream& out, uint64_t input_size) {
            out.write(reinterpret_cast<const char*>(HeaderV1::SIG.data()), 6);
            out.put(1); out.put(0); // Version 1.0
            StreamIO::write_u16(out, 0); // No compression
            StreamIO::write_u16(out, 0); // No protection
            StreamIO::write_u64(out, input_size);
            StreamIO::write_u32(out, 4); // Extra header for CRC

            uint32_t crc_accum = 0xFFFFFFFF;
            
            // Reserve space for CRC
            std::streampos crc_pos = out.tellp();
            StreamIO::write_u32(out, 0); 

            std::vector<char> buffer(BUFFER_SIZE);
            while (in) {
                in.read(buffer.data(), buffer.size());
                std::streamsize count = in.gcount();
                if (count > 0) {
                    out.write(buffer.data(), count);
                    crc_accum = CRC32::calculate(buffer.data(), count, crc_accum);
                }
            }

            uint32_t final_crc = CRC32::finalize(crc_accum);
            out.seekp(crc_pos);
            StreamIO::write_u32(out, final_crc);
            out.seekp(0, std::ios::end);
        }

        void decompress(std::istream& in, std::ostream& out) {
            std::array<Byte, 6> sig;
            if (!in.read(reinterpret_cast<char*>(sig.data()), 6) || sig != HeaderV1::SIG)
                throw CodecException("Invalid signature");

            uint8_t ver_maj = in.get();
            uint8_t ver_min = in.get(); 
            if (ver_maj < 1) throw CodecException("Unsupported version");

            StreamIO::read_u16(in); // Comp
            StreamIO::read_u16(in); // Prot
            uint64_t orig_size = StreamIO::read_u64(in);
            uint32_t extra_len = StreamIO::read_u32(in);

            uint32_t expected_crc = 0;
            bool has_crc = (extra_len >= 4);
            if (has_crc) {
                expected_crc = StreamIO::read_u32(in);
                if (extra_len > 4) in.seekg(extra_len - 4, std::ios::cur);
            } else {
                in.seekg(extra_len, std::ios::cur);
            }

            uint32_t crc_accum = 0xFFFFFFFF;
            std::vector<char> buffer(BUFFER_SIZE);
            uint64_t remaining = orig_size;

            while (remaining > 0 && in) {
                size_t to_read = (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE;
                in.read(buffer.data(), to_read);
                size_t count = in.gcount();
                
                if (has_crc) crc_accum = CRC32::calculate(buffer.data(), count, crc_accum);
                
                out.write(buffer.data(), count);
                remaining -= count;
            }

            if (has_crc) {
                if (CRC32::finalize(crc_accum) != expected_crc)
                    throw IntegrityException("CRC32 Mismatch: Data corrupted");
            }
        }
    };
}

class ArgParser {
    std::string command;
    std::string input;
    std::string output;
public:
    ArgParser(int argc, char** argv) {
        if (argc < 4) throw std::invalid_argument("Insufficient arguments");
        command = argv[1];
        input = argv[2];
        output = argv[3];
    }
    bool is_compress() const { return command == "-c" || command == "--compress"; }
    bool is_decompress() const { return command == "-d" || command == "--decompress"; }
    std::string get_input() const { return input; }
    std::string get_output() const { return output; }
};

int main(int argc, char* argv[]) {
    try {
        ArgParser args(argc, argv);
        CodecEngine::ArchiveProcessor processor;

        if (args.is_compress()) {
            std::ifstream in(args.get_input(), std::ios::binary);
            std::ofstream out(args.get_output(), std::ios::binary);
            if (!in || !out) throw std::runtime_error("IO Error opening files");
            
            auto size = std::filesystem::file_size(args.get_input());
            std::cout << "Compressing " << size << " bytes..." << std::endl;
            processor.compress(in, out, size);
            std::cout << "Done." << std::endl;
        } 
        else if (args.is_decompress()) {
            std::ifstream in(args.get_input(), std::ios::binary);
            std::ofstream out(args.get_output(), std::ios::binary);
            if (!in || !out) throw std::runtime_error("IO Error opening files");

            std::cout << "Decompressing..." << std::endl;
            processor.decompress(in, out);
            std::cout << "Done." << std::endl;
        } 
        else {
            std::cerr << "Usage: app [-c|-d] <in> <out>" << std::endl;
            return 1;
        }
    } 
    catch (const CodecEngine::IntegrityException& e) {
        std::cerr << "[CRITICAL] Data Integrity Error: " << e.what() << std::endl;
        return 2;
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
