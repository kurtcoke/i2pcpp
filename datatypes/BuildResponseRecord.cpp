/**
 * @file BuildResponseRecord.cpp
 * @brief implements BuildReponseRecord.h
 */
#include "BuildResponseRecord.h"

#include <stdexcept>

#include <botan/auto_rng.h>
#include <botan/pipe.h>
#include <botan/lookup.h>

#include "../exceptions/FormattingError.h"

namespace i2pcpp {
	BuildResponseRecord::BuildResponseRecord(const BuildRecord &r) :
		BuildRecord(r) {}

	BuildResponseRecord::BuildResponseRecord(Reply r) :
		m_reply(r) {}

	bool BuildResponseRecord::parse()
	{
		m_reply = (Reply)m_data.back();

		Botan::Pipe hashPipe(new Botan::Hash_Filter("SHA-256"));
		hashPipe.start_msg();
		hashPipe.write(m_data.data() + 16, m_data.size() - 16);
		hashPipe.end_msg();

		size_t size = hashPipe.remaining();
		ByteArray calcHash(size);
		hashPipe.read(calcHash.data(), size);

		ByteArray givenHash(32);
		std::copy(m_header.cbegin(), m_header.cend(), givenHash.begin());
		std::copy(m_data.cbegin(), m_data.cbegin() + 16, givenHash.begin() + m_header.size());

		if(givenHash != calcHash)
			return false;

		return true;
	}

	void BuildResponseRecord::compile()
	{
		Botan::AutoSeeded_RNG rng;

		ByteArray response(496);
		rng.randomize(response.data(), 495);
		response[495] = (unsigned char)m_reply;

		Botan::Pipe hashPipe(new Botan::Hash_Filter("SHA-256"));
		hashPipe.start_msg();
		hashPipe.write(response.data(), response.size());
		hashPipe.end_msg();

		size_t size = hashPipe.remaining();
		if(size != 32)
			throw std::runtime_error("hash is not 32 bytes");

		hashPipe.read(m_header.data(), 16);
		m_data.resize(512);
		hashPipe.read(m_data.data(), 16);
		std::copy(response.cbegin(), response.cend(), m_data.begin() + 16);
	}

	BuildResponseRecord::Reply BuildResponseRecord::getReply() const
	{
		return m_reply;
	}
}
