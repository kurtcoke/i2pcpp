#include "gtest/gtest.h"

#include <boost/asio.hpp>

#include <botan/botan.h>

#include "../exceptions/FormattingError.h"

#include "../datatypes/Endpoint.h"
#include "../datatypes/Date.h"
#include "../datatypes/Certificate.h"
#include "../datatypes/Mapping.h"
#include "../datatypes/RouterIdentity.h"
#include "../datatypes/RouterAddress.h"
#include "../datatypes/RouterInfo.h"

#include "../util/Base64.h"

//#include "../transport/UDPTransport.h"
#include "../transport/ssu/Packet.h"

#include <fstream>

#include "routerInfo.cpp"

#define SSU_TEST_IP "127.0.0.1"
#define SSU_TEST_PORT 55555
#define SSU_TEST_BAD_IP "127.0.0.2"

// TODO Not comprehensive.
TEST(Datatypes, Endpoint) {
	using namespace i2pcpp;

	Endpoint e1("127.0.0.1", 1234);
	Endpoint e2("127.0.0.1", 1234);
	Endpoint e3("127.0.0.1", 1235);

	ASSERT_EQ(e1, e2);
	ASSERT_FALSE(e2 == e3);

	ASSERT_EQ(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234), e1.getUDPEndpoint());
}

TEST(Datatypes, Date) {
	using namespace i2pcpp;

	Date d1(3553611897179156816);
	ByteArray d1_serialized = d1.serialize();
	ByteArray expected_data = { 0x31, 0x50, 0xF7, 0x31, 0x50, 0xF7, 0x31, 0x50 };
	EXPECT_EQ(d1_serialized, expected_data);

	auto d1_itr = d1_serialized.cbegin();
	Date d2(d1_itr, d1_serialized.cend());
	ByteArray d2_serialized = d2.serialize();
	EXPECT_EQ(d2_serialized, expected_data);

	ByteArray bad_data = { 0x00, 0x01, 0x02 };
	auto bad_data_itr = bad_data.cbegin();
	ASSERT_THROW(Date(bad_data_itr, bad_data.cend()), FormattingError);

	// Check to see that it increments the iterator properly.
	ByteArray good_data = { 0x31, 0x50, 0xF7, 0x31, 0x50, 0xF7, 0x31, 0x50, 0x31, 0x50, 0xF7, 0x31, 0x50, 0xF7, 0x31, 0x50 };
	auto good_data_itr = good_data.cbegin();
	Date d3(good_data_itr, good_data.cend());
	Date d4(good_data_itr, good_data.cend());
	EXPECT_EQ(d3.serialize(), expected_data);
	EXPECT_EQ(d4.serialize(), expected_data);
}

