#ifndef DATATYPE_H
#define DATATYPE_H

#include "ByteArray.h"

namespace i2pcpp {
	class Datatype {
		public:
			virtual ByteArray getBytes() const = 0;
	};
}

#endif
