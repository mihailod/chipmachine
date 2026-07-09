#include "mkhebios.h"

#include "binary/compression/zlib_stream.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

void Check(bool condition, const char* msg)
{
  if (!condition)
  {
    throw std::runtime_error(msg);
  }
}

class Output
{
public:
  Output()
  {
    std::cout << std::hex << std::setfill('0');
  }

  ~Output()
  {
    std::cout << std::endl;
  }

  void operator<<(uint8_t byte)
  {
    if (0 == (idx++ % 32))
    {
      std::cout << '\n';
    }
    std::cout << "0x" << std::setw(2) << static_cast<uint_t>(byte) << std::setw(-1) << ", ";
  }

  void operator<<(uint32_t dword)
  {
    if (0 == (idx++ % 8))
    {
      std::cout << '\n';
    }
    std::cout << "0x" << std::setw(8) << dword << std::setw(-1) << ", ";
  }

private:
  uint_t idx = 0;
};

int main(int argc, const char* argv[])
{
  Check(argc >= 2, "$0 <ps2bios> [zlib]");
  const std::string inputFilename(argv[1]);
  const std::string mode = argc > 2 ? std::string(argv[2]) : std::string();
  Check(mode == "zlib" || mode == "", "Invalid mode");

  std::ifstream input(inputFilename.c_str(), std::ios::binary);
  input.seekg(0, std::ios::end);
  const int biosSize = input.tellg();
  input.seekg(0);
  std::vector<char> bios(biosSize);
  Check(!input.read(bios.data(), biosSize).fail(), "Failed to read");

  int heBiosSize = biosSize;
  uint32_t* heBios = static_cast<uint32_t*>(::mkhebios_create(bios.data(), &heBiosSize));
  Check(0 == heBiosSize % 4, "Invalid result size");

  std::cout << "// Source=" << inputFilename << " size=" << biosSize << " extracted=" << heBiosSize << std::endl;

  if (mode == "zlib")
  {
    Binary::DataBuilder packed(heBiosSize);
    {
      Binary::DataInputStream input({heBios, heBiosSize});
      Binary::Compression::Zlib::Compress(input, packed);
      std::cout << "// Compressed=" << packed.Size() << std::endl;
    }
    Output out;
    for (const auto *it = static_cast<const uint8_t*>(packed.Get(0)), *lim = it + packed.Size(); it != lim; ++it)
    {
      out << *it;
    }
  }
  else if (mode == "")
  {
    Output out;
    for (const auto *it = heBios, *lim = heBios + heBiosSize / sizeof(*heBios); it != lim; ++it)
    {
      out << *it;
    }
  }

  ::mkhebios_delete(heBios);

  return 0;
}
