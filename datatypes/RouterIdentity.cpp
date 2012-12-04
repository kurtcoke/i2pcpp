#include "RouterIdentity.h"

namespace i2pcpp {
	RouterIdentity::RouterIdentity(ByteArray const &publicKey, ByteArray const &signingKey, Certificate const &certificate) : m_certificate(certificate)
	{
		copy(publicKey.begin(), publicKey.end(), m_publicKey.begin());
		copy(signingKey.begin(), signingKey.end(), m_signingKey.begin());
	}

	RouterIdentity::RouterIdentity(ByteArray::const_iterator &idItr) : m_certificate(Certificate(idItr))
	{
		copy(idItr, idItr + 256, m_publicKey.begin()), idItr += 256;
		copy(idItr, idItr + 128, m_signingKey.begin()), idItr += 128;
	}

	ByteArray RouterIdentity::getBytes() const
	{
		ByteArray b, cert;

		cert = m_certificate.getBytes();

		b.insert(b.end(), m_publicKey.begin(), m_publicKey.end());
		b.insert(b.end(), m_signingKey.begin(), m_signingKey.end());
		b.insert(b.end(), cert.begin(), cert.end());

		return b;
	}
}
