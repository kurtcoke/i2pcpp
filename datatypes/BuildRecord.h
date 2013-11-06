#ifndef BUILDRECORD_H
#define BUILDRECORD_H

#include <bitset>
#include <memory>

#include <botan/elgamal.h>

#include "Datatype.h"
#include "ByteArray.h"
#include "SessionKey.h"

namespace i2pcpp {
	class BuildRecord : public Datatype {
		public:
			BuildRecord();
			BuildRecord(ByteArrayConstItr &begin, ByteArrayConstItr end);

			BuildRecord& operator=(BuildRecord const &rec);

			ByteArray serialize() const;

			void encrypt(ByteArray const &encryptionKey);
			void decrypt(Botan::ElGamal_PrivateKey const *key);
			void encrypt(StaticByteArray<16, true> const &iv, SessionKey const &key);
			void decrypt(StaticByteArray<16, true>  const &iv, SessionKey const &key);

			void setHeader(const std::array<unsigned char, 16> &header);
			const std::array<unsigned char, 16>& getHeader() const;

		protected:
			std::array<unsigned char, 16> m_header;
			ByteArray m_data;
	};

	typedef std::shared_ptr<BuildRecord> BuildRecordPtr;
}

#endif