TEST(Datatypes, Certificate) {
	using namespace i2pcpp;

	Certificate c1;
	ByteArray c1_serialized = c1.serialize();
	ByteArray expected_data = { 0x00, 0x00, 0x00 };
	ASSERT_EQ(c1_serialized, expected_data);

	ByteArray good_data = { 0x01, 0x00, 0x10, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x01, 0x00, 0x10, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
	auto good_data_itr = good_data.cbegin();
	Certificate c2(good_data_itr, good_data.cend());
	ASSERT_EQ(c2.getLength(), 0x10);

	Certificate c3(good_data_itr, good_data.cend());
	ASSERT_EQ(c3.serialize(), ByteArray(good_data.cbegin(), good_data.cbegin() + 0x13));

	ByteArray bad_data1 = { 0x05, 0x00, 0x00 };
	auto bad_data1_itr = bad_data1.cbegin();
	ASSERT_THROW(Certificate(bad_data1_itr, bad_data1.cend()), FormattingError);

	ByteArray bad_data2 = { 0x01, 0xDE, 0xAD, 0xBE, 0xEF };
	auto bad_data2_itr = bad_data2.cbegin();
	ASSERT_THROW(Certificate(bad_data2_itr, bad_data2.cend()), FormattingError);
}

TEST(Datatypes, Mapping) {
	using namespace i2pcpp;

	Mapping m1;
	m1.setValue("hello", "world");
	m1.setValue("foo", "bar");

	ByteArray m1_serialized = m1.serialize();
	ByteArray expected_data = { 0x00, 0x18, 0x03, 'f', 'o', 'o', '=', 0x03, 'b', 'a', 'r', ';', 0x05, 'h', 'e', 'l', 'l', 'o', '=', 0x05, 'w', 'o', 'r', 'l', 'd', ';' };

	ASSERT_EQ(m1_serialized, expected_data);

	auto m1_itr = m1_serialized.cbegin();
	Mapping m2(m1_itr, m1_serialized.cend());
	ASSERT_EQ(m2.getValue("hello"), "world");
	ASSERT_EQ(m2.getValue("foo"), "bar");

	m2.deleteValue("foo");
	ASSERT_EQ(m2.getValue("foo"), "");

	ByteArray bad = expected_data;
	bad[0] = 0xFF;
	bad[1] = 0xFF;
	auto bad_itr = bad.cbegin();
	ASSERT_THROW(Mapping(bad_itr, bad.cend()), FormattingError);

	bad = expected_data;
	bad[2] = 0x04;
	bad_itr = bad.cbegin();
	ASSERT_THROW(Mapping(bad_itr, bad.cend()), FormattingError);

	bad = expected_data;
	bad[7] = 0x04;
	bad_itr = bad.cbegin();
	ASSERT_THROW(Mapping(bad_itr, bad.cend()), FormattingError);
}

TEST(Datatypes, RouterIdentity) {
	using namespace i2pcpp;

	auto sample = sample_routerInfo.cbegin();
	RouterIdentity r1(sample, sample_routerInfo.cend());
	ASSERT_EQ(r1.getHashEncoded(), "1pp0rQV7hK~XsLib8o8AHX74kWHmRjDsmDqF7aigZD0=");

	ASSERT_EQ(r1.serialize(), ByteArray(sample_routerInfo.cbegin(), sample_routerInfo.cbegin() + 256 + 128 + 3));

	sample = sample_routerInfo.cbegin();
	ASSERT_THROW(RouterIdentity(sample, sample + 32), FormattingError);

	sample = sample_routerInfo.cbegin();
	ASSERT_THROW(RouterIdentity(sample, sample + 384), FormattingError);
}

TEST(Datatypes, RouterAddress) {
	using namespace i2pcpp;

	auto sample = sample_routerInfo.cbegin() + 256 + 128 + 3 + 8 + 1;
	RouterAddress r1(sample, sample_routerInfo.cend());

	sample = sample_routerInfo.cbegin() + 256 + 128 + 3 + 8 + 1;
	ASSERT_EQ(r1.serialize(), ByteArray(sample, sample + 48));
}

// TODO Not comprehensive.
TEST(Datatypes, RouterInfo) {
	using namespace i2pcpp;

	auto sample = sample_routerInfo.cbegin();
	RouterInfo r1(sample, sample_routerInfo.cend());

	ASSERT_EQ(r1.serialize(), sample_routerInfo);

	ASSERT_TRUE(r1.verifySignature(group));

	ASSERT_EQ(r1.getOptions().getValue("netdb.knownLeaseSets"), "37");
	ASSERT_EQ(r1.getOptions().getValue("caps"), "OfR");
}

TEST(Datatypes, SessionKey) {
	using namespace i2pcpp;

	ByteArray sk_bytes = { 0x00 };
	ASSERT_THROW(SessionKey sk(sk_bytes), std::logic_error);

	sk_bytes = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };

	SessionKey sk(sk_bytes);
}

TEST(Utils, Base64) {
	using namespace i2pcpp;

	ByteArray test = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };

	ASSERT_EQ(Base64::encode(test), "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=");
	ASSERT_EQ(Base64::decode("AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8="), test);
}

