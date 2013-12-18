#ifndef STATICBYTEARRAY_H
#define STATICBYTEARRAY_H

#include <algorithm>
#include <array>
#include <string>
#include <functional>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "../util/Base64.h"

#include "ByteArray.h"

namespace i2pcpp {
	template<unsigned int L>
		class StaticByteArray {
			public:
				typedef unsigned char value_type;
				typedef typename std::array<value_type, L>::size_type size_type;

				StaticByteArray()
				{
					m_data.fill(0);
				}

				StaticByteArray(const StaticByteArray &) = default;
				StaticByteArray& operator=(const StaticByteArray &) = default;
				StaticByteArray(StaticByteArray &&) = default;
				StaticByteArray& operator=(StaticByteArray &&) = default;

				~StaticByteArray()
				{
					m_data.fill(0);
				}

				StaticByteArray(ByteArray const &b)
				{
					if(b.size() < L)
						std::copy(b.cbegin(), b.cbegin() + b.size(), m_data.begin());
					else
						std::copy(b.cbegin(), b.cbegin() + L, m_data.begin());
				}

				StaticByteArray(std::string const &s)
				{
					ByteArray b = Base64::decode(s);
					if(b.size() != L) throw std::runtime_error("input string not correct size for StaticByteArray");
					std::copy(b.cbegin(), b.cbegin() + L, m_data.begin());
				}

				StaticByteArray(std::array<value_type, L> const &a)
				{
					std::copy(a.cbegin(), a.cbegin() + L, m_data.begin());
				}

				std::string toHex() const
				{
					std::stringstream s;
					for(auto c: m_data) s << std::setw(2) << std::setfill('0') << std::hex << (int)c;

					return s.str();
				}

				typename std::array<value_type, L>::iterator begin()
				{
					return m_data.begin();
				}

				typename std::array<value_type, L>::iterator end()
				{
					return m_data.end();
				}

				typename std::array<value_type, L>::const_iterator cbegin() const
				{
					return m_data.cbegin();
				}

				typename std::array<value_type, L>::const_iterator cend() const
				{
					return m_data.cend();
				}

				typename std::array<value_type, L>::pointer data()
				{
					return m_data.data();
				}

				typename std::array<value_type, L>::const_pointer data() const
				{
					return m_data.data();
				}

				typename std::array<value_type, L>::size_type size() const
				{
					return m_data.size();
				}

				bool operator<(const StaticByteArray<L> &sba) const
				{
					return m_data < sba.m_data;
				}

				typename std::array<value_type, L>::reference operator[](typename std::array<value_type, L>::size_type n)
				{
					return m_data[n];
				}

				typename std::array<value_type, L>::const_reference operator[](typename std::array<value_type, L>::size_type n) const
				{
					return m_data[n];
				}

				bool operator!=(const StaticByteArray<L> &sba) const
				{
					return m_data != sba.m_data;
				}

				bool operator==(const StaticByteArray<L> &sba) const
				{
					return m_data == sba.m_data;
				}

				operator ByteArray() const
				{
					return ByteArray(m_data.cbegin(), m_data.cend());
				}

				operator std::string() const
				{
					std::string b64;
					b64 = Base64::encode(ByteArray(m_data.cbegin(), m_data.cend()));
					std::replace(b64.begin(), b64.end(), '+', '-');
					std::replace(b64.begin(), b64.end(), '/', '~');

					return b64;
				}

			private:
				std::array<value_type, L> m_data;
		};

	template<unsigned int L>
	std::ostream& operator<<(std::ostream &s, StaticByteArray<L> const &sba)
	{
		s << std::string(sba);
		return s;
	}
}

namespace std {
	template<unsigned int L>
		struct hash<i2pcpp::StaticByteArray<L>> {
			public:
				size_t operator()(const i2pcpp::StaticByteArray<L> &sba) const
				{
					hash<string> f;
					return f(sba);
				}
		};
}

#endif
