#include <iostream>
#include <vector>
#include <cstdint>
#include <array>
#include <string>
#include <stdexcept>
#include <iomanip>

namespace Protocol {

    using Byte = uint8_t;
    constexpr std::size_t SIG_SIZE = 6;
    const std::array<Byte, SIG_SIZE> SIGNATURE_V1 = {0x4F, 0x54, 0x49, 0x4B, 0x56, 0x31};

    struct HeaderData {
        uint8_t version_major;
        uint8_t version_minor;
        uint16_t compression_id;
        uint16_t protection_id;
        uint64_t original_size;
        uint32_t extra_header_len;
    };

    class BinarySerializer {
    public:
        static void put_u8(std::vector<Byte>& buf, uint8_t v) {
            buf.push_back(v);
        }

        static void put_u16(std::vector<Byte>& buf, uint16_t v) {
            buf.push_back(v & 0xFF);
            buf.push_back((v >> 8) & 0xFF);
        }

        static void put_u32(std::vector<Byte>& buf, uint32_t v) {
            for (int i = 0; i < 4; ++i) buf.push_back((v >> (i * 8)) & 0xFF);
        }

        static void put_u64(std::vector<Byte>& buf, uint64_t v) {
            for (int i = 0; i < 8; ++i) buf.push_back((v >> (i * 8)) & 0xFF);
        }
    };

    class HeaderBuilder {
    public:
        static std::vector<Byte> serialize(const HeaderData& data) {
            std::vector<Byte> buffer;
            buffer.reserve(32);

            for (auto b : SIGNATURE_V1) buffer.push_back(b);
            
            BinarySerializer::put_u8(buffer, data.version_major);
            BinarySerializer::put_u8(buffer, data.version_minor);
            BinarySerializer::put_u16(buffer, data.compression_id);
            BinarySerializer::put_u16(buffer, data.protection_id);
            BinarySerializer::put_u64(buffer, data.original_size);
            BinarySerializer::put_u32(buffer, data.extra_header_len);
            
            return buffer;
        }

        static void inspect_layout() {
            std::cout << "OTIK-V1 Header Layout Specification (Little-Endian Enforced)\n";
            std::cout << "========================================================\n";
            
            size_t offset = 0;
            auto print_field = [&](const std::string& name, size_t size) {
                std::cout << std::setw(4) << offset << " | " << std::setw(20) << std::left << name 
                          << " | " << size << " bytes" << std::endl;
                offset += size;
            };

            print_field("Signature", 6);
            print_field("Ver Major", 1);
            print_field("Ver Minor", 1);
            print_field("Compression ID", 2);
            print_field("Protection ID", 2);
            print_field("Original Size", 8);
            print_field("Extra Hdr Size", 4);
            
            std::cout << "--------------------------------------------------------\n";
            std::cout << "Total Header Size: " << offset << " bytes" << std::endl;
        }
    };
}

int main() {
    Protocol::HeaderBuilder::inspect_layout();

    Protocol::HeaderData dummy = {1, 0, 0x00, 0x00, 1048576, 0};
    auto binary_blob = Protocol::HeaderBuilder::serialize(dummy);

    if (binary_blob.size() != 24) {
        std::cerr << "Spec Violation: Header size mismatch!" << std::endl;
        return 1;
    }

    std::cout << "\nSerialization Test: PASSED. Blob size: " << binary_blob.size() << " bytes." << std::endl;
    return 0;
}
