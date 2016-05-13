
#include "catch/catch.hpp"
#include "qsh/qshfile.h"

#include <iostream>
#include <fstream>
#include <ctime>

using namespace std;
using namespace qsh;

static const std::string flagStr[] = {
	"NonZeroReplAct",
	"SessIdChanged",
	"Add",
	"Fill",
	"Buy",
	"Sell",
	"",
	"Quote",
	"Counter",
	"NonSystem",
	"EndOfTransaction",
	"FillOrKill",
	"Moved",
	"Canceled",
	"CancelledGroup",
	"CrossTrade"
};

class Sink
{
public:
	Sink()
	{
	}

	void orderLogFrame(const QshFile<Sink>::OrderLogEntry& entry)
	{
		orderLog.push_back(entry);
	}

	std::vector<QshFile<Sink>::OrderLogEntry> orderLog;

	std::string toString()
	{
		std::string result;
		for(const auto& entry : orderLog)
		{
			result += entryToString(entry) + '\n';
		}
		return result;
	}

	std::string entryToString(const QshFile<Sink>::OrderLogEntry& entry)
	{
		std::string result;
		auto tp = helpers::convertGrowDatetimeToTimePoint(entry.frameTimestamp);
		struct tm* t = gmtime(&tp.first);
		char buf[64];
		sprintf(buf, "%02d.%02d.%04d %02d:%02d:%02d.%03d;", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
				t->tm_hour, t->tm_min, t->tm_sec, tp.second / 1000);

		result += buf;

		tp = helpers::convertGrowDatetimeToTimePoint(entry.timestamp);
		t = gmtime(&tp.first);
		sprintf(buf, "%02d.%02d.%04d %02d:%02d:%02d.%03d;", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
				t->tm_hour, t->tm_min, t->tm_sec, tp.second / 1000);
		result += buf;
		result += std::to_string(entry.orderId) + ";";
		result += std::to_string(entry.orderPrice.value) + ";";
		result += std::to_string(entry.volume) + ";";
		result += std::to_string(entry.remain) + ";";
		result += std::to_string(entry.matchingOrderId) + ";";
		result += std::to_string(entry.tradePrice.value) + ";";
		result += std::to_string(entry.openInterest) + ";";
		result += flagsToString(entry.flags);
		return result;
	}

	std::string flagsToString(uint16_t flags)
	{
		std::string result;
		bool first = true;
		for(int i = 0; i < 16; i++)
		{
			if(flags & (1 << i))
			{
				if(!first)
				{
					result += ", ";
				}
				else
					first = false;
				result += flagStr[i];
			}
		}
		return result;
	}

};

TEST_CASE("QshFile", "")
{
	ifstream stream("data/OrdLog.VTBR-6.16.2016-04-26.qsh", ios_base::binary | ios_base::in);

	REQUIRE(stream.good());

	Sink sink;
	QshFile<Sink> file(stream, sink);
	using OrderLogEntry = QshFile<Sink>::OrderLogEntry;

	SECTION("Metadata parsing")
	{
		auto meta = file.getMetadata();

		REQUIRE(meta.applicationName == "QshWriter.5904");
		REQUIRE(meta.comment == "Zerich QSH Service");

		auto tp = helpers::convertDatetimeToTimePoint(meta.startTime);
		struct tm* t = gmtime(&tp.first);
		REQUIRE(t->tm_year + 1900 == 2016);
		REQUIRE(t->tm_mon + 1 == 4);
		REQUIRE(t->tm_mday == 26);
		REQUIRE(t->tm_hour == 6);
		REQUIRE(t->tm_min == 59);
		REQUIRE(t->tm_sec == 50);
	}

	SECTION("Stream header")
	{
		auto streams = file.streams();
		REQUIRE(streams.size() == 1);

		auto stream = streams[0];
		REQUIRE(stream.type == QshFile<Sink>::StreamType::OrdLog);
		REQUIRE(stream.ticker == "VTBR-6.16");
		REQUIRE(stream.numId == 813877);
		REQUIRE(stream.step == Approx(1));
		REQUIRE(stream.connector == "Plaza2");
	}

	SECTION("One frame")
	{
		file.readOneFrame();
		auto entry = sink.orderLog.front();
		REQUIRE(entry.orderId == 21024239476ull);
		REQUIRE(entry.orderPrice == decimal_fixed(7000, 0));
		REQUIRE(entry.volume == 2);
		REQUIRE(entry.remain == 2);
		REQUIRE(entry.tradePrice.value == 0);
		REQUIRE(entry.matchingOrderId == 0);
		REQUIRE(entry.openInterest == 0);
		REQUIRE(entry.flags == (OrderLogEntry::SessIdChanged | OrderLogEntry::Add | OrderLogEntry::Buy | OrderLogEntry::Quote | OrderLogEntry::EndOfTransaction));

		auto tp = helpers::convertGrowDatetimeToTimePoint(entry.timestamp);
		struct tm* t = gmtime(&tp.first);
		REQUIRE(t->tm_year + 1900 == 2016);
		REQUIRE(t->tm_mon + 1 == 4);
		REQUIRE(t->tm_mday == 25);
		REQUIRE(t->tm_hour == 18);
		REQUIRE(t->tm_min == 51);
		REQUIRE(t->tm_sec == 53);
		REQUIRE(tp.second == 306000);

		tp = helpers::convertGrowDatetimeToTimePoint(entry.frameTimestamp);
		t = gmtime(&tp.first);
		REQUIRE(t->tm_year + 1900 == 2016);
		REQUIRE(t->tm_mon + 1 == 4);
		REQUIRE(t->tm_mday == 26);
		REQUIRE(t->tm_hour == 6);
		REQUIRE(t->tm_min == 40);
		REQUIRE(t->tm_sec == 14);
		REQUIRE(tp.second == 353000);

		SECTION("Second frame")
		{
			file.readOneFrame();
			auto entry = sink.orderLog.back();
			REQUIRE(entry.orderId == 21024239486ull);
			REQUIRE(entry.orderPrice == decimal_fixed(6901, 0));
			REQUIRE(entry.volume == 1);
			REQUIRE(entry.flags == (OrderLogEntry::Add | OrderLogEntry::Buy | OrderLogEntry::Quote | OrderLogEntry::EndOfTransaction));
		}
	}

	SECTION("All frames")
	{
		file.readAllFrames();

	}
}

