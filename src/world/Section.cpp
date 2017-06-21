#include <world/Section.hpp>


Section::Section(Vector position, byte *dataBlocks, size_t dataBlocksLength, byte *dataLight, byte *dataSky,
                 byte bitsPerBlock,
                 std::vector<unsigned short> palette) {
	worldPosition = position;

	m_dataBlocksLen = dataBlocksLength;
	m_dataBlocks = new byte[m_dataBlocksLen];
	std::copy(dataBlocks, dataBlocks + m_dataBlocksLen, m_dataBlocks);

	m_dataLight = new byte[2048];
	std::copy(dataLight, dataLight + 2048, m_dataLight);

	if (dataSky) {
		m_dataSkyLight = new byte[2048];
		std::copy(dataSky, dataSky + 2048, m_dataSkyLight);
	}

	m_palette = palette;
	m_bitsPerBlock = bitsPerBlock;
}

Section::~Section() {
	delete[] m_dataBlocks;
	m_dataBlocksLen = 0;
	m_dataBlocks = nullptr;
	delete[] m_dataLight;
	m_dataLight = nullptr;
	delete[] m_dataSkyLight;
	m_dataSkyLight = nullptr;
}

Block &Section::GetBlock(Vector pos) {
	if (m_dataBlocks != nullptr) {
		std::mutex parseMutex;
		std::unique_lock<std::mutex> parseLocker(parseMutex);
		parseWaiter.wait(parseLocker);
		while (m_dataBlocks != nullptr) {
			parseWaiter.wait(parseLocker);
		}
		LOG(WARNING) << "Successfully waited for block render!";
	}
	return m_blocks[pos.GetY() * 256 + pos.GetZ() * 16 + pos.GetX()];
}

void Section::Parse() {
	if (m_dataBlocks == nullptr)
		return;

	long long *longArray = reinterpret_cast<long long *>(m_dataBlocks);
	for (size_t i = 0; i < m_dataBlocksLen / 8; i++)
		endswap(&longArray[i]);
	std::vector<unsigned short> blocks;
	blocks.reserve(4096);
	int bitPos = 0;
	unsigned short t = 0;
	for (size_t i = 0; i < m_dataBlocksLen; i++) {
		for (int j = 0; j < 8; j++) {
			t |= (m_dataBlocks[i] & 0x01) ? 0x80 : 0x00;
			t >>= 1;
			m_dataBlocks[i] >>= 1;
			bitPos++;
			if (bitPos >= m_bitsPerBlock) {
				bitPos = 0;
				t >>= m_bitsPerBlock - 1;
				blocks.push_back(t);
				t = 0;
			}
		}
	}

	std::vector<byte> light;
	light.reserve(4096);
	for (int i = 0; i < 2048; i++) {
		byte t = m_dataLight[i];
		byte first = t & 0b11110000;
		byte second = t >> 4;
		light.push_back(first);
		light.push_back(second);
	}
	for (int i = 0; i < 4096; i++) {
		unsigned short blockId = m_palette.size() > 0 ? m_palette[blocks[i]] : blocks[i];
		Block block(blockId >> 4, blockId & 0xF);
		m_blocks.push_back(block);
	}
	if ((light.size() + blocks.size()) / 2 != 4096) {
		throw 118;
	}
	delete[] m_dataBlocks;
	m_dataBlocksLen = 0;
	m_dataBlocks = nullptr;
	delete[] m_dataLight;
	m_dataLight = nullptr;
	delete[] m_dataSkyLight;
	m_dataSkyLight = nullptr;

	parseWaiter.notify_all();
}

Section &Section::operator=(Section other) {
	std::swap(*this, other);
	return *this;
}

void swap(Section &a, Section &b) {
	using std::swap;
	swap(a.m_dataBlocksLen, b.m_dataBlocksLen);
	swap(a.m_dataBlocks, b.m_dataBlocks);
	swap(a.m_dataLight, b.m_dataLight);
	swap(a.m_dataSkyLight, b.m_dataSkyLight);
	swap(a.m_blocks, b.m_blocks);
	swap(a.m_palette, b.m_palette);
	swap(a.m_bitsPerBlock, b.m_bitsPerBlock);
}

Section::Section(const Section &other) {
	worldPosition = other.worldPosition;
	m_dataBlocksLen = other.m_dataBlocksLen;
	m_dataBlocks = new byte[m_dataBlocksLen];
	std::copy(other.m_dataBlocks, other.m_dataBlocks + m_dataBlocksLen, m_dataBlocks);

	m_dataLight = new byte[2048];
	std::copy(other.m_dataLight, other.m_dataLight + 2048, m_dataLight);

	if (other.m_dataSkyLight) {
		m_dataSkyLight = new byte[2048];
		std::copy(other.m_dataSkyLight, other.m_dataSkyLight + 2048, m_dataSkyLight);
	}

	m_palette = other.m_palette;
	m_bitsPerBlock = other.m_bitsPerBlock;
}

Vector Section::GetPosition() {
	return worldPosition;
}
