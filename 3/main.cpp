#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <memory>

constexpr char SIGNATURE[6] = {'O', 'T', 'I', 'K', '8', '6'};
constexpr uint16_t FORMAT_VERSION = 1;
constexpr uint8_t NO_COMPRESSION = 0;
constexpr uint8_t NO_ERROR_CORRECTION = 0;
constexpr size_t BUFFER_SIZE = 65536;

#pragma pack(push, 1)
struct ArchiveHeader {
    char signature[6];
    uint16_t version;
    uint64_t original_size;
};
#pragma pack(pop)

class ArchiveProcessor {
public:
    static void encode(const std::string& input_path, const std::string& output_path) {
        std::ifstream input(input_path, std::ios::binary | std::ios::ate);
        if (!input.is_open()) {
            throw std::runtime_error("Failed to open input file: " + input_path);
        }

        const uint64_t file_size = input.tellg();
        input.seekg(0, std::ios::beg);

        std::ofstream output(output_path, std::ios::binary);
        if (!output.is_open()) {
            throw std::runtime_error("Failed to create output file: " + output_path);
        }

        write_header(output, file_size);
        process_data(input, output, file_size, true);

        input.close();
        output.close();
    }

    static void decode(const std::string& archive_path, const std::string& output_path) {
        std::ifstream archive(archive_path, std::ios::binary);
        if (!archive.is_open()) {
            throw std::runtime_error("Failed to open archive file: " + archive_path);
        }

        const uint64_t original_size = read_and_validate_header(archive);

        std::ofstream output(output_path, std::ios::binary);
        if (!output.is_open()) {
            throw std::runtime_error("Failed to create output file: " + output_path);
        }

        process_data(archive, output, original_size, false);

        archive.close();
        output.close();
    }

private:
    static void write_header(std::ofstream& output, uint64_t original_size) {
        ArchiveHeader header;
        std::memcpy(header.signature, SIGNATURE, 6);
        header.version = FORMAT_VERSION;
        header.original_size = original_size;

        output.write(reinterpret_cast<const char*>(&header), sizeof(header));
        
        const uint8_t compression_code = NO_COMPRESSION;
        const uint8_t error_correction_code = NO_ERROR_CORRECTION;
        output.write(reinterpret_cast<const char*>(&compression_code), sizeof(compression_code));
        output.write(reinterpret_cast<const char*>(&error_correction_code), sizeof(error_correction_code));
    }

    static uint64_t read_and_validate_header(std::ifstream& archive) {
        ArchiveHeader header;
        archive.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        if (archive.gcount() != sizeof(header)) {
            throw std::runtime_error("Invalid archive: incomplete header");
        }

        if (std::memcmp(header.signature, SIGNATURE, 6) != 0) {
            throw std::runtime_error("Invalid archive: incorrect signature");
        }

        if (header.version != FORMAT_VERSION) {
            throw std::runtime_error("Unsupported archive version: " + std::to_string(header.version));
        }

        uint8_t compression_code, error_correction_code;
        archive.read(reinterpret_cast<char*>(&compression_code), sizeof(compression_code));
        archive.read(reinterpret_cast<char*>(&error_correction_code), sizeof(error_correction_code));

        if (compression_code != NO_COMPRESSION || error_correction_code != NO_ERROR_CORRECTION) {
            throw std::runtime_error("Unsupported compression or error correction algorithm");
        }

        return header.original_size;
    }

    static void process_data(std::istream& input, std::ostream& output, 
                            uint64_t total_size, bool invert) {
        std::vector<char> buffer(BUFFER_SIZE);
        uint64_t processed = 0;

        while (processed < total_size) {
            const size_t to_read = std::min(BUFFER_SIZE, 
                                           static_cast<size_t>(total_size - processed));
            
            input.read(buffer.data(), to_read);
            const std::streamsize bytes_read = input.gcount();
            
            if (bytes_read == 0) {
                break;
            }

            if (invert) {
                invert_bits(buffer.data(), bytes_read);
            } else {
                invert_bits(buffer.data(), bytes_read);
            }

            output.write(buffer.data(), bytes_read);
            processed += bytes_read;
        }

        if (processed != total_size) {
            throw std::runtime_error("Data size mismatch: expected " + 
                                   std::to_string(total_size) + 
                                   ", got " + std::to_string(processed));
        }
    }

    static void invert_bits(char* data, std::streamsize size) {
        for (std::streamsize i = 0; i < size; ++i) {
            data[i] = ~data[i];
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 4) {
            std::cerr << "Usage:" << std::endl;
            std::cerr << "  Encode: " << argv[0] << " -c <input_file> <archive_file>" << std::endl;
            std::cerr << "  Decode: " << argv[0] << " -d <archive_file> <output_file>" << std::endl;
            return 1;
        }

        const std::string mode = argv[1];
        const std::string input_file = argv[2];
        const std::string output_file = argv[3];

        if (mode == "-c") {
            ArchiveProcessor::encode(input_file, output_file);
            std::cout << "File successfully encoded to archive: " << output_file << std::endl;
        } else if (mode == "-d") {
            ArchiveProcessor::decode(input_file, output_file);
            std::cout << "Archive successfully decoded to: " << output_file << std::endl;
        } else {
            std::cerr << "Unknown mode: " << mode << std::endl;
            std::cerr << "Use -c for encoding or -d for decoding" << std::endl;
            return 1;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}