// TODO
/*TEST(UDPTransport, start) {
	using namespace i2pcpp;

	Botan::LibraryInitializer init;
	Botan::AutoSeeded_RNG rng;
	Botan::DSA_PrivateKey key(rng, group);
	UDPTransport t(SessionKey(Base64::decode("A6DVqs4yCV1s9QalgeB28iiV6341qm88Gblf3-c1SVg=")), group, key);
	t.start(Endpoint(SSU_TEST_IP, SSU_TEST_PORT));
	ASSERT_THROW(t.start(Endpoint(SSU_TEST_BAD_IP, SSU_TEST_PORT)), boost::system::system_error);
}*/

TEST(Packet, encrypt) {
	using namespace i2pcpp;
	using namespace i2pcpp::SSU;

	ByteArray sk_bytes = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };
	ByteArray mk_bytes = sk_bytes;
	reverse(mk_bytes.begin(), mk_bytes.end());
	ByteArray iv_bytes = { 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef,0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef };

	Botan::InitializationVector iv(iv_bytes.data(), iv_bytes.size());
	SessionKey sk(sk_bytes), mk(mk_bytes);
	Packet p(Endpoint("127.0.0.1", 12345), (unsigned char *)"test data", 9);
	p.encrypt(iv, sk, mk);

	ByteArray expected = { 0x7a, 0xf9, 0x45, 0x94, 0x95, 0x5e, 0x48, 0x9d, 0x5e, 0x5f, 0xa0, 0x8a, 0x33, 0x70, 0x64, 0xd7, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xcd, 0xae, 0xf7, 0xb3, 0x31, 0xa6, 0x33, 0x61, 0xbf, 0x83, 0x60, 0xae, 0x10, 0xea, 0x4f, 0x6f };

	ASSERT_EQ(p.getData(), expected);
}

TEST(Packet, verify) {
	using namespace i2pcpp;   
	using namespace i2pcpp::SSU;

	Packet p(Endpoint("127.0.0.1", 12345));
	ByteArray mk = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };
	reverse(mk.begin(), mk.end());

	p.getData() = { 0x7a, 0xf9, 0x45, 0x94, 0x95, 0x5e, 0x48, 0x9d, 0x5e, 0x5f, 0xa0, 0x8a, 0x33, 0x70, 0x64, 0xd7, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xcd, 0xae, 0xf7, 0xb3, 0x31, 0xa6, 0x33, 0x61, 0xbf, 0x83, 0x60, 0xae, 0x10, 0xea, 0x4f, 0x6f };
	ASSERT_TRUE(p.verify(mk));

	p.getData() = { 0x7a, 0xf9, 0x45, 0x94, 0x95, 0x5e, 0x48, 0x9d, 0x5e, 0x5f, 0xa0, 0x8a, 0x33, 0x70, 0x64, 0xd7, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xcd, 0xae, 0xf7, 0xb3, 0x31, 0xa6, 0x33, 0x61, 0xbf, 0x83, 0x60, 0xae, 0x10, 0xea, 0x4f, 0x6e };
	ASSERT_FALSE(p.verify(mk));
}

TEST(Packet, decrypt) {
	using namespace i2pcpp;
	using namespace i2pcpp::SSU;

	ByteArray sk_bytes = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };
	ByteArray mk_bytes = sk_bytes;
	reverse(mk_bytes.begin(), mk_bytes.end());

	SessionKey sk(sk_bytes), mk(mk_bytes);

	Packet p(Endpoint("127.0.0.1", 12345));
	p.getData() = { 0x7a, 0xf9, 0x45, 0x94, 0x95, 0x5e, 0x48, 0x9d, 0x5e, 0x5f, 0xa0, 0x8a, 0x33, 0x70, 0x64, 0xd7, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xbe, 0xef, 0xcd, 0xae, 0xf7, 0xb3, 0x31, 0xa6, 0x33, 0x61, 0xbf, 0x83, 0x60, 0xae, 0x10, 0xea, 0x4f, 0x6f };
	p.decrypt(sk);

	ByteArray expected = { 't', 'e', 's', 't', ' ', 'd', 'a', 't', 'a', 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07 };
	ASSERT_EQ(p.getData(), expected);
}
