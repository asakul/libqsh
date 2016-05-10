
#include "catch/catch.hpp"
#include "qsh/qshfile.h"

#include <iostream>
#include <fstream>

using namespace std;
using namespace qsh;

class Sink
{
public:
	Sink()
	{
	}
};

TEST_CASE("QshFile", "")
{
	fstream stream("data/OrdLog.VTBR-6.16.2016-04-26.qsh", ios_base::binary | ios_base::in);

	REQUIRE(stream.good());

	Sink sink;
	QshFile<Sink> file(stream, sink);

	SECTION("Metadata parsing")
	{
		auto meta = file.getMetadata();

		REQUIRE(meta.applicationName == "QshWriter.5904");
		REQUIRE(meta.comment == "Zerich QSH Service");
	}
}

